#include <sga/window.hpp>

#include <iostream>
#include <cassert>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vkhlf/vkhlf.h>

#include "global.hpp"
#include "utils.hpp"

namespace sga{


class Window::Impl{
public:
  Impl(unsigned int width, unsigned int height, std::string title) :
    width(width), height(height) {
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
  
  void do_resize(unsigned int w, unsigned int h){
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
    assert(framebufferSwapchain->getExtent() == vk::Extent2D(w, h));

    width = w;
    height = h;
    // TODO: Call user handler for resize.
  }

  void nextFrame() {
    // Present current framebuffer. Wait until it's done.
    //   \/ semaphore?
    // Acquire next swapchain framebuffer. Wait until it's done.
    //   \/ fence?
    
  }

  bool getShouldClose() {
    return glfwWindowShouldClose(window);
  }
  
  static void resizeCallback(GLFWwindow *window, int width, int height){
    Window::Impl * wd = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
    wd->do_resize(width, height);
  }

private:
  GLFWwindow* window;
  unsigned int width, height;
  
  vk::Format colorFormat;
  vk::Format depthFormat;
  
  std::shared_ptr<vkhlf::Surface> surface;
  std::shared_ptr<vkhlf::RenderPass> renderPass;
  std::unique_ptr<vkhlf::FramebufferSwapchain> framebufferSwapchain;

};

Window::Window(unsigned int width, unsigned int height, std::string title) :
  impl(std::make_unique<Window::Impl>(width, height, title)) {}
Window::~Window() = default;

void Window::nextFrame() {impl->nextFrame();}
bool Window::getShouldClose() {return impl->getShouldClose();}

} // namespace sga
