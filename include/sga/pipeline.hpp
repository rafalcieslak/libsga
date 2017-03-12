#ifndef __SGA_PIPELINE_HPP__
#define __SGA_PIPELINE_HPP__

#include "config.hpp"
#include "window.hpp"
#include "layout.hpp"

namespace sga{

class VBO;
class VertexShader;
class FragmentShader;

class Pipeline{
public:
  ~Pipeline();

  void setTarget(std::shared_ptr<Window> target);
  void drawVBO(std::shared_ptr<VBO>);
  void setClearColor(float r, float g, float b);
  
  void setVertexShader(std::shared_ptr<VertexShader>);
  void setFragmentShader(std::shared_ptr<FragmentShader>);

  void setUniform(std::string name, float value);
  void setUniform(std::string name, int value);
  void setUniform(std::string name, unsigned int value);
  void setUniform(std::string name, std::array<float,2> value);
  void setUniform(std::string name, std::array<float,3> value);
  void setUniform(std::string name, std::array<float,4> value);
  void setUniform(std::string name, double value);

  template <typename T>
  void setUniform(std::string name, T value, DataType dt){
    setUniform(dt, name, (char*)&value, sizeof(value));
  }
  
  static std::shared_ptr<Pipeline> create(){
    return std::shared_ptr<Pipeline>(new Pipeline());
  }
private:
  Pipeline();
  void setUniform(DataType dt, std::string name, char* pData, size_t size);
  
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_PIPELINE_HPP__
