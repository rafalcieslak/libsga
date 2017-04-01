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

void Shader::addInput(DataType type, std::string name) {impl->addInput(type, name);}
void Shader::addInput(std::pair<DataType, std::string> pair) {impl->addInput(pair);}
void Shader::addInput(std::initializer_list<std::pair<DataType, std::string>> list) {impl->addInput(list);}
void Shader::addOutput(DataType type, std::string name) {impl->addOutput(type, name);}
void Shader::addOutput(std::pair<DataType, std::string> pair) {impl->addOutput(pair);}
void Shader::addOutput(std::initializer_list<std::pair<DataType, std::string>> list) {impl->addOutput(list);}

void Shader::addUniform(DataType type, std::string name) {impl->addUniform(type, name);}
void Shader::addSampler(std::string name) {impl->addSampler(name);}

Program::Program() : impl(std::make_unique<Program::Impl>()) {}
Program::~Program() = default;
void Program::compile() {impl->compile();}
void Program::compileFullQuad() {impl->compileFullQuad();}
void Program::setVertexShader(std::shared_ptr<VertexShader> vs) {impl->setVertexShader(vs);}
void Program::setFragmentShader(std::shared_ptr<FragmentShader> fs) {impl->setFragmentShader(fs);}

// ======= Shader impl =======

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
}
void Shader::Impl::addOutput(std::pair<DataType, std::string> pair) {
  // TODO: check if name OK (e.g. no redefinition)
  outputAttr.push_back(pair);
}

void Shader::Impl::addUniform(sga::DataType type, std::string name, bool special){
  if(!special && name.substr(0,3) == "sga")
    ProgramConfigError("UniformNameReserved", "Cannot add an uniform using a reserved name", "Uniforms with names beginning with `sga` have a special meaning, and you cannot declare your own").raise();
  if(!isVariableNameValid(name))
    ProgramConfigError("UniformNameInvalid", "Cannot use \"" + name + "\" for the identifier of a uniform, it must be a valid C indentifier.").raise();
  uniforms.push_back(std::make_pair(type,name));
}
void Shader::Impl::addSampler(std::string name){
  if(!isVariableNameValid(name))
    ProgramConfigError("SamplerNameInvalid", "Cannot use \"" + name + "\" for the identifier of a sampler, it must be a valid C indentifier.").raise();
  samplers.push_back(name);
}

void Shader::Impl::addStandardUniforms(){
  addUniform(DataType::Float, "sgaTime", true);
  addUniform(DataType::Float2, "sgaResolution", true);
}

// ====== Program impl ======

Program::Impl::Impl() {}

void Program::Impl::setVertexShader(std::shared_ptr<VertexShader> vs) {
  if(compiled)
    ProgramConfigError("AlreadyCompiled", "This program has already been compiled, you cannot change shaders anymore.").raise();
  if(VS.source != "")
    ProgramConfigError("AlreadyHasVS", "This program already has a vertex shader, you cannot set another.").raise();
  if(!vs || vs->impl->source == "")
    ProgramConfigError("EmptyShader", "The provided shader is empty.").raise();

  VS.source = vs->impl->source;
  VS.inputAttr = vs->impl->inputAttr;
  VS.outputAttr = vs->impl->outputAttr;
  VS.uniforms = vs->impl->uniforms;
  VS.samplers = vs->impl->samplers;
}
void Program::Impl::setFragmentShader(std::shared_ptr<FragmentShader> fs) {
  if(compiled)
    ProgramConfigError("AlreadyCompiled", "This program has already been compiled, you cannot change shaders anymore").raise();
  if(FS.source != "")
    ProgramConfigError("AlreadyHasFS", "This program already has a fragment shader, you cannot set another.").raise();
  if(!fs || fs->impl->source == "")
    ProgramConfigError("EmptyShader", "The provided shader is empty.").raise();
  
  FS.source = fs->impl->source;
  FS.inputAttr = fs->impl->inputAttr;
  FS.outputAttr = fs->impl->outputAttr;
  FS.uniforms = fs->impl->uniforms;
  FS.samplers = fs->impl->samplers;
}

void Program::Impl::compile() {
  if(VS.source == "" || FS.source == "")
    ProgramConfigError("MissingShader", "A program must have both vertex and fragment shaders set before compiling!").raise();

  isFullQuad = false;
  compile_internal();
}

void Program::Impl::compileFullQuad(){  
  out_dbg("Compiling program");
  if(VS.source != "" || FS.source == "")
    ProgramConfigError("MissingShader", "A full quad program must have a fragment shader and NO vertex shader set before compiling!").raise();
  
  auto vertShader = VertexShader::createFromSource(R"(
    void main()
    {
      gl_Position = vec4(inVertex, 0, 1);
    }
  )");
  vertShader->addInput(sga::DataType::Float2, "inVertex");
  setVertexShader(vertShader);
  
  isFullQuad = true;  
  compile_internal();
}
  
void Program::Impl::compile_internal() {

  out_dbg("Compiling program");
  if(VS.source == "" || FS.source == "")
    ProgramConfigError("MissingShader", "A program must have both vertex and fragment shaders set before compiling!").raise();

  if(compiled)
    ProgramConfigError("AlreadyCompiled", "This program has already been compiled.").raise();
  
  // Prepare preamble.
  std::string preamble = "#version 420\n";

  // Gather uniforms.
  std::map<std::string, DataType> uniforms;
  for(const ShaderData& S : {std::ref(FS), std::ref(VS)}){
    for(const auto& p : S.uniforms){
      auto it = uniforms.find(p.second);
      if(it == uniforms.end()){
        uniforms[p.second] = p.first;
      }else{
        if(it->second != p.first)
          PipelineConfigError("UniformMismatch", "There are some uniforms that share name, but not type.");
      }
    }
  }

  // Prepare uniform layout.
  size_t offset = 0;
  std::string uniformCode = "layout(std140, binding = 0) uniform sga_uniforms {\n";
  for(const auto& p : uniforms){
    offset = align(offset, getDataTypeGLSLstd140Alignment(p.second));
    c_uniformOffsets[p.first] = std::make_pair(offset, p.second);
    uniformCode += "  " + getDataTypeGLSLName(p.second) + " " + p.first + ";\n";
    offset += getDataTypeSize(p.second);
  }
  uniformCode += "} u;\n\n";
  c_uniformSize = offset;

  // Prepare samplers.
  std::set<std::string> sampler_names;
  for(const ShaderData& S : {std::ref(FS), std::ref(VS)})
    for(const auto& p : S.samplers)
      sampler_names.insert(p);
  
  // Prepare sampler bindings.
  unsigned int bindno = 1; // binding 0 is used for UBO
  for(const auto& name : sampler_names)
    c_samplerBindings[name] = bindno++;

  // Prepare sampler source code.
  std::string samplerCode;
  for(const auto& p : c_samplerBindings)
    samplerCode += "layout (binding = " + std::to_string(p.second) + ") uniform sampler2D " + p.first + ";\n";
  
  // Prepare attributes and their source code.
  for(ShaderData& S : {std::ref(FS), std::ref(VS)}){
    for(unsigned int i = 0; i < S.inputAttr.size(); i++){
      S.attrCode += "layout(location = " + std::to_string(i) + ") in " +
        getDataTypeGLSLName(S.inputAttr[i].first) + " " + S.inputAttr[i].second + ";\n";
      S.inputLayout.extend(S.inputAttr[i].first);
    }
    for(unsigned int i = 0; i < S.outputAttr.size(); i++){
      S.attrCode += "layout(location = " + std::to_string(i) + ") out " +
        getDataTypeGLSLName(S.outputAttr[i].first) + " " + S.outputAttr[i].second + ";\n";
      S.outputLayout.extend(S.outputAttr[i].first);
    }
  }

  // Verify attributes inferface.
  if(VS.outputLayout != FS.inputLayout)
    ProgramConfigError("ShaderInterfaceMismatch", "Vertex shader output does not match fragment shader input.");
  
  // Emit full sources.
  for(ShaderData& S : {std::ref(FS), std::ref(VS)}){
    S.fullSource = preamble + S.attrCode + uniformCode + samplerCode + S.source;
    out_dbg("=== FULL SHADER SOURCE ===\n" + S.fullSource);
  }
  
  // Extract metadata.
  c_inputLayout = VS.inputLayout;
  c_outputLayout = FS.outputLayout;
  
  // Compile to SPIRV.
  c_VS_shader = global::device->createShaderModule(
    compileGLSLToSPIRV(vk::ShaderStageFlagBits::eVertex, VS.fullSource)
    );
  c_FS_shader = global::device->createShaderModule(
    compileGLSLToSPIRV(vk::ShaderStageFlagBits::eFragment, FS.fullSource)
    );
  
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
      std::string infoLog = program.getInfoLog();
      std::string infoDebugLog = program.getInfoDebugLog();
      ShaderLinkingError(infoLog, infoDebugLog).raise();
    }

  //program.buildReflection();
  //program.dumpReflection();
  
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
