#ifndef __SGA_WINDOW_HPP__
#define __SGA_WINDOW_HPP__

#include <memory>
#include <string>

#include "config.hpp"
#include "keys.hpp"

namespace sga{

class Window{
public:
  ~Window();
  
  static std::shared_ptr<Window> create(unsigned int width, unsigned int height, std::string title) {
    return std::shared_ptr<Window>(new Window(width, height, title));
  }

  // Frame display and statistics
  void nextFrame();
  void setFPSLimit(double fps);
  float getLastFrameDelta() const;
  float getAverageFPS() const;
  unsigned int getFrameNo() const;

  // Window state
  bool isOpen();
  void close();

  // Fullscreen options
  bool isFullscreen();
  void setFullscreen(bool fullscreen);
  void toggleFullscreen();
  
  // Keyboard support
  bool isKeyPressed(Key k);
  void setOnKeyDown(Key k, std::function<void()>);
  void setOnKeyUp(Key k, std::function<void()>);

  // Mouse support
  void setOnMouseMove(std::function<void(double, double)> f);
  void setOnMouseButton(std::function<void(bool, bool)> f);
  void setOnMouseAny(std::function<void(double, double, bool, bool)> f);

  // Window size
  unsigned int getWidth();
  unsigned int getHeight();
  void setOnResize(std::function<void(double, double)> f);
  
  friend class Pipeline;
  friend class Image;
private:
  Window(unsigned int width, unsigned int height, std::string title);
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};
  
} // namespace sga

#endif // __SGA_WINDOW_HPP__
