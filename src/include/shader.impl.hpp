#ifndef __SHADER_IMPL_HPP__
#define __SHADER_IMPL_HPP__

#include <sga/shader.hpp>

#include <vkhlf/vkhlf.h>

#include <sga/layout.hpp>

namespace sga{

class Shader::Impl{
public:
  Impl();

  std::shared_ptr<vkhlf::ShaderModule> shader;
  DataLayout inputLayout;
  DataLayout outputLayout;
};

std::vector<uint32_t> compileGLSLToSPIRV(vk::ShaderStageFlagBits stage, std::string const & source);

} // namespace sga

#endif // __SHADER_IMPL_HPP__
