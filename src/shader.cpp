#include <sga/shader.hpp>
#include "shader.impl.hpp"

#include <iostream>

#include <vkhlf/vkhlf.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <sga/exceptions.hpp>
#include "global.hpp"
#include "layout.hpp"
#include "utils.hpp"

namespace sga{

Shader::Shader() : impl(std::make_unique<Shader::Impl>()) {}
Shader::~Shader() = default;
void Shader::compile() {impl->compile();}

void Shader::addInput(DataType type, std::string name) {impl->addInput(type, name);}
void Shader::addInput(std::pair<DataType, std::string> pair) {impl->addInput(pair);}
void Shader::addInput(std::initializer_list<std::pair<DataType, std::string>> list) {impl->addInput(list);}
void Shader::addOutput(DataType type, std::string name) {impl->addOutput(type, name);}
void Shader::addOutput(std::pair<DataType, std::string> pair) {impl->addOutput(pair);}
void Shader::addOutput(std::initializer_list<std::pair<DataType, std::string>> list) {impl->addInput(list);}

void Shader::addUniform(DataType type, std::string name) {impl->addUniform(type, name);}

Shader::Impl::Impl() {}

std::shared_ptr<VertexShader> VertexShader::createFromSource(std::string source){
  auto p = std::make_shared<VertexShader>();
  p->impl->stage = vk::ShaderStageFlagBits::eVertex;
  p->impl->source = source;
  p->impl->addStandardUniforms();
  return p;
}

std::shared_ptr<FragmentShader> FragmentShader::createFromSource(std::string source){
  auto p = std::make_shared<FragmentShader>();
  p->impl->stage = vk::ShaderStageFlagBits::eFragment;
  p->impl->source = source;
  p->impl->addStandardUniforms();
  return p;
}


void Shader::Impl::addInput(DataType type, std::string name) {
  addInput(std::make_pair(type,name));
}
void Shader::Impl::addInput(std::initializer_list<std::pair<DataType, std::string>> list) {
  for(const auto &p : list)
    addInput(p);
}
void Shader::Impl::addOutput(DataType type, std::string name) {
  addOutput(std::make_pair(type,name));
}
void Shader::Impl::addOutput(std::initializer_list<std::pair<DataType, std::string>> list){
  for(const auto &p : list)
    addOutput(p);
}

void Shader::Impl::addInput(std::pair<DataType, std::string> pair) {
  // TODO: check if name OK (e.g. no redefinition)
  inputAttr.push_back(pair);
  compiled = false;
}
void Shader::Impl::addOutput(std::pair<DataType, std::string> pair) {
  // TODO: check if name OK (e.g. no redefinition)
  outputAttr.push_back(pair);
  compiled = false;
}

void Shader::Impl::addUniform(sga::DataType type, std::string name){
  uniforms.push_back(std::make_pair(type,name));
  compiled = false;
}

void Shader::Impl::addStandardUniforms(){
  addUniform(DataType::Float, "sgaTime");
}

void Shader::Impl::compile(){
  // Prepare preamble.
  std::string preamble = "#version 420\n";

  inputLayout = DataLayout();
  // Gather input attributes.
  for(unsigned int i = 0; i < inputAttr.size(); i++){
    preamble += "layout(location = " + std::to_string(i) + ") in " +
      getDataTypeGLSL(inputAttr[i].first) + " " + inputAttr[i].second + ";\n";
    inputLayout.extend(inputAttr[i].first);
  }
  // Gather output attributes.
  for(unsigned int i = 0; i < outputAttr.size(); i++){
    preamble += "layout(location = " + std::to_string(i) + ") out " +
      getDataTypeGLSL(outputAttr[i].first) + " " + outputAttr[i].second + ";\n";
    outputLayout.extend(outputAttr[i].first);
  }
  // Gather uniforms
  assert(uniforms.size() > 0);
  unsigned int bindno = (stage == vk::ShaderStageFlagBits::eVertex) ? 0 : 1;
  preamble += "layout(binding = "  + std::to_string(bindno) + ") uniform sga_uniforms {\n";
  for(unsigned int i = 0; i < uniforms.size(); i++){
    preamble += "  " + getDataTypeGLSL(uniforms[i].first) + " " + uniforms[i].second + ";\n";
  }
  preamble += "} u;\n";
  
  auto module = compileGLSLToSPIRV(stage, preamble + source);
  shader = global::device->createShaderModule(module);
  compiled = true;
}

// === GLSL compiler interface to glslang. Mostly based on vkhlf. === 

class GLSLToSPIRVCompiler
{
public:
  VKHLF_API GLSLToSPIRVCompiler();
  VKHLF_API ~GLSLToSPIRVCompiler();

  VKHLF_API std::vector<uint32_t> compile(vk::ShaderStageFlagBits stage, std::string const & source) const;

private:
  TBuiltInResource  m_resource;
};

GLSLToSPIRVCompiler::GLSLToSPIRVCompiler()
{
  glslang::InitializeProcess();

  m_resource.maxLights = 32;
  m_resource.maxClipPlanes = 6;
  m_resource.maxTextureUnits = 32;
  m_resource.maxTextureCoords = 32;
  m_resource.maxVertexAttribs = 64;
  m_resource.maxVertexUniformComponents = 4096;
  m_resource.maxVaryingFloats = 64;
  m_resource.maxVertexTextureImageUnits = 32;
  m_resource.maxCombinedTextureImageUnits = 80;
  m_resource.maxTextureImageUnits = 32;
  m_resource.maxFragmentUniformComponents = 4096;
  m_resource.maxDrawBuffers = 32;
  m_resource.maxVertexUniformVectors = 128;
  m_resource.maxVaryingVectors = 8;
  m_resource.maxFragmentUniformVectors = 16;
  m_resource.maxVertexOutputVectors = 16;
  m_resource.maxFragmentInputVectors = 15;
  m_resource.minProgramTexelOffset = -8;
  m_resource.maxProgramTexelOffset = 7;
  m_resource.maxClipDistances = 8;
  m_resource.maxComputeWorkGroupCountX = 65535;
  m_resource.maxComputeWorkGroupCountY = 65535;
  m_resource.maxComputeWorkGroupCountZ = 65535;
  m_resource.maxComputeWorkGroupSizeX = 1024;
  m_resource.maxComputeWorkGroupSizeY = 1024;
  m_resource.maxComputeWorkGroupSizeZ = 64;
  m_resource.maxComputeUniformComponents = 1024;
  m_resource.maxComputeTextureImageUnits = 16;
  m_resource.maxComputeImageUniforms = 8;
  m_resource.maxComputeAtomicCounters = 8;
  m_resource.maxComputeAtomicCounterBuffers = 1;
  m_resource.maxVaryingComponents = 60;
  m_resource.maxVertexOutputComponents = 64;
  m_resource.maxGeometryInputComponents = 64;
  m_resource.maxGeometryOutputComponents = 128;
  m_resource.maxFragmentInputComponents = 128;
  m_resource.maxImageUnits = 8;
  m_resource.maxCombinedImageUnitsAndFragmentOutputs = 8;
  m_resource.maxCombinedShaderOutputResources = 8;
  m_resource.maxImageSamples = 0;
  m_resource.maxVertexImageUniforms = 0;
  m_resource.maxTessControlImageUniforms = 0;
  m_resource.maxTessEvaluationImageUniforms = 0;
  m_resource.maxGeometryImageUniforms = 0;
  m_resource.maxFragmentImageUniforms = 8;
  m_resource.maxCombinedImageUniforms = 8;
  m_resource.maxGeometryTextureImageUnits = 16;
  m_resource.maxGeometryOutputVertices = 256;
  m_resource.maxGeometryTotalOutputComponents = 1024;
  m_resource.maxGeometryUniformComponents = 1024;
  m_resource.maxGeometryVaryingComponents = 64;
  m_resource.maxTessControlInputComponents = 128;
  m_resource.maxTessControlOutputComponents = 128;
  m_resource.maxTessControlTextureImageUnits = 16;
  m_resource.maxTessControlUniformComponents = 1024;
  m_resource.maxTessControlTotalOutputComponents = 4096;
  m_resource.maxTessEvaluationInputComponents = 128;
  m_resource.maxTessEvaluationOutputComponents = 128;
  m_resource.maxTessEvaluationTextureImageUnits = 16;
  m_resource.maxTessEvaluationUniformComponents = 1024;
  m_resource.maxTessPatchComponents = 120;
  m_resource.maxPatchVertices = 32;
  m_resource.maxTessGenLevel = 64;
  m_resource.maxViewports = 16;
  m_resource.maxVertexAtomicCounters = 0;
  m_resource.maxTessControlAtomicCounters = 0;
  m_resource.maxTessEvaluationAtomicCounters = 0;
  m_resource.maxGeometryAtomicCounters = 0;
  m_resource.maxFragmentAtomicCounters = 8;
  m_resource.maxCombinedAtomicCounters = 8;
  m_resource.maxAtomicCounterBindings = 1;
  m_resource.maxVertexAtomicCounterBuffers = 0;
  m_resource.maxTessControlAtomicCounterBuffers = 0;
  m_resource.maxTessEvaluationAtomicCounterBuffers = 0;
  m_resource.maxGeometryAtomicCounterBuffers = 0;
  m_resource.maxFragmentAtomicCounterBuffers = 1;
  m_resource.maxCombinedAtomicCounterBuffers = 1;
  m_resource.maxAtomicCounterBufferSize = 16384;
  m_resource.maxTransformFeedbackBuffers = 4;
  m_resource.maxTransformFeedbackInterleavedComponents = 64;
  m_resource.maxCullDistances = 8;
  m_resource.maxCombinedClipAndCullDistances = 8;
  m_resource.maxSamples = 4;
  m_resource.limits.nonInductiveForLoops = 1;
  m_resource.limits.whileLoops = 1;
  m_resource.limits.doWhileLoops = 1;
  m_resource.limits.generalUniformIndexing = 1;
  m_resource.limits.generalAttributeMatrixVectorIndexing = 1;
  m_resource.limits.generalVaryingIndexing = 1;
  m_resource.limits.generalSamplerIndexing = 1;
  m_resource.limits.generalVariableIndexing = 1;
  m_resource.limits.generalConstantMatrixVectorIndexing = 1;
}

GLSLToSPIRVCompiler::~GLSLToSPIRVCompiler()
{
  glslang::FinalizeProcess();
}

std::vector<uint32_t> GLSLToSPIRVCompiler::compile(vk::ShaderStageFlagBits stage, std::string const & source) const
{
  static const std::map<vk::ShaderStageFlagBits, EShLanguage> stageToLanguageMap
  {
    {vk::ShaderStageFlagBits::eVertex, EShLangVertex},
    {vk::ShaderStageFlagBits::eTessellationControl, EShLangTessControl},
    {vk::ShaderStageFlagBits::eTessellationEvaluation, EShLangTessEvaluation},
    {vk::ShaderStageFlagBits::eGeometry, EShLangGeometry},
    {vk::ShaderStageFlagBits::eFragment, EShLangFragment},
    {vk::ShaderStageFlagBits::eCompute, EShLangCompute}
  };

  std::map<vk::ShaderStageFlagBits, EShLanguage>::const_iterator stageIt = stageToLanguageMap.find(stage);
  assert( stageIt != stageToLanguageMap.end());
  glslang::TShader shader(stageIt->second);

  const char *shaderStrings[1];
  shaderStrings[0] = source.c_str();
  shader.setStrings(shaderStrings, 1);
  shader.setEntryPoint("main");
  
  // Enable SPIR-V and Vulkan rules when parsing GLSL
  EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

  if (!shader.parse(&m_resource, 100, false, messages))
    {
      // TODO: Parse error details?
      std::string infoLog = shader.getInfoLog();
      std::string infoDebugLog = shader.getInfoDebugLog();
      ShaderParsingError(infoLog, infoDebugLog).raise();
    }

  glslang::TProgram program;
  program.addShader(&shader);

  if (!program.link(messages))
    {
      std::string infoLog = shader.getInfoLog();
      std::string infoDebugLog = shader.getInfoDebugLog();
      ShaderLinkingError(infoLog, infoDebugLog).raise();
    }
  
  // TODO: Does this have a return value?
  std::vector<uint32_t> code;
  glslang::GlslangToSpv(*program.getIntermediate(stageIt->second), code);
  
  
  return code;
}

std::vector<uint32_t> compileGLSLToSPIRV(vk::ShaderStageFlagBits stage, std::string const & source)
{
  static GLSLToSPIRVCompiler compiler;
  return compiler.compile(stage, source);
}


} // namespace sga
