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
private:
  class Impl;
  std::unique_ptr<Impl> impl;
};
  
} // namespace sga

#endif // __SGA_WINDOW_HPP__
