#ifndef __SGA_PIPELINE_HPP__
#define __SGA_PIPELINE_HPP__

#include <memory>

#include "window.hpp"

namespace sga{

class Pipeline{
public:
  Pipeline();
  ~Pipeline();

  void setTarget(std::shared_ptr<Window> target);
  void drawTestTriangle();
  
private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_PIPELINE_HPP__
