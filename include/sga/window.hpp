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
  void nextFrame();
  bool isOpen();

  void close();
  void setOnKeyDown(Key k, std::function<void()>);
  void setOnKeyUp(Key k, std::function<void()>);
  
  void setFPSLimit(double fps);
  static std::shared_ptr<Window> create(unsigned int width, unsigned int height, std::string title) {
    return std::shared_ptr<Window>(new Window(width, height, title));
  }

  void setOnMouseMove(std::function<void(double, double)> f);
  void setOnMouseButton(std::function<void(bool, bool)> f);
  void setOnMouseAny(std::function<void(double, double, bool, bool)> f);

  unsigned int getWidth();
  unsigned int getHeight();
  
  friend class Pipeline;
  friend class Image;
private:
  Window(unsigned int width, unsigned int height, std::string title);
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};
  
} // namespace sga

#endif // __SGA_WINDOW_HPP__
