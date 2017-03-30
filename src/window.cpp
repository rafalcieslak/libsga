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

bool Window::isFullscreen(){ return impl->isFullscreen(); }
void Window::setFullscreen(bool fullscreen){ impl->setFullscreen(fullscreen); }
void Window::toggleFullscreen(){ impl->toggleFullscreen(); }

unsigned int Window::getWidth() {return impl->getWidth();}
unsigned int Window::getHeight() {return impl->getHeight();}

void Window::close() {impl->close();}

void Window::setOnKeyDown(sga::Key k, std::function<void ()> f){
  impl->setOnKeyDown(k, f);
}
void Window::setOnKeyUp(sga::Key k, std::function<void ()> f){
  impl->setOnKeyUp(k, f);
}
void Window::setOnMouseMove(std::function<void(double, double)> f) {
  impl->setOnMouseMove(f);
}
void Window::setOnMouseButton(std::function<void(bool, bool)> f) {
  impl->setOnMouseButton(f);
}
void Window::setOnMouseAny(std::function<void(double, double, bool, bool)> f) {
  impl->setOnMouseAny(f);
}

// ====== IMPL ======

Window::Impl::Impl(unsigned int width, unsigned int height, std::string title){
    // Do not create OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Do not minimize fullscreen windows on focus loss
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    
    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    glfwSetWindowUserPointer(window, this);

    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);
    
    surface = global::instance->createSurface(window);
    ensurePhysicalDeviceSurfaceSupport(surface);
    
    // Query supported surface formats.
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = global::physicalDevice->getSurfaceFormats(surface);
    
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
    renderPass = global::device->createRenderPass(
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
    glfwSetCursorPosCallback(window, Window::Impl::mousePositionCallback);
    glfwSetMouseButtonCallback(window, Window::Impl::mouseButtonCallback);
    glfwSetKeyCallback(window, Window::Impl::keyCallback);

    // These fields do not require initialization, but just to make sure:
    stored_winpos_width = width;
    stored_winpos_height = height;
}

Window::Impl::~Impl(){
  glfwDestroyWindow(window);
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
      global::device,
      surface,
      colorFormat, 
      depthFormat,
      renderPass)
    );
  out_dbg("Performed window resize to " + std::to_string(w) + "x" + std::to_string(h));
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

  if(!isOpen()) return;
  
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
        
        auto cmdBuffer = global::commandPool->allocateCommandBuffer();
        cmdBuffer->begin();
        cmdBuffer->beginRenderPass(renderPass, framebufferSwapchain->getFramebuffer(),
                                   vk::Rect2D({ 0, 0 }, framebufferSwapchain->getExtent()),
                                   { vk::ClearValue(clear_color), vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0)) },
                                   vk::SubpassContents::eInline);
        cmdBuffer->endRenderPass();
        cmdBuffer->end();
        
        auto fence = global::device->createFence(false);
        global::queue->submit(
          vkhlf::SubmitInfo{
            {},{}, cmdBuffer, {} },
          fence
          );
        fence->wait(UINT64_MAX);
      }
      
      //std::cout << "PRESENTING" << std::endl;
      framebufferSwapchain->present(global::queue);
      global::queue->waitIdle();
    }
    
    //std::cout << "ACQUIRING" << std::endl;
    auto fence = global::device->createFence(false);
    framebufferSwapchain->acquireNextFrame(UINT64_MAX, fence, true);
    fence->wait(UINT64_MAX);

    frameno++;

    glfwPollEvents();
}

bool Window::Impl::isOpen() {
  glfwPollEvents();
  return !glfwWindowShouldClose(window);
}

void Window::Impl::close(){
  glfwHideWindow(window);
  glfwSetWindowShouldClose(window, 1);
}

void Window::Impl::setFPSLimit(double fps){
  // -1 for no limit
  if(fps > 0.0 && fps < 1.0) fps = 1.0;
  fpsLimit = fps;
}

bool Window::Impl::isFullscreen(){
  return fullscreen;
}
void Window::Impl::toggleFullscreen(){
  setFullscreen(!fullscreen);
}
void Window::Impl::setFullscreen(bool f){
  fullscreen = f;
  if(!fullscreen){
    glfwSetWindowMonitor(window, NULL,
                         stored_winpos_x, stored_winpos_y,
                         stored_winpos_width, stored_winpos_height, 0);
    do_resize(stored_winpos_width, stored_winpos_height);
  }else{
    glfwGetWindowPos(window, &stored_winpos_x, &stored_winpos_y);
    glfwGetWindowSize(window, &stored_winpos_width, &stored_winpos_height);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (monitor){
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
      do_resize(mode->width, mode->height);
    }else{
      fullscreen = false;
    }
    
  }
}

void Window::Impl::setOnKeyDown(sga::Key k, std::function<void ()> f){
  fmap_onKeyDown[k] = f;
}
void Window::Impl::setOnKeyUp(sga::Key k, std::function<void ()> f){
  fmap_onKeyUp[k] = f;
}

void Window::Impl::setOnMouseMove(std::function<void(double, double)> f) {
  f_onMouseMove = f;
};
void Window::Impl::setOnMouseButton(std::function<void(bool, bool)> f) {
  f_onMouseButton = f;
};
void Window::Impl::setOnMouseAny(std::function<void(double, double, bool, bool)> f) {
  f_onMouseAny = f;
};

void Window::Impl::resizeCallback(GLFWwindow *window, int width, int height){
  Window::Impl * wd = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
  wd->do_resize(width, height);
}
void Window::Impl::mousePositionCallback(GLFWwindow *window, double x, double y){
  Window::Impl * wd = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
  wd->mouse_x = x;
  wd->mouse_y = wd->height - y;
  if(wd->f_onMouseMove)
    wd->f_onMouseMove(wd->mouse_x, wd->mouse_y);
  if(wd->f_onMouseAny)
    wd->f_onMouseAny(wd->mouse_x, wd->mouse_y, wd->mouse_l, wd->mouse_r);
}
void Window::Impl::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods){
  Window::Impl * wd = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
  if(button == GLFW_MOUSE_BUTTON_1){
    wd->mouse_l = action==GLFW_PRESS;
  }
  if(button == GLFW_MOUSE_BUTTON_2){
    wd->mouse_r = action==GLFW_PRESS;
  }
  if(wd->f_onMouseButton)
    wd->f_onMouseButton(wd->mouse_l, wd->mouse_r);
  if(wd->f_onMouseAny)
    wd->f_onMouseAny(wd->mouse_x, wd->mouse_y, wd->mouse_l, wd->mouse_r);
}

void Window::Impl::keyCallback(GLFWwindow* window, int key, int, int action, int){
  Window::Impl * wd = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
  
  Key k = glfwKeyToSgaKey(key);

  // Update keyState map.
  wd->keyState[k] = (action == GLFW_PRESS);

  // Trigger registered user function.
  const auto& fmap = (action == GLFW_PRESS) ? wd->fmap_onKeyDown : wd->fmap_onKeyUp;
  auto it = fmap.find(k);
  if(it != fmap.end())
    it->second();
}

} // namespace sga
