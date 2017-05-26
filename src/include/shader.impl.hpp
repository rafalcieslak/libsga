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
  
  void addUniform(DataType type, std::string name, bool special = false);
  void addSampler(std::string name);
  
  std::vector<std::pair<DataType, std::string>> inputAttr;
  std::vector<std::pair<DataType, std::string>> outputAttr;
  std::vector<std::pair<DataType, std::string>> uniforms;
  std::vector<std::string> samplers;

  void addStandardUniforms();
};

class Program::Impl{
public:
  Impl();
  void setVertexShader(VertexShader vs);
  void setFragmentShader(FragmentShader vs);
  
  void compile();
  void compileFullQuad();
  void compile_internal();
  
  friend class Pipeline;
  friend class FullQuadPipeline;
private:
  struct ShaderData{
    std::string autoSource, source, attrCode, fullSource;
    std::vector<std::pair<DataType, std::string>> inputAttr, outputAttr, uniforms;
    std::vector<std::string> samplers;
    DataLayout inputLayout, outputLayout;
  };
  ShaderData VS;
  ShaderData FS;
  
  std::string prepareErrorDescrip(std::string infoLog, const ShaderData& sd) const;

  bool compiled = false;
  bool isFullQuad = false;
  
  // Only valid once compiled.
  DataLayout c_inputLayout;
  DataLayout c_outputLayout;
  std::shared_ptr<vkhlf::ShaderModule> c_VS_shader = nullptr;
  std::shared_ptr<vkhlf::ShaderModule> c_FS_shader = nullptr;

  std::map<std::string, std::pair<size_t, DataType>> c_uniformOffsets;
  size_t c_uniformSize = 0;
  
  std::map<std::string, unsigned int> c_samplerBindings;
};

std::vector<uint32_t> compileGLSLToSPIRV(vk::ShaderStageFlagBits stage, std::string const & source);

} // namespace sga

#endif // __SHADER_IMPL_HPP__
