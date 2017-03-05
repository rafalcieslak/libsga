#ifndef __PIPELINE_IMPL_HPP__
#define __PIPELINE_IMPL_HPP__

#include <sga/pipeline.hpp>

#include <vkhlf/vkhlf.h>

namespace sga{

class Pipeline::Impl{
public:
  Impl();
  void setTarget(std::shared_ptr<Window> tgt);
  void drawTestTriangle();
  void setClearColor(float r, float g, float b);

  void cook();
private:
  bool cooked = false;
  bool target_is_window;
  std::shared_ptr<Window> targetWindow;

  // These fields require cooking
  std::shared_ptr<vkhlf::Pipeline> c_pipeline;
  std::shared_ptr<vkhlf::RenderPass> c_renderPass;
  std::shared_ptr<vkhlf::Buffer> c_vertexBuffer;

std::array<float, 4> clear_color = {{ 0.0f, 0.0f, 0.0f}};
};

} // namespace sga

#endif // __PIPELINE_IMPL_HPP__
