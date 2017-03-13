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
  
  void setProgram(std::shared_ptr<Program>);
  
  void setUniform(DataType dt, std::string name, char* pData, size_t size, bool standard=false);
  void updateStandardUniforms();
  
  bool ensureValidity();
private:
  bool target_is_window;
  std::shared_ptr<Window> targetWindow;

  std::shared_ptr<Program> program;
  
  void cook();
  bool cooked = false;
  // These fields require cooking
  std::shared_ptr<vkhlf::Pipeline> c_pipeline;
  std::shared_ptr<vkhlf::RenderPass> c_renderPass;
  std::shared_ptr<vkhlf::PipelineLayout> c_pipelineLayout;
  std::shared_ptr<vkhlf::DescriptorSet> c_descriptorSet;

  void prepare_unibuffers();
  bool unibuffers_prepared = false;
  std::shared_ptr<vkhlf::Buffer> b_uniformBuffer;
  size_t b_uniformSize;
  char* b_uniformArea = nullptr;

std::array<float, 4> clear_color = {{ 0.0f, 0.0f, 0.0f}};
};

} // namespace sga

#endif // __PIPELINE_IMPL_HPP__
