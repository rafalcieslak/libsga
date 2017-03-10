#ifndef __PIPELINE_IMPL_HPP__
#define __PIPELINE_IMPL_HPP__

#include <sga/pipeline.hpp>

#include <vkhlf/vkhlf.h>

#include <sga/vbo.hpp>
#include <sga/shader.hpp>

namespace sga{

class Pipeline::Impl{
public:
  Impl();
  void setTarget(std::shared_ptr<Window> tgt);
  void drawVBO(std::shared_ptr<VBO>);
  void drawBuffer(std::shared_ptr<vkhlf::Buffer>, unsigned int n);
  void setClearColor(float r, float g, float b);

  void setVertexShader(std::shared_ptr<VertexShader>);
  void setFragmentShader(std::shared_ptr<FragmentShader>);

  void setUniform(DataType dt, std::string name, char* pData, size_t size, bool standard=false);
  void updateStandardUniforms();
  
  bool ensureValidity();
private:
  bool target_is_window;
  std::shared_ptr<Window> targetWindow;

  DataLayout vsInputLayout;
  std::shared_ptr<vkhlf::ShaderModule> vertexShader;
  std::vector<std::pair<DataType, std::string>> vsUniforms;
  DataLayout vsOutputLayout;

  DataLayout fsInputLayout;
  std::shared_ptr<vkhlf::ShaderModule> fragmentShader;
  std::vector<std::pair<DataType, std::string>> fsUniforms;
  DataLayout fsOutputLayout;
  
  void cook();
  bool cooked = false;
  // These fields require cooking
  std::shared_ptr<vkhlf::Pipeline> c_pipeline;
  std::shared_ptr<vkhlf::RenderPass> c_renderPass;
  std::shared_ptr<vkhlf::PipelineLayout> c_pipelineLayout;
  std::shared_ptr<vkhlf::DescriptorSet> c_descriptorSet;
  
  std::shared_ptr<vkhlf::Buffer> c_vsUniformBuffer;
  std::shared_ptr<vkhlf::Buffer> c_fsUniformBuffer;
  size_t c_vsUniformSize;
  size_t c_fsUniformSize;
  char* c_vsUniformArea = nullptr;
  char* c_fsUniformArea = nullptr;

std::array<float, 4> clear_color = {{ 0.0f, 0.0f, 0.0f}};
};

} // namespace sga

#endif // __PIPELINE_IMPL_HPP__
