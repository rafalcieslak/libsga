#include <sga/window.hpp>
#include "window.impl.hpp"

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

#include "global.hpp"
#include "utils.hpp"

namespace sga{

Window::Window(unsigned int width, unsigned int height, std::string title) :
  impl(std::make_unique<Window::Impl>(width, height, title)) {}
Window::~Window() = default;

void Window::nextFrame() {impl->nextFrame();}
bool Window::isOpen() {return impl->isOpen();}
void Window::setFPSLimit(double fps) {return impl->setFPSLimit(fps);}

// ====== IMPL ======

Window::Impl::Impl(unsigned int width, unsigned int height, std::string title){
    // Do not create OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    glfwSetWindowUserPointer(window, this);
    
    surface = impl_global::instance->createSurface(window);
    ensurePhysicalDeviceSurfaceSupport(surface);
    
    // Query supported surface formats.
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = impl_global::physicalDevice->getSurfaceFormats(surface);
    
    // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
    // there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined){
      colorFormat = vk::Format::eB8G8R8A8Unorm;
      // colorSpace = surfaceFormats[0].colorSpace;
    }else{
      colorFormat = surfaceFormats[0].format;
      // colorSpace = surfaceFormats[0].colorSpace;
    }
    depthFormat = vk::Format::eD24UnormS8Uint;
    
    vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);  
    renderPass = impl_global::device->createRenderPass(
      { vk::AttachmentDescription( // attachment 0
          {}, colorFormat, vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, // color
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil
          vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
          ),
        vk::AttachmentDescription( // attachment 1
          {}, depthFormat, vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, // depth
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil
          vk::ImageLayout::eUndefined,vk::ImageLayout::eDepthStencilAttachmentOptimal
          )
      },
      vk::SubpassDescription( {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1,
                              &colorReference, nullptr,
                              &depthReference, 0, nullptr),
      nullptr );

    // This will create the initial swapchain.
    do_resize(width, height);

    // Hook some glfw callbacks.
    glfwSetFramebufferSizeCallback(window, Window::Impl::resizeCallback);
}
  
void Window::Impl::do_resize(unsigned int w, unsigned int h){
    if (w == 0 || h == 0) return;
    int w2, h2;
    glfwGetFramebufferSize(window, &w2, &h2); w = w2; h = h2;
    if (w == width && h == height) return;
    
    // Before creating the new framebuffer stapchain, the old one must be destroyed.
    framebufferSwapchain.reset();
    
    framebufferSwapchain.reset(
      new vkhlf::FramebufferSwapchain(
        impl_global::device,
        surface,
        colorFormat, 
        depthFormat,
        renderPass)
      );
    frameno = 0;
    //std::cout << framebufferSwapchain->getExtent().width << " " << framebufferSwapchain->getExtent().height << std::endl;
    //std::cout << w << " " << h << std::endl;
    //std::cout << width << " " << height << std::endl;
    //std::cout << "===" << std::endl;

    width = w;
    height = h;
    // TODO: Call user handler for resize.
  }

void Window::Impl::nextFrame() {
  
  // Wait with presentation for a while, if fpslimit is enabled.
  if(frameno > 0 && fpsLimit > 0){
    double prev = limitFPS_lastTime;
    double now  = glfwGetTime();
    double timeSinceLastFrame = now - prev;
    double desiredTime = 1.0/fpsLimit;
    double time_left = desiredTime - timeSinceLastFrame;
    if(time_left > 0.0){
      std::this_thread::sleep_for(std::chrono::milliseconds(int(time_left * 1000)));
    }
    limitFPS_lastTime = glfwGetTime();
  }
  
    if(frameno > 0){
      if(!currentFrameRendered){
        std::cout << "SGA WARNING: Nothing was rendered onto current frame, skipping it." << std::endl;

        // TODO: Consider always clearing target immediatelly after acquiring a new frame. This may simplify things a lot (but may bost some performance).
        
        // TODO: Use pink to indicate unrendered frames?
        std::array<float, 4> clear_color = { 0.0f, 0.0f, 0.0f };
        
        auto cmdBuffer = impl_global::commandPool->allocateCommandBuffer();
        cmdBuffer->begin();
        cmdBuffer->beginRenderPass(renderPass, framebufferSwapchain->getFramebuffer(),
                                   vk::Rect2D({ 0, 0 }, framebufferSwapchain->getExtent()),
                                   { vk::ClearValue(clear_color), vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0)) },
                                   vk::SubpassContents::eInline);
        cmdBuffer->endRenderPass();
        cmdBuffer->end();
        
        auto fence = impl_global::device->createFence(false);
        impl_global::queue->submit(
          vkhlf::SubmitInfo{
            {},{}, cmdBuffer, {} },
          fence
          );
        fence->wait(UINT64_MAX);
      }
      
      //std::cout << "PRESENTING" << std::endl;
      framebufferSwapchain->present(impl_global::queue);
      impl_global::queue->waitIdle();
    }
    
    //std::cout << "ACQUIRING" << std::endl;
    auto fence = impl_global::device->createFence(false);
    framebufferSwapchain->acquireNextFrame(UINT64_MAX, fence, true);
    fence->wait(UINT64_MAX);

    frameno++;

    glfwPollEvents();
}

bool Window::Impl::isOpen() {
    glfwPollEvents();
    return !glfwWindowShouldClose(window);
}

void Window::Impl::setFPSLimit(double fps){
  // -1 for no limit
  if(fps > 0.0 && fps < 1.0) fps = 1.0;
  fpsLimit = fps;
}
  
void Window::Impl::resizeCallback(GLFWwindow *window, int width, int height){
    Window::Impl * wd = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
    wd->do_resize(width, height);
}

} // namespace sga
