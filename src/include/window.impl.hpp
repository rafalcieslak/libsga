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
  ~Impl();
  
  void do_resize(unsigned int w, unsigned int h);
  void nextFrame();
  bool isOpen();
  // TODO: Automatically compute FPS.
  void setFPSLimit(double fps);

  bool isFullscreen();
  void setFullscreen(bool fullscreen);
  void toggleFullscreen();
  
  static void resizeCallback(GLFWwindow *window, int width, int height);
  static void mousePositionCallback(GLFWwindow *window, double width, double height);
  static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
  static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

  unsigned int getWidth() {return width;}
  unsigned int getHeight() {return height;}

  bool isKeyPressed(Key k);
  
  void close();
  
  void setOnKeyDown(Key k, std::function<void()>);
  void setOnKeyUp(Key k, std::function<void()>);
  void setOnMouseMove(std::function<void(double, double)> f);
  void setOnMouseButton(std::function<void(bool, bool)> f);
  void setOnMouseAny(std::function<void(double, double, bool, bool)> f); 
private:
  GLFWwindow* window;
  unsigned int width, height;

  // These are used for restoring window pos/size when returning from fullscreen
  // mode.
  int stored_winpos_x = 0, stored_winpos_y = 0;
  int stored_winpos_width, stored_winpos_height;

  bool fullscreen = false;
  
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
  std::shared_ptr<vkhlf::Image> getCurrentImage(){
    return framebufferSwapchain->getColorImage();
  }
private:
  std::unique_ptr<vkhlf::FramebufferSwapchain> framebufferSwapchain;
  
private:
  unsigned int frameno = 0;
  
  double fpsLimit = 60;
  double limitFPS_lastTime = 0.0;

  double mouse_x = 0.0;
  double mouse_y = 0.0;
  bool mouse_l = false;
  bool mouse_r = false;

  std::function<void(double, double)> f_onMouseMove;
  std::function<void(bool, bool)> f_onMouseButton;
  std::function<void(double, double, bool, bool)> f_onMouseAny;

  std::map<Key, std::function<void()>> fmap_onKeyDown;
  std::map<Key, std::function<void()>> fmap_onKeyUp;
  std::map<Key, bool> keyState;
};

} // namespace sga

#endif // __WINDOW_IMPL_HPP__
