#ifndef __SGA_WINDOW_HPP__
#define __SGA_WINDOW_HPP__

#include <memory>
#include <string>

#include "config.hpp"

namespace sga{

class Window{
public:
  ~Window();
  void nextFrame();
  bool isOpen();
  void setFPSLimit(double fps);
  static std::shared_ptr<Window> create(unsigned int width, unsigned int height, std::string title) {
    return std::shared_ptr<Window>(new Window(width, height, title));
  }

  friend class Pipeline;
private:
  Window(unsigned int width, unsigned int height, std::string title);
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};
  
} // namespace sga

#endif // __SGA_WINDOW_HPP__
