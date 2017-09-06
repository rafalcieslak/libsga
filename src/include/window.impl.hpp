#ifndef __WINDOW_IMPL_HPP__
#define __WINDOW_IMPL_HPP__

#include <sga/window.hpp>
#include <sga/image.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vkhlf/vkhlf.h>

namespace sga{

class Window::Impl{
public:
  Impl(unsigned int width, unsigned int height, std::string title);
  ~Impl();
  
  void nextFrame();
  void setFPSLimit(double fps);
  float getLastFrameDelta() const;
  float getAverageFPS() const;
  float getAverageFrameTime() const;
  unsigned int getFrameNo() const;
  
  void do_resize(unsigned int w, unsigned int h);
  
  bool isOpen();
  void close();

  bool isKeyPressed(Key k);
  void setOnKeyDown(Key k, std::function<void()>);
  void setOnKeyUp(Key k, std::function<void()>);
  
  bool isFullscreen();
  void setFullscreen(bool fullscreen);
  void toggleFullscreen();
  
  void setOnMouseMove(std::function<void(double, double)> f);
  void setOnMouseButton(std::function<void(bool, bool)> f);
  void setOnMouseAny(std::function<void(double, double, bool, bool)> f);

  void grabMouse();
  void releaseMouse();

  unsigned int getWidth() {return width;}
  unsigned int getHeight() {return height;}
  void setOnResize(std::function<void(unsigned int, unsigned int)> f);
  
  static void resizeCallback(GLFWwindow *window, int width, int height);
  static void mousePositionCallback(GLFWwindow *window, double width, double height);
  static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
  static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

  void setClearColor(ImageClearColor cc);
  void clearCurrentFrame();
  void clearCurrentFrame(vk::ClearColorValue cc);

  void createSwapchainsAndFramebuffer();
  void setRenderPass(vkhlf::RenderPass);
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
  // Number of frames displayed with this swapchain - gets reset on each resize,
  // screen mode change etc.
  unsigned int frameno = 0;
  // Total number of frames displayed on this window since its creation -
  // efectively this is the number of times NextFrame was called.
  unsigned int totalFrameNo = 0;

  // Curent FPS limit.
  float fpsLimit = -1;
  // Time when the last frame was drawn.
  double lastFrameTimestamp = 0.0;
  // Total time between the last frame was acquired to the time it was ready for
  // presentation.
  double lastFrameTime = 0.0;
  // Time between last two drawn frames. May be higher than lastFrameTime due to VSync etc.
  double lastFrameDelta = 0.0;
  // Low-pass filtered time between frames
  double averagedFrameDelta = 0.0;
  // Low-pass filtered time to render a frame.
  double averagedFrameTime = 0.0;

  float mouse_x = 0.0;
  float mouse_y = 0.0;
  bool mouse_l = false;
  bool mouse_r = false;
  // These are used to pretend that after mouse cursor is grabbed, it's centered at (0, 0).
  double mouse_x_offset = 0.0, mouse_y_offset = 0.0;
  bool mouse_grabbed = false;

  // Color used when clearing this window.
  ImageClearColor clearColor;
  
  std::function<void(double, double)> f_onMouseMove;
  std::function<void(bool, bool)> f_onMouseButton;
  std::function<void(double, double, bool, bool)> f_onMouseAny;
  std::function<void(unsigned int, unsigned int)> f_onResize;
  
  std::map<Key, std::function<void()>> fmap_onKeyDown;
  std::map<Key, std::function<void()>> fmap_onKeyUp;
  std::map<Key, bool> keyState;
};

} // namespace sga

#endif // __WINDOW_IMPL_HPP__
