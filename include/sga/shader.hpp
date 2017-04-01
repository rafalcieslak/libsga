#ifndef __SGA_SHADER_HPP__
#define __SGA_SHADER_HPP__

#include <string>

#include "config.hpp"
#include "layout.hpp"

namespace sga{

class Shader{
public:
  ~Shader();
  
  void addInput(DataType type, std::string name);
  void addInput(std::pair<DataType, std::string>);
  void addInput(std::initializer_list<std::pair<DataType, std::string>>);
  void addOutput(DataType type, std::string name);
  void addOutput(std::pair<DataType, std::string>);
  void addOutput(std::initializer_list<std::pair<DataType, std::string>>);

  void addUniform(DataType type, std::string name);
  void addSampler(std::string name);
  
  friend class Program;
protected:
  Shader();
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

class VertexShader : public Shader{
public:
  static std::shared_ptr<VertexShader> createFromSource(std::string source);
};

class FragmentShader : public Shader{
public:
  static std::shared_ptr<FragmentShader> createFromSource(std::string source);
};

class Program{
public:
  ~Program();
  void setVertexShader(std::shared_ptr<VertexShader> vs);
  void setFragmentShader(std::shared_ptr<FragmentShader> fs);
  
  void compile();
  void compileFullQuad();

  static std::shared_ptr<Program> create(){
    return std::shared_ptr<Program>(new Program());
  }
  
  static std::shared_ptr<Program> createAndCompile(std::shared_ptr<VertexShader> vs, std::shared_ptr<FragmentShader> fs){
    auto p = create();
    p->setVertexShader(vs);
    p->setFragmentShader(fs);
    p->compile();
    return p;
  }
  static std::shared_ptr<Program> createAndCompile(std::shared_ptr<FragmentShader> fs){
    auto p = create();
    p->setFragmentShader(fs);
    p->compileFullQuad();
    return p;
  }
  
  friend class Pipeline;
  friend class FullQuadPipeline;
protected:
  Program();
  
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

/*
class FragmentOnlyProgram : public Program{
public:
  ~FragmentOnlyProgram();
  
  static std::shared_ptr<FragmentOnlyProgram> create(){
    return std::shared_ptr<FragmentOnlyProgram>(new FragmentOnlyProgram());
  }  
  static std::shared_ptr<Program> createAndCompile(std::shared_ptr<VertexShader> vs, std::shared_ptr<FragmentShader> fs) = delete;
  
  static std::shared_ptr<Program> createAndCompile(std::shared_ptr<FragmentShader> fs) = delete;

  void compile();
  
  friend class FullQuadPipeline;
protected:
  FragmentOnlyProgram();
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};
*/

} // namespace sga

#endif // __SGA_SHADER_HPP__
