#ifndef __WINDOW_IMPL_HPP__
#define __WINDOW_IMPL_HPP__

#include <sga/window.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vkhlf/vkhlf.h>

namespace sga{

class Window::Impl{
public:
  Impl(unsigned int width, unsigned int height, std::string title);
  
  void do_resize(unsigned int w, unsigned int h);
  void nextFrame();
  bool isOpen();
  void setFPSLimit(double fps);
  static void resizeCallback(GLFWwindow *window, int width, int height);
  
private:
  GLFWwindow* window;
  unsigned int width, height;
  
  vk::Format colorFormat;
  vk::Format depthFormat;
  
  std::shared_ptr<vkhlf::Surface> surface;
public: // TODO: friend Pipeline? getter?
  std::shared_ptr<vkhlf::RenderPass> renderPass;
  // This gets flipped to true when there is data for current frame available for presentation.
  bool currentFrameRendered = false;

  std::pair<
    std::shared_ptr<vkhlf::Framebuffer>,
    vk::Extent2D>
  getCurrentFramebuffer(){
    return std::make_pair(
      framebufferSwapchain->getFramebuffer(),
      framebufferSwapchain->getExtent()
      );
  }
private:
  std::unique_ptr<vkhlf::FramebufferSwapchain> framebufferSwapchain;
  
private:
  unsigned int frameno = 0;
  
  double fpsLimit = 60;
  double limitFPS_lastTime = 0.0;
};

} // namespace sga

#endif // __WINDOW_IMPL_HPP__
