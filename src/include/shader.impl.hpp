#ifndef __SHADER_IMPL_HPP__
#define __SHADER_IMPL_HPP__

#include <sga/shader.hpp>

#include <vkhlf/vkhlf.h>

#include <sga/layout.hpp>

namespace sga{

class Shader::Impl{
public:
  Impl();

  std::string source;
  vk::ShaderStageFlagBits stage;

  void addInput(DataType type, std::string name);
  void addInput(std::pair<DataType, std::string>);
  void addInput(std::initializer_list<std::pair<DataType, std::string>>);
  void addOutput(DataType type, std::string name);
  void addOutput(std::pair<DataType, std::string>);
  void addOutput(std::initializer_list<std::pair<DataType, std::string>>);
  
  void addUniform(DataType type, std::string name);
  
  std::vector<std::pair<DataType, std::string>> inputAttr;
  std::vector<std::pair<DataType, std::string>> outputAttr;
  std::vector<std::pair<DataType, std::string>> uniforms;

  void addStandardUniforms();
  
  void compile();
  bool compiled = false;
  
  std::shared_ptr<vkhlf::ShaderModule> shader;
  
  DataLayout inputLayout;
  DataLayout outputLayout;
};

std::vector<uint32_t> compileGLSLToSPIRV(vk::ShaderStageFlagBits stage, std::string const & source);

} // namespace sga

#endif // __SHADER_IMPL_HPP__
