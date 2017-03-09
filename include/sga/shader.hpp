#ifndef __SGA_SHADER_HPP__
#define __SGA_SHADER_HPP__

#include <memory>
#include <string>

#include <sga/layout.hpp>

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
  
  void compile();
  
  friend class Pipeline;
protected:
  Shader();
  class Impl;
  std::unique_ptr<Impl> impl;
};

class VertexShader : public Shader{
public:
  static std::shared_ptr<VertexShader> createFromSource(std::string source);
};

class FragmentShader : public Shader{
public:
  static std::shared_ptr<FragmentShader> createFromSource(std::string source);
};


} // namespace sga

#endif // __SGA_SHADER_HPP__
