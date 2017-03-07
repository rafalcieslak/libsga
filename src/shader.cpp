#include <sga/shader.hpp>
#include "shader.impl.hpp"

#include <iostream>

#include <vkhlf/vkhlf.h>

#include "global.hpp"
#include "utils.hpp"

namespace sga{

Shader::Shader() : impl(std::make_unique<Shader::Impl>()) {}
Shader::~Shader() = default;

Shader::Impl::Impl() {}

std::shared_ptr<VertexShader> VertexShader::createFromSource(std::string source, DataLayout il, DataLayout ol){
  auto p = std::make_shared<VertexShader>();
  // TODO: Write a custom compilation function which will collect errors etc.
  auto module = vkhlf::compileGLSLToSPIRV(vk::ShaderStageFlagBits::eVertex, source);
  p->impl->shader = impl_global::device->createShaderModule(module);
  p->impl->inputLayout = il;
  p->impl->outputLayout = ol;
  return p;
}

std::shared_ptr<FragmentShader> FragmentShader::createFromSource(std::string source, DataLayout il, DataLayout ol){
  auto p = std::make_shared<FragmentShader>();
  // TODO: Write a custom compilation function which will collect errors etc.
  auto module = vkhlf::compileGLSLToSPIRV(vk::ShaderStageFlagBits::eFragment, source);
  p->impl->shader = impl_global::device->createShaderModule(module);
  p->impl->inputLayout = il;
  p->impl->outputLayout = ol;
  return p;
}

} // namespace sga
