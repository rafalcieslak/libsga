#include <sga/shader.hpp>
#include "shader.impl.hpp"

namespace sga {

Shader::Shader() : impl(std::make_shared<Shader::Impl>()) {
  
}
Shader::~Shader() = default;

void Shader::addInput(DataType type, std::string name) {
  impl->addInput(type, name);
}
void Shader::addInput(std::pair<DataType, std::string> pair) {
  impl->addInput(pair);
}
void Shader::addInput(std::initializer_list<std::pair<DataType, std::string>> list) {
  impl->addInput(list);
}
void Shader::addOutput(DataType type, std::string name) {
  impl->addOutput(type, name);
}
void Shader::addOutput(std::pair<DataType, std::string> pair) {
  impl->addOutput(pair);
}
void Shader::addOutput(std::initializer_list<std::pair<DataType, std::string>> list) {
  impl->addOutput(list);
}

void Shader::addUniform(DataType type, std::string name) {
  impl->addUniform(type, name);
}
void Shader::addSampler(std::string name) {
  impl->addSampler(name);
}


Program::Program() : impl(std::make_shared<Program::Impl>()) {
  
}

Program::~Program() = default;

void Program::compile() {
  impl->compile();
}

void Program::compileFullQuad() {
  impl->compileFullQuad();
}

void Program::setVertexShader(VertexShader vs) {
  impl->setVertexShader(vs);
}

void Program::setFragmentShader(FragmentShader fs) {
  impl->setFragmentShader(fs);
}

void VertexShader::setOutputInterpolationMode(std::string name, sga::OutputInterpolationMode mode){
  impl->setOutputInterpolationMode(name, mode);
}

} // namespace sga
