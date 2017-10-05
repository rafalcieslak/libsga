#include <sga/window.hpp>
#include "window.impl.hpp"

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <functional>

#include "global.hpp"
#include "utils.hpp"
#include "scheduler.hpp"

namespace sga{

// ====== Glue layer ======

Window::Window(unsigned int width, unsigned int height, std::string title) :
  impl(std::make_shared<Window::Impl>(width, height, title)) {}
Window::~Window() = default;

void Window::nextFrame() {impl->nextFrame();}
void Window::setFPSLimit(double fps) {impl->setFPSLimit(fps);}
float Window::getLastFrameDelta() const {return impl->getLastFrameDelta();}
float Window::getAverageFPS() const {return impl->getAverageFPS();}
float Window::getAverageFrameTime() const {return impl->getAverageFrameTime();}
unsigned int Window::getFrameNo() const {return impl->getFrameNo();}

bool Window::isOpen() {return impl->isOpen();}
void Window::close() {impl->close();}

bool Window::isFullscreen() {return impl->isFullscreen();}
void Window::setFullscreen(bool fullscreen) {impl->setFullscreen(fullscreen);}
void Window::toggleFullscreen() {impl->toggleFullscreen();}

bool Window::isKeyPressed(Key k) {return impl->isKeyPressed(k);}
void Window::setOnKeyDown(sga::Key k, std::function<void()> f){
  impl->setOnKeyDown(k, f);
}
void Window::setOnKeyUp(sga::Key k, std::function<void()> f){
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
void Window::grabMouse() { impl->grabMouse(); }
void Window::releaseMouse() { impl->releaseMouse(); }

unsigned int Window::getWidth() {return impl->getWidth();}
unsigned int Window::getHeight() {return impl->getHeight();}
void Window::setOnResize(std::function<void (unsigned int, unsigned int)> f) {impl->setOnResize(f);}

void Window::setClearColor(ImageClearColor cc) {return impl->setClearColor(cc);}

// ====== IMPL ======

Window::Impl::Impl(unsigned int width, unsigned int height, std::string title) :
  clearColor(ImageFormat::NInt8, 3) {
    // Do not create OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Do not minimize fullscreen windows on focus loss
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    
    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    glfwSetWindowUserPointer(window, this);

    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
    
    surface = global::instance->createSurface(window);
    ensurePhysicalDeviceSurfaceSupport(surface);
    
    // Query supported surface formats.
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = global::physicalDevice->getSurfaceFormats(surface);
    
    // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
    // there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
      colorFormat = vk::Format::eB8G8R8A8Unorm;
    else
      colorFormat = surfaceFormats[0].format;
    depthFormat = vk::Format::eD24UnormS8Uint;
    
    // Create the renderpass used for drawing onto this window.
    // Note that it may be shared between multiple pipelines!
    vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);  
    renderPass = global::device->createRenderPass(
      { vk::AttachmentDescription( // attachment 0
          {}, colorFormat, vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, // color
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil
          vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR
          ),
        vk::AttachmentDescription( // attachment 1
          {}, depthFormat, vk::SampleCountFlagBits::e1,
          vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, // depth
          vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil
          vk::ImageLayout::eUndefined,vk::ImageLayout::eDepthStencilAttachmentOptimal
          )
      },
      vk::SubpassDescription( {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1,
                              &colorReference, nullptr,
                              &depthReference, 0, nullptr),
      nullptr );
    
    // This will also create the initial swapchain.
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

  createSwapchainsAndFramebuffer();
  out_dbg("Performed window resize to " + std::to_string(w) + "x" + std::to_string(h));
  frameno = 0;

  clearCurrentFrame();
  
  width = w;
  height = h;

  if(f_onResize) f_onResize(width,height);
}

void Window::Impl::createSwapchainsAndFramebuffer(){
  // Before creating the new framebuffer stapchain, the old one must be destroyed.
  framebufferSwapchain.reset();
  
  framebufferSwapchain.reset(
    new vkhlf::FramebufferSwapchain(
      global::device,
      surface,
      colorFormat, 
      depthFormat,
      renderPass,
      // TODO: TransferDst is not guaranteed to be supported!
      // See: https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#_surface_queries
      vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
      std::vector<vk::PresentModeKHR>{vk::PresentModeKHR::eImmediate}
      )
    );
}

void Window::Impl::nextFrame() {
  if(!isOpen()) return;

  // Very important to sync here. We're about to commit a new frame, so we need
  // to finish all (potentially time-consuming) GPU actions before we measure
  // time and proceed to present the frame.
  Scheduler::sync();  
  double frameReadyTimestamp = glfwGetTime();
    
  // Wait with presentation for a while, if fpslimit is enabled.
  if(frameno > 0 && fpsLimit > 0){
    double timeSinceLastFrame = frameReadyTimestamp - lastFrameTimestamp;
    double desiredTime = (1.0/fpsLimit)*0.95;
    double time_left = desiredTime - timeSinceLastFrame;
    if(time_left > 0.0){
      //std::cout << "Sleeping for " << time_left << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(int(time_left * 1000)));
    }
  }

  if(framebufferSwapchain){
    if(frameno > 0){
      if(!currentFrameRendered){
        std::cout << "SGA WARNING: Nothing was rendered onto current frame, skipping it." << std::endl;
        clearCurrentFrame(vk::ClearColorValue(std::array<float,4>({1.0f, 0.0f, 1.0f, 1.0f})));
      }
      
      Scheduler::presentSynced(framebufferSwapchain);
    }
    
    /* WSI interface doesn't use our cmd queue. We still full-sync manually,
     * though. */
    auto fence = global::device->createFence(false);
    framebufferSwapchain->acquireNextFrame(UINT64_MAX, fence, true);
    fence->wait(UINT64_MAX);
    
    // DO NOT clear new frame. User clears it with Pipeline::clear().
  }

  double newFrameTimestamp = glfwGetTime();
  
  // Compute time deltas
  lastFrameDelta = newFrameTimestamp - lastFrameTimestamp;
  lastFrameTime = frameReadyTimestamp - lastFrameTimestamp;
  lastFrameTimestamp = newFrameTimestamp;

  const double fpsFilterTheta = 0.85, ftimeFilterTheta = 0.7;
  averagedFrameDelta =
    averagedFrameDelta * fpsFilterTheta +
    lastFrameDelta * (1.0 - fpsFilterTheta);
  averagedFrameTime =
    averagedFrameTime * ftimeFilterTheta +
    lastFrameTime * (1.0 - ftimeFilterTheta);

  // Update frame counters
  frameno++;
  totalFrameNo++;
  currentFrameRendered = false;
  
  glfwPollEvents();
}

void Window::Impl::setClearColor(ImageClearColor cc){
  if(cc.getComponents() != 3)
    ImageFormatError("ClearChannelMismatch", "The clear color for a window must have 3 values, while this one has " + std::to_string(cc.getComponents()) + ".").raise();
  // TODO: Print out human-readable format name!
  if(cc.getFormat() != ImageFormat::NInt8)
    ImageFormatError("ClearFormatMismatch", "The clear color used for clearning a window must use NInt8 format.").raise();

  clearColor = cc;
}

void Window::Impl::clearCurrentFrame(){
  clearCurrentFrame(Utils::imageClearColorToVkClearColorValue(clearColor));
}

void Window::Impl::clearCurrentFrame(vk::ClearColorValue cc){
  if(!framebufferSwapchain)
    return;
  Scheduler::buildAndSubmitSynced("Clearing frame", [&](std::shared_ptr<vkhlf::CommandBuffer> cmdBuffer){
      // Clear the new frame.
      auto image = framebufferSwapchain->getColorImage();
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
      
      cmdBuffer->clearColorImage(image, vk::ImageLayout::eTransferDstOptimal, vk::ClearColorValue(cc));
      
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal);

      // Clear the depth buffer.
      image = framebufferSwapchain->getDepthImage();
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eDepth,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
      cmdBuffer->clearDepthStencilImage(image, vk::ImageLayout::eTransferDstOptimal, 1.0f, 0, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eDepth,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    });
}

void Window::Impl::setFPSLimit(double fps){
  // TODO: Document fps < 0 for no FPS limit
  if(fps > 0.0 && fps < 1.0) fps = 1.0;
  fpsLimit = fps;
}

float Window::Impl::getLastFrameDelta() const{
  return lastFrameDelta;
}
float Window::Impl::getAverageFPS() const{
  return 1.0 / averagedFrameDelta;
}
float Window::Impl::getAverageFrameTime() const{
  return averagedFrameTime;
}
unsigned int Window::Impl::getFrameNo() const{
  return totalFrameNo;
}

bool Window::Impl::isOpen() {
  glfwPollEvents();
  return !glfwWindowShouldClose(window);
}

void Window::Impl::close(){
  glfwHideWindow(window);
  glfwSetWindowShouldClose(window, 1);
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
}
void Window::Impl::setOnMouseButton(std::function<void(bool, bool)> f) {
  f_onMouseButton = f;
}
void Window::Impl::setOnMouseAny(std::function<void(double, double, bool, bool)> f) {
  f_onMouseAny = f;
}
void Window::Impl::grabMouse(){
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwGetCursorPos(window, &mouse_x_offset, &mouse_y_offset);
  mouse_grabbed = true;
}
void Window::Impl::releaseMouse(){
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  mouse_x_offset = mouse_y_offset = 0;
  mouse_grabbed = false;
}

void Window::Impl::setOnResize(std::function<void (unsigned int, unsigned int)> f){
  f_onResize = f;
}

bool Window::Impl::isKeyPressed(sga::Key k){
  auto it = keyState.find(k);
  if(it == keyState.end())
    return false;
  return it->second;
}

void Window::Impl::resizeCallback(GLFWwindow *window, int width, int height){
  Window::Impl * wd = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
  wd->do_resize(width, height);
}
void Window::Impl::mousePositionCallback(GLFWwindow *window, double x, double y){
  Window::Impl * wd = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
  x -= wd->mouse_x_offset;
  y -= wd->mouse_y_offset;
  wd->mouse_x = x;
  wd->mouse_y = wd->mouse_grabbed ? y : (wd->height - y);
  if(wd->f_onMouseMove)
    wd->f_onMouseMove(wd->mouse_x, wd->mouse_y);
  if(wd->f_onMouseAny)
    wd->f_onMouseAny(wd->mouse_x, wd->mouse_y, wd->mouse_l, wd->mouse_r);
}
void Window::Impl::mouseButtonCallback(GLFWwindow *window, int button, int action, int){
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

  // Ignore key repeats.
  if(action == GLFW_REPEAT)
    return;
  
  // Update keyState map.
  wd->keyState[k] = (action == GLFW_PRESS);

  // Ignore key repeat events
  if(action == GLFW_REPEAT)
    return;
  
  // Trigger registered user function.
  const auto& fmap = (action == GLFW_PRESS) ? wd->fmap_onKeyDown : wd->fmap_onKeyUp;
  auto it = fmap.find(k);
  if(it != fmap.end())
    it->second();
}

} // namespace sga
