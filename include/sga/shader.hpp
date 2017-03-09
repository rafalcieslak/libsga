#ifndef __SGA_SHADER_HPP__
#define __SGA_SHADER_HPP__

#include <memory>
#include <string>

#include <sga/layout.hpp>

namespace sga{

class Shader{
public:
  ~Shader();
  void compile();
  friend class Pipeline;
protected:
  Shader();
  class Impl;
  std::unique_ptr<Impl> impl;
};

class VertexShader : public Shader{
public:
  static std::shared_ptr<VertexShader> createFromSource(std::string source, DataLayout il, DataLayout ol);
};

class FragmentShader : public Shader{
public:
  static std::shared_ptr<FragmentShader> createFromSource(std::string source, DataLayout il, DataLayout ol);
};


} // namespace sga

#endif // __SGA_SHADER_HPP__
