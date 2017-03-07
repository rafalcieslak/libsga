#ifndef __PIPELINE_IMPL_HPP__
#define __PIPELINE_IMPL_HPP__

#include <sga/pipeline.hpp>

#include <vkhlf/vkhlf.h>

#include <sga/vbo.hpp>

namespace sga{

class Pipeline::Impl{
public:
  Impl();
  void setTarget(std::shared_ptr<Window> tgt);
  void drawTestTriangle();
  void drawVBO(std::shared_ptr<VBOBase>);
  void drawBuffer(std::shared_ptr<vkhlf::Buffer>, unsigned int n);
  void setClearColor(float r, float g, float b);

  void cook();
private:
  bool cooked = false;
  bool target_is_window;
  std::shared_ptr<Window> targetWindow;

  DataLayout vertexInputLayout = {DataType::Float2, DataType::UByte4};
  
  // These fields require cooking
  std::shared_ptr<vkhlf::Pipeline> c_pipeline;
  std::shared_ptr<vkhlf::RenderPass> c_renderPass;

std::array<float, 4> clear_color = {{ 0.0f, 0.0f, 0.0f}};
};

} // namespace sga

#endif // __PIPELINE_IMPL_HPP__
