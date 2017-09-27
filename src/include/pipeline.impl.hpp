#ifndef __PIPELINE_IMPL_HPP__
#define __PIPELINE_IMPL_HPP__

#include <sga/pipeline.hpp>

#include <vkhlf/vkhlf.h>

#include <sga/vbo.hpp>
#include <sga/shader.hpp>
#include <sga/image.hpp>

#include <unordered_set>

namespace sga{

class Pipeline::Impl{
public:
  Impl();
  virtual ~Impl();
  void setTarget(const Window& tgt);
  void setTarget(std::vector<Image> images);
  
  void drawVBO(const VBO&);
  void drawBuffer(std::shared_ptr<vkhlf::Buffer>, unsigned int n);
  void clear();
  
  virtual void setProgram(const Program&);

  void setUniform(std::string name, std::initializer_list<float> floats);  
  void setUniform(DataType dt, std::string name, char* pData, size_t size, bool standard=false);
  void updateStandardUniforms();

  void setSampler(std::string, const Image&,
                  SamplerInterpolation intefrpolation = SamplerInterpolation::Linear,
                  SamplerWarpMode warp_mode = SamplerWarpMode::Clamp);
  
  void setFaceCull(FaceCullMode fcm = FaceCullMode::None, FaceDirection fd = FaceDirection::Clockwise);
  void setPolygonMode(PolygonMode p);
  void setRasterizerMode(RasterizerMode r);
  void setLineWidth(float w);

  void resetViewport();
  void setViewport(float left, float top, float right, float bottom);
  
  bool ensureValidity();
protected:
  bool target_is_window;
  std::shared_ptr<Window::Impl> targetWindow;
  std::vector<std::shared_ptr<Image::Impl>> targetImages;
  std::shared_ptr<Image::Impl> depthTarget;

  std::shared_ptr<Program::Impl> program;
  
  FaceCullMode faceCullMode = FaceCullMode::None;
  FaceDirection faceDirection = FaceDirection::Clockwise;
  PolygonMode polygonMode = PolygonMode::Triangles;
  RasterizerMode rasterizerMode = RasterizerMode::Filled;
  float line_width = 1.0f;
  
  bool vp_set = false;
  float vp_top = 0.0f, vp_bottom = 0.0f, vp_left = 0.0f, vp_right = 0.0f;
  // If vp_set is false, this function sets vp_* according to target size.
  void prepareVp(); 
  
  void clearDepthImage();
  
  void cook();
  bool cooked = false;
  // These fields require cooking
  std::shared_ptr<vkhlf::Pipeline> c_pipeline;
  std::shared_ptr<vkhlf::RenderPass> c_renderPass;
  std::shared_ptr<vkhlf::PipelineLayout> c_pipelineLayout;

  void prepare_descset();
  bool descset_prepared = false;
  std::shared_ptr<vkhlf::DescriptorSet> d_descriptorSet;
  std::shared_ptr<vkhlf::DescriptorSetLayout> d_descriptorSetLayout;
  
  void prepare_unibuffers();
  bool unibuffers_prepared = false;
  /* This bufer is permanently present on the device and contains current values
   * (device-time). Draw commands update it by copying from the right clone of a
   * staging buffer. */
  std::shared_ptr<vkhlf::Buffer> b_uniformDeviceBuffer;
  size_t b_uniformSize;
  /* This buffer is in host memory. It is used for building the buffer as values
   * are set with the API (host-time). On draw, the contents are copied to a
   * staging buffer, which keeps this data until the frame is drawn by the
   * device. This means there may simultaneously exist multiple staging buffers
   * with different values, waiting to be used for rendering. */
  char* b_uniformHostBuffer = nullptr;
  /* Stores the names of uniforms that were set at least once. This is used for
     ensuring that the user did not forget to set any uniform. */
  std::unordered_set<std::string> uniformsSet;

  void prepare_samplers();
  bool samplers_prepared = false;
  struct SamplerData{
    SamplerData() {}
    SamplerData(int b) : bindno(b) {}
    int bindno;
    mutable std::shared_ptr<Image::Impl> image;
    mutable std::shared_ptr<vkhlf::Sampler> sampler;
    
    bool operator<(const SamplerData& other) {return bindno < other.bindno;}
  };
  // TODO: This keeps a OWNED reference to Image. This way we are sure the image
  // is never destroyed as long as it is bound to some pipeline. However, how
  // should a pipeline react on image changes (e.g. resizing?);
  std::map<std::string, SamplerData> s_samplers;

  void prepare_renderpass();
  bool renderpass_prepared;
  std::shared_ptr<vkhlf::RenderPass> rp_renderpass;
  std::shared_ptr<vkhlf::Framebuffer> rp_framebuffer;
  std::shared_ptr<vkhlf::Image> rp_depthimage;
  vk::Extent2D rp_image_target_extent;
};

class FullQuadPipeline::Impl : public Pipeline::Impl{
public:
  Impl();
  
  void drawFullQuad();
  
  void setProgram(const Program&) override;

protected:
  VBO vbo;
};

} // namespace sga

#endif // __PIPELINE_IMPL_HPP__
