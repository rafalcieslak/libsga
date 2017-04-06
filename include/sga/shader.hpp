#ifndef __SGA_SHADER_HPP__
#define __SGA_SHADER_HPP__

#include <string>

#include "config.hpp"
#include "layout.hpp"

namespace sga{

class Shader{
public:
  SGA_API ~Shader();
  
  SGA_API void addInput(DataType type, std::string name);
  SGA_API void addInput(std::pair<DataType, std::string>);
  SGA_API void addInput(std::initializer_list<std::pair<DataType, std::string>>);
  SGA_API void addOutput(DataType type, std::string name);
  SGA_API void addOutput(std::pair<DataType, std::string>);
  SGA_API void addOutput(std::initializer_list<std::pair<DataType, std::string>>);

  SGA_API void addUniform(DataType type, std::string name);
  SGA_API void addSampler(std::string name);
  
  friend class Program;
protected:
  SGA_API Shader();
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

class VertexShader : public Shader{
public:
  SGA_API static std::shared_ptr<VertexShader> createFromSource(std::string source);
};

class FragmentShader : public Shader{
public:
  SGA_API static std::shared_ptr<FragmentShader> createFromSource(std::string source);
};

class Program{
public:
  SGA_API ~Program();
  SGA_API void setVertexShader(std::shared_ptr<VertexShader> vs);
  SGA_API void setFragmentShader(std::shared_ptr<FragmentShader> fs);
  
  SGA_API void compile();
  SGA_API void compileFullQuad();

  SGA_API static std::shared_ptr<Program> create(){
    return std::shared_ptr<Program>(new Program());
  }
  
  SGA_API static std::shared_ptr<Program> createAndCompile(std::shared_ptr<VertexShader> vs, std::shared_ptr<FragmentShader> fs){
    auto p = create();
    p->setVertexShader(vs);
    p->setFragmentShader(fs);
    p->compile();
    return p;
  }
  SGA_API static std::shared_ptr<Program> createAndCompile(std::shared_ptr<FragmentShader> fs){
    auto p = create();
    p->setFragmentShader(fs);
    p->compileFullQuad();
    return p;
  }
  
  friend class Pipeline;
  friend class FullQuadPipeline;
protected:
  SGA_API Program();
  
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_SHADER_HPP__
