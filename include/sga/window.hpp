#ifndef __SGA_WINDOW_HPP__
#define __SGA_WINDOW_HPP__

#include <memory>
#include <string>

namespace sga{

class Window{
public:
  Window(unsigned int width, unsigned int height, std::string title);
  ~Window();
  void nextFrame();
  bool getShouldClose();
  void limitFPS(double fps);
  static std::shared_ptr<Window> create(unsigned int width, unsigned int height, std::string title) {
    return std::make_shared<Window>(width, height, title);
  }

  friend class Pipeline;
private:
  class Impl;
  std::unique_ptr<Impl> impl;
};
  
} // namespace sga

#endif // __SGA_WINDOW_HPP__
