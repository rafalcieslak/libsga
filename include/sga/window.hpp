#ifndef __SGA_WINDOW_HPP__
#define __SGA_WINDOW_HPP__

#include <memory>
#include <string>
#include <functional>

#include "config.hpp"
#include "keys.hpp"
#include "image.hpp"

namespace sga{

class Window{
public:
  SGA_API Window(unsigned int width, unsigned int height, std::string title);
  SGA_API ~Window();

  // Frame display and statistics
  SGA_API void nextFrame();
  SGA_API void setFPSLimit(double fps);
  SGA_API float getLastFrameDelta() const;
  SGA_API float getAverageFPS() const;
  SGA_API float getAverageFrameTime() const;
  SGA_API unsigned int getFrameNo() const;

  // Window state
  SGA_API bool isOpen();
  SGA_API void close();

  // Fullscreen options
  SGA_API bool isFullscreen();
  SGA_API void setFullscreen(bool fullscreen);
  SGA_API void toggleFullscreen();
  
  // Keyboard support
  SGA_API bool isKeyPressed(Key k);
  SGA_API void setOnKeyDown(Key k, std::function<void()>);
  SGA_API void setOnKeyUp(Key k, std::function<void()>);

  // Mouse support
  SGA_API void setOnMouseMove(std::function<void(double, double)> f);
  SGA_API void setOnMouseButton(std::function<void(bool, bool)> f);
  SGA_API void setOnMouseAny(std::function<void(double, double, bool, bool)> f);
  SGA_API void grabMouse();
  SGA_API void releaseMouse();

  // Window size
  SGA_API unsigned int getWidth();
  SGA_API unsigned int getHeight();
  SGA_API void setOnResize(std::function<void(unsigned int, unsigned int)> f);
  
  void setClearColor(ImageClearColor cc);
  
  friend class Pipeline;
  friend class Image;
private:
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};
  
} // namespace sga

#endif // __SGA_WINDOW_HPP__
