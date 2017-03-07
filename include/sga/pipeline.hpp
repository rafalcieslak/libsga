#ifndef __SGA_PIPELINE_HPP__
#define __SGA_PIPELINE_HPP__

#include <memory>

#include "window.hpp"

namespace sga{

class VBOBase;
class VertexShader;
class FragmentShader;

class Pipeline{
public:
  Pipeline();
  ~Pipeline();

  void setTarget(std::shared_ptr<Window> target);
  void drawVBO(std::shared_ptr<VBOBase>);
  void setClearColor(float r, float g, float b);

  void setVertexShader(std::shared_ptr<VertexShader>);
  void setFragmentShader(std::shared_ptr<FragmentShader>);
  
  static std::shared_ptr<Pipeline> create(){
    return std::make_shared<Pipeline>();
  }
private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_PIPELINE_HPP__
