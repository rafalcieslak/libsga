#include <sga/pipeline.hpp>
#include "pipeline.impl.hpp"

#include <iostream>
#include <cassert>
#include <cmath>

#include <vkhlf/vkhlf.h>

#include <sga/exceptions.hpp>
#include "global.hpp"
#include "utils.hpp"
#include "window.impl.hpp"
#include "vbo.impl.hpp"
#include "shader.impl.hpp"
#include "image.impl.hpp"
#include "layout.hpp"
#include "scheduler.hpp"

namespace sga{

/// ====== GLUE ======

Pipeline::Pipeline() : impl_(std::make_shared<Pipeline::Impl>()) {}
Pipeline::Pipeline(pimpl_unique_ptr<Impl> &&impl) : impl_(std::move(impl)) {}
Pipeline::~Pipeline() = default;

void Pipeline::setTarget(const Window& target) {impl()->setTarget(target);}
void Pipeline::setTarget(std::vector<Image> images) {impl()->setTarget(images);}
void Pipeline::drawVBO(const VBO& vbo) {impl()->drawVBO(vbo);}
void Pipeline::clear() {impl()->clear();}
void Pipeline::setProgram(const Program& p) {impl()->setProgram(p);}

void Pipeline::setUniform(std::string name, std::initializer_list<float> floats){
  impl()->setUniform(name, floats);
}
void Pipeline::setUniform(DataType dt, std::string name, char* pData, size_t size){
  impl()->setUniform(dt, name, pData, size);
}

void Pipeline::setSampler(std::string s, const Image& i,
                          SamplerInterpolation in, SamplerWarpMode wm){
  impl()->setSampler(s,i,in,wm);
}

void Pipeline::setFaceCull(FaceCullMode fcm, FaceDirection fd){
  impl()->setFaceCull(fcm,fd);
}
void Pipeline::setRasterizerMode(sga::RasterizerMode r){impl()->setRasterizerMode(r);}
void Pipeline::setPolygonMode(sga::PolygonMode p){impl()->setPolygonMode(p);}
void Pipeline::setLineWidth(float w){impl()->setLineWidth(w);}

void Pipeline::resetViewport(){ impl()->resetViewport(); }
void Pipeline::setViewport(float left, float top, float right, float bottom){ impl()->setViewport(left, top, right, bottom); }

FullQuadPipeline::FullQuadPipeline() :
  Pipeline(std::make_unique<FullQuadPipeline::Impl>()){
}
FullQuadPipeline::~FullQuadPipeline() = default;

FullQuadPipeline::Impl* FullQuadPipeline::impl(){
  return dynamic_cast<FullQuadPipeline::Impl*>(Pipeline::impl());
}

void FullQuadPipeline::setProgram(const Program& p) {impl()->setProgram(p);}
void FullQuadPipeline::drawFullQuad() {impl()->drawFullQuad();}


// ====== IMPL ======

Pipeline::Impl::Impl(){
}

Pipeline::Impl::~Impl(){
}

void Pipeline::Impl::setTarget(const Window& tgt){
  cooked = false;
  target_is_window = true;
  targetWindow = tgt.impl;
  // Drop references to image targets
  targetImages = std::vector<std::shared_ptr<Image::Impl>>();
  rp_renderpass = nullptr;
  rp_framebuffer = nullptr;
  renderpass_prepared = false;

  resetViewport();
}

void Pipeline::Impl::setTarget(std::vector<Image> images){
  targetImages.clear();
  for(const Image& i : images){
    const auto image = i.impl;
    // Ensure images are not used for sampling.
    for(const auto& sp : s_samplers){
      if(sp.second.image == image){
        PipelineConfigError("InvalidTargetImageUsage", "An image cannot be both render target and sampler source in the same pipeline.");
      }
    }
    targetImages.push_back(image);
  }

  cooked = false;
  target_is_window = false;
  renderpass_prepared = false;
  targetWindow = nullptr;

  resetViewport();
}

void Pipeline::Impl::setFaceCull(FaceCullMode fcm, FaceDirection fd){
  faceCullMode = fcm;
  faceDirection = fd;
  cooked = false;
}

void Pipeline::Impl::setPolygonMode(PolygonMode p) {
  polygonMode = p;
  cooked = false;
}
void Pipeline::Impl::setRasterizerMode(RasterizerMode r) {
  rasterizerMode = r;
  cooked = false;
}
void Pipeline::Impl::setLineWidth(float w){
  // TODO: Validate whether w makes sense.
  line_width = w;
  cooked = false;
}

void Pipeline::Impl::resetViewport(){
  vp_set = false;
}

void Pipeline::Impl::setViewport(float left, float top, float right, float bottom){
  vp_left = left;
  vp_top = top;
  vp_right = right;
  vp_bottom = bottom;
  vp_set = true;
}

void Pipeline::Impl::prepareVp(){
  if(!vp_set){
    if(target_is_window && targetWindow){
      auto extent = targetWindow->getCurrentFramebuffer().second;
      vp_right = extent.width;
      vp_bottom = extent.height;
    }else if(!target_is_window && targetImages[0]){
      vp_right = targetImages[0]->getWidth();
      vp_bottom = targetImages[0]->getHeight();
    }
  }
}

void Pipeline::Impl::setProgram(const Program& p_){
  auto p = p_.impl;
  if(p && !p->compiled)
    PipelineConfigError("ProgramNotCompiled", "The program passed to the pipeline was not compiled.",
                        "The program passed to a pipeline must be compiled first, using Program::compile() method.").raise();
  if(p->isFullQuad)
    PipelineConfigError("InvalidProgramType", "This pipeline does not support full quad programs.").raise();

  program = p;
  cooked = false;
  samplers_prepared = descset_prepared = unibuffers_prepared = false;
}

void Pipeline::Impl::setUniform(std::string name, std::initializer_list<float> floats){
  if(floats.size() == 1){
    setUniform(DataType::Float, name, (char*)floats.begin(), 1*4);
  }else if(floats.size() == 2){
    setUniform(DataType::Float2, name, (char*)floats.begin(), 2*4);
  }else if(floats.size() == 3){
    setUniform(DataType::Float3, name, (char*)floats.begin(), 3*4);
  }else if(floats.size() == 4){
    setUniform(DataType::Float4, name, (char*)floats.begin(), 4*4);
  }else if(floats.size() == 9){
    setUniform(DataType::Mat3, name, (char*)floats.begin(), 9*4);
  }else if(floats.size() == 16){
    setUniform(DataType::Mat4, name, (char*)floats.begin(), 16*4);
  }else{
    DataFormatError("SetUniformInitializerList", "Unable to automatically deduce data type from an initializer list used for setUniform, please use a more specific type (like std::array, glm::vec etc.)").raise();
  }
}

void Pipeline::Impl::setUniform(DataType dt, std::string name, char* pData, size_t size, bool standard){
  if(size != getDataTypeSize(dt)){
    // TODO: Name types in output?
    DataFormatError("UniformSizeMismatch", "setUniform failed: Provided input has different size than declared data type!").raise();
  }

  if(!standard && name.substr(0,3) == "sga")
    PipelineConfigError("SpecialUniform", "Cannot set the value of a standard uniform.", "Uniforms with names beginning with `sga` have a special meaning, and you cannot manually assign values to them.").raise();

  if(!program)
    PipelineConfigError("NoProgram", "Cannot set uniforms when no program is set.").raise();

  prepare_unibuffers();

  // Lookup offset.
  const auto& off_map = program->c_uniformOffsets;
  auto it = off_map.find(name);
  if(it == off_map.end())
    PipelineConfigError("NoUniform", "Uniform \"" + name + "\" does not exist.").raise();
  if(it->second.second != dt)
    DataFormatError("UniformDataTypeMimatch", "The data type of uniform " + name + " is different than the value written to it.").raise();

  size_t offset = it->second.first;
  memcpy(b_uniformHostBuffer + offset, pData, size);

  // Mark the uniform as set.
  uniformsSet.insert(name);
}

void Pipeline::Impl::setSampler(std::string name, const Image& image_ref, SamplerInterpolation interpolation, SamplerWarpMode warp_mode){
  if(!program)
    PipelineConfigError("NoProgram", "Cannot set uniforms when no program is set.").raise();
  std::shared_ptr<Image::Impl> image = image_ref.impl;


  // Ensure image is not a render target.
  for(const auto& i : targetImages){
    if(image == i){
      PipelineConfigError("InvalidSamplerImageUsage", "An image cannot be both sampler source and render target in the same pipeline.");
    }
  }

  prepare_samplers();
  prepare_descset();

  auto it = s_samplers.find(name);
  if(it == s_samplers.end())
    PipelineConfigError("NoSampler", "Sampler \"" + name + "\" does not exist.").raise();

  vk::Filter filter;
  switch(interpolation){
  case SamplerInterpolation::Nearest: filter = vk::Filter::eNearest;  break;
  case SamplerInterpolation::Linear:  filter = vk::Filter::eLinear;   break;
  }
  vk::SamplerAddressMode amode;
  switch(warp_mode){
  case SamplerWarpMode::Clamp:  amode = vk::SamplerAddressMode::eClampToEdge;    break;
  case SamplerWarpMode::Repeat: amode = vk::SamplerAddressMode::eRepeat;         break;
  case SamplerWarpMode::Mirror: amode = vk::SamplerAddressMode::eMirroredRepeat; break;
  }

  float minLod = 0.0f, maxLod = 0.0f;
  float max_anisotropy = 0.0f;
  bool enable_anisotropy = false;

  switch(image->filtermode){
  case ImageFilterMode::None:
  default:
    break;
  case ImageFilterMode::Anisotropic:
    // TODO: Store device limits somewhere globally.
    max_anisotropy = global::physicalDevice->getProperties().limits.maxSamplerAnisotropy;
    enable_anisotropy = true;
    /* FALLTHROGH */
  case ImageFilterMode::MipMapped:
    maxLod = image->getDesiredMipsNo();
    break;
  }

  auto& sdata = it->second;
  sdata.image = image;
  sdata.sampler = global::device->createSampler(
    filter, filter,
    vk::SamplerMipmapMode::eLinear,
    amode, amode, amode,
    0.0f, enable_anisotropy, max_anisotropy, false,
    vk::CompareOp::eNever, minLod, maxLod,
    vk::BorderColor::eFloatOpaqueWhite, false);


  std::vector<vkhlf::WriteDescriptorSet> wdss;
  wdss.push_back(vkhlf::WriteDescriptorSet(
                   d_descriptorSet, sdata.bindno, 0, 1,
                   vk::DescriptorType::eCombinedImageSampler,
                   vkhlf::DescriptorImageInfo(sdata.sampler, image->image_view, vk::ImageLayout::eGeneral),
                   nullptr
                   ));
  global::device->updateDescriptorSets(wdss, nullptr);
}

void Pipeline::Impl::updateStandardUniforms(){
  float time = getTime();
  setUniform(DataType::Float, "sgaTime", (char*)&time, sizeof(time), true);

  vk::Extent2D extent;
  if(target_is_window){
    extent = targetWindow->getCurrentFramebuffer().second;
  }else{
    prepare_renderpass();
    extent = rp_image_target_extent;
  }
  float e[2] = {(float)extent.width, (float)extent.height};
  setUniform(DataType::Float2, "sgaResolution", (char*)&e, sizeof(e), true);

  prepareVp();
  float vp[4] = {vp_left, vp_top, vp_right-vp_left, vp_bottom-vp_top};
  setUniform(DataType::Float4, "sgaViewport", (char*)&vp, sizeof(vp), true);
}

void Pipeline::Impl::drawVBO(const VBO& vbo_){
  auto vbo = vbo_.impl;
  if(!ensureValidity()) return;

  // Extra VBO-specific validity check
  if(vbo->layout != program->c_inputLayout){
    PipelineConfigError("VertexLayoutMismatch", "VBO layout does not match pipeline input layout!").raise();
  }

  cook();
  updateStandardUniforms();
  drawBuffer(vbo->buffer, vbo->getSize());
}

bool Pipeline::Impl::ensureValidity(){
  if(!program){
    PipelineConfigError("ProgramNotSet", "This pipeline is not ready for rendering, the program was not set.").raise();
  }
  if(!targetWindow && targetImages.size() == 0){
    PipelineConfigError("RenderTargetMissing", "The pipeline is not ready for rendering, target surface not set.").raise();
  }
  // Check PX output layout with target.
  DataLayout outl = program->c_outputLayout;
  if(target_is_window){
    if(outl.layout.size() != 1){
      PipelineConfigError("OutputLayoutMismatch", "This pipeline is configured to use a window as the output, so it should use only one color output, but the pixel shader provides " + std::to_string(outl.layout.size())+ " outputs.").raise();
    }
    if(outl.layout[0] != sga::DataType::Float4){
      PipelineConfigError("OutputLayoutMismatch", "Outputs from a pixel shader when rendering onto a window must be of Float4 type.").raise();
    }
  }else{
    // Texture target output.
    if(outl.layout.size() != targetImages.size()){
      PipelineConfigError("OutputLayoutMismatch", "This pipeline is configured to use " + std::to_string(targetImages.size()) + " textures as the output, but the pixel shader provides " + std::to_string(outl.layout.size())+ " outputs.").raise();
    }
    for(unsigned int i = 0; i < outl.layout.size(); i++){
      if(outl.layout[i] != targetImages[i]->format.shaderDataType){
        // TODO: Present type names to explain the error.
        PipelineConfigError("OutputLayoutMismatch", "Shader output "  + std::to_string(i) + "type does match target image format.").raise();
    }
    }
  }
  return true;
}

void Pipeline::Impl::drawBuffer(std::shared_ptr<vkhlf::Buffer> buffer, unsigned int n){
  std::shared_ptr<vkhlf::Framebuffer> framebuffer;
  vk::Extent2D extent;
  if(target_is_window){
    std::tie(framebuffer, extent) = targetWindow->getCurrentFramebuffer();
  }else{
    prepare_renderpass();
    framebuffer = rp_framebuffer;
    extent = rp_image_target_extent;
  }

  // Configure layout for target images.
  for(const auto& i : targetImages){
    i->switchLayout(vk::ImageLayout::eColorAttachmentOptimal);
  }

  // Ensure all samplers are set and configure their layout
  for(const auto & s: s_samplers){
    if(!s.second.sampler)
      PipelineConfigError("SamplerNotSet", "This pipeline cannot render, sampler \"" + s.first + "\" was not bound to an image.").raise();
    s.second.image->switchLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
  }

  // Ensure all uniforms are set
  for(const auto& u : program->c_uniformOffsets){
    if(uniformsSet.find(u.first) == uniformsSet.end()){
      PipelineConfigError("UniformNotSet", "This pipeline cannot render, uniform \"" + u.first + "\" was not set.").raise();
    }
  }

  // Prepare a new staging buffer.
  std::shared_ptr<vkhlf::Buffer> uniform_staging_buffer = global::device->createBuffer(
    b_uniformSize,
    vk::BufferUsageFlagBits::eTransferSrc,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eHostVisible,
    nullptr);
  // Fill it with data for current uniform state
  auto sbdm = uniform_staging_buffer->get<vkhlf::DeviceMemory>();
  void* pMapped = sbdm->map(0, b_uniformSize);
  memcpy(pMapped, b_uniformHostBuffer, b_uniformSize);
  sbdm->flush(0, b_uniformSize); sbdm->unmap();

  // Have scheduler keep a reference to the buffer so that it doesn't get
  // destroyed when this function ends (the copy may be performed much later).
  Scheduler::appendChainedResource(uniform_staging_buffer);

  prepareVp();
  vk::Rect2D area({(int)floor(vp_left), (int)floor(vp_top)},
                  {(unsigned int)std::ceil(vp_right - vp_left), (unsigned int)std::ceil(vp_bottom - vp_top)});
  vk::Viewport viewport(vp_left, vp_top, vp_right, vp_bottom, 0.0f, 1.0f);
  
  Scheduler::borrowChainableCmdBuffer("pipeline draw", [&](auto cmdBuffer){

      cmdBuffer->beginRenderPass(c_renderPass, framebuffer, area, {}, vk::SubpassContents::eInline);

      cmdBuffer->copyBuffer(uniform_staging_buffer, b_uniformDeviceBuffer, vk::BufferCopy(0, 0, b_uniformSize));

      cmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, c_pipeline);
      cmdBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, c_pipelineLayout, 0, {d_descriptorSet}, nullptr);
      cmdBuffer->setViewport(0, viewport);
      cmdBuffer->setScissor(0, area);
      
      cmdBuffer->bindVertexBuffer(0, buffer, 0);
      cmdBuffer->draw(uint32_t(n), 1, 0, 0);

      cmdBuffer->endRenderPass();
    });

  if(target_is_window){
    targetWindow->currentFrameRendered = true;
  }else{
    // Recalculate mipmaps in each target image.
    for(auto img : targetImages){
      img->regenerateMips();
    }
  }
}

void Pipeline::Impl::clear(){
  if(!ensureValidity()) return;
  
  if(target_is_window){
    targetWindow->currentFrameRendered = true;
    targetWindow->clearCurrentFrame();
  }else{
    for(const auto& i : targetImages){
      i->clear();
    }
    // No need to clear the depth buffer if it wasn't craeted yet.
    if(renderpass_prepared)
      clearDepthImage();
  }
}

void Pipeline::Impl::prepare_unibuffers(){
  if(unibuffers_prepared) return;

  b_uniformSize = program->c_uniformSize;
  // Prepare uniform buffers.
  b_uniformDeviceBuffer = global::device->createBuffer(
    b_uniformSize,
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eDeviceLocal);
  if(b_uniformHostBuffer != nullptr) delete[] b_uniformHostBuffer;
  b_uniformHostBuffer = new char[b_uniformSize];

  unibuffers_prepared = true;
}

void Pipeline::Impl::prepare_samplers(){
  if(samplers_prepared) return;
  for(const auto& s : program->c_samplerBindings){
    s_samplers[s.first] = SamplerData(s.second);
  }
  samplers_prepared = true;
}
void Pipeline::Impl::prepare_descset(){
  if(descset_prepared) return;

  prepare_unibuffers();

  unsigned int samplerno = program->c_samplerBindings.size();


  // Descriptor bindings
  std::vector<vkhlf::DescriptorSetLayoutBinding> dslbs;
  dslbs.push_back(vkhlf::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr));
  for(unsigned int i = 0; i < samplerno; i++)
    dslbs.push_back(vkhlf::DescriptorSetLayoutBinding(1 + i, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr));
  // Descriptor set layout
  d_descriptorSetLayout = global::device->createDescriptorSetLayout(dslbs);

  // Set up descriptor sets
  std::shared_ptr<vkhlf::DescriptorPool> descriptorPool = global::device->createDescriptorPool(
    {}, 1 + samplerno,
    {{vk::DescriptorType::eUniformBuffer, 1},
        {vk::DescriptorType::eCombinedImageSampler, samplerno}});

  d_descriptorSet = global::device->allocateDescriptorSet(descriptorPool, d_descriptorSetLayout);

  std::vector<vkhlf::WriteDescriptorSet> wdss;
  wdss.push_back(vkhlf::WriteDescriptorSet(
                   d_descriptorSet, 0, 0, 1,
                   vk::DescriptorType::eUniformBuffer, nullptr,
                   vkhlf::DescriptorBufferInfo(b_uniformDeviceBuffer, 0, b_uniformSize)));
    global::device->updateDescriptorSets(wdss, nullptr);

  descset_prepared = true;
}

void Pipeline::Impl::prepare_renderpass(){
  if(target_is_window || renderpass_prepared) return;

  out_dbg("Preparing pipeline renderpass.");

  // Ensure all target images use the same extent.
  // Assume there is at least one image in target.
  unsigned int width = targetImages[0]->getWidth();
  unsigned int height = targetImages[0]->getHeight();
  for(const auto& i: targetImages){
    if(i->getWidth() != width || i->getHeight() != height)
      PipelineConfigError("TargetImageSizeMismatch", "All target images must share identical dimensions.").raise();
  }
  rp_image_target_extent = vk::Extent2D(width, height);


  // Gather color attachment references.
  std::vector<vk::AttachmentReference> colorReferences;
  unsigned int n = 0;
  for(const auto& i : targetImages){
    (void)i;
    colorReferences.push_back(vk::AttachmentReference(n, vk::ImageLayout::eColorAttachmentOptimal));
    n++;
  }
  n = targetImages.size();
  vk::AttachmentReference depthReference(n, vk::ImageLayout::eDepthStencilAttachmentOptimal);

  // Gather attachment descriptions.
  std::vector<vk::AttachmentDescription> attachmentDescriptions;
  for(const auto& i : targetImages){
    attachmentDescriptions.push_back(vk::AttachmentDescription(
                                       {}, i->format.vkFormat, vk::SampleCountFlagBits::e1,
                                       vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, // color
                                       vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil
                                       vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal
                                       ));
  }
  attachmentDescriptions.push_back(vk::AttachmentDescription(
                                     {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1,
                                     vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore, // depth
                                     vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, // stencil
                                     vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
                                     ));

  vk::SubpassDescription subpassDesc(
    {}, vk::PipelineBindPoint::eGraphics, 0, nullptr,
    colorReferences.size(), colorReferences.data(),
    nullptr,
    &depthReference,
    0, nullptr
    );

  // Prepare renderpass
  rp_renderpass = global::device->createRenderPass(attachmentDescriptions, subpassDesc, nullptr);

  // Prepare imageviews for targets
  std::vector<std::shared_ptr<vkhlf::ImageView>> iviews;
  for(const auto& i : targetImages){
    // TODO: Consider image format
    auto iv = i->image->createImageView(vk::ImageViewType::e2D, i->format.vkFormat,
                                            { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                              vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
                                            { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
    iviews.push_back(iv);
  }
  // Prepare imageviews for depth
  if(depthTarget){
    //rp_depthtarget = depthTarget;
  }else{
    rp_depthimage = global::device->createImage(
      vk::ImageCreateFlags(),
      vk::ImageType::e2D,
      vk::Format::eD32Sfloat,
      vk::Extent3D(width, height, 1),
      1,
      1,
      vk::SampleCountFlagBits::e1,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eDepthStencilAttachment,
      vk::SharingMode::eExclusive,
      std::vector<uint32_t>(), // queue family indices
      vk::ImageLayout::ePreinitialized,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      nullptr, nullptr
      );
  }
  auto iv = rp_depthimage->createImageView(vk::ImageViewType::e2D, vk::Format::eD32Sfloat,
                                         { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                                           vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
                                         { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 });
  iviews.push_back(iv);

  clearDepthImage();

  // Prepare framebuffer
  rp_framebuffer = global::device->createFramebuffer(rp_renderpass, iviews, rp_image_target_extent, 1);

  renderpass_prepared = true;
}

void Pipeline::Impl::clearDepthImage(){
  // Clear depth image.
  Scheduler::buildAndSubmitSynced("Clearing depth image", [&](std::shared_ptr<vkhlf::CommandBuffer> cmdBuffer){
      auto image = rp_depthimage;
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eDepth,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
      cmdBuffer->clearDepthStencilImage(image, vk::ImageLayout::eTransferDstOptimal, 1.0f, 0, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eDepth,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    });
}

void Pipeline::Impl::cook(){
    if(cooked) return;

    // Prepare vkPipeline etc.
    out_dbg("Cooking a pipeline.");

    prepare_unibuffers();

    prepare_samplers();

    prepare_descset();

    // pipeline layout
    c_pipelineLayout = global::device->createPipelineLayout(d_descriptorSetLayout, nullptr);

    // Take renderpass and framebuffer
    if(target_is_window){
      c_renderPass = targetWindow->renderPass;
    }else{
      prepare_renderpass();
      c_renderPass = rp_renderpass;
    }

    // init pipeline
    std::shared_ptr<vkhlf::PipelineCache> pipelineCache = global::device->createPipelineCache(0, nullptr);

    // Use shaders
    vkhlf::PipelineShaderStageCreateInfo vertexStage(
      vk::ShaderStageFlagBits::eVertex,   program->c_VS_shader, "main");
    vkhlf::PipelineShaderStageCreateInfo fragmentStage(
      vk::ShaderStageFlagBits::eFragment, program->c_FS_shader, "main");

    // Prepare input bindings according to vertexInputLayout.
    static std::map<DataType, vk::Format> dataTypeLayout = {
      {DataType::SInt,  vk::Format::eR32Sint},
      {DataType::UInt,  vk::Format::eR32Uint},
      {DataType::SInt2,  vk::Format::eR32G32Sint},
      {DataType::UInt2,  vk::Format::eR32G32Uint},
      {DataType::SInt3,  vk::Format::eR32G32B32Sint},
      {DataType::UInt3,  vk::Format::eR32G32B32Uint},
      {DataType::SInt4,  vk::Format::eR32G32B32A32Sint},
      {DataType::UInt4,  vk::Format::eR32G32B32A32Uint},
      {DataType::Float,  vk::Format::eR32Sfloat},
      {DataType::Float2, vk::Format::eR32G32Sfloat},
      {DataType::Float3, vk::Format::eR32G32B32Sfloat},
      {DataType::Float4, vk::Format::eR32G32B32A32Sfloat},
      {DataType::Double,  vk::Format::eR64Sfloat},
    };
    std::vector<vk::VertexInputAttributeDescription> attribs;
    size_t offset = 0, n = 0;
    for(DataType dt : program->c_inputLayout.layout){
      attribs.push_back(vk::VertexInputAttributeDescription(
                          n, 0, dataTypeLayout[dt], offset));
      n++;
      offset += getDataTypeSize(dt);
    }
    vk::VertexInputBindingDescription binding(0, offset, vk::VertexInputRate::eVertex);
    vkhlf::PipelineVertexInputStateCreateInfo vertexInput(binding, attribs);


    vk::PipelineInputAssemblyStateCreateInfo assembly(
      {},
      [=]{ switch(polygonMode){
        case PolygonMode::Points:       return vk::PrimitiveTopology::ePointList;
        case PolygonMode::Lines:        return vk::PrimitiveTopology::eLineList;
        case PolygonMode::LineStrip:    return vk::PrimitiveTopology::eLineStrip;
        case PolygonMode::Triangles:    return vk::PrimitiveTopology::eTriangleList;
        case PolygonMode::TriangleStrip:return vk::PrimitiveTopology::eTriangleStrip;
        case PolygonMode::TriangleFan:  return vk::PrimitiveTopology::eTriangleFan;
        default: return vk::PrimitiveTopology::eTriangleList;
        }}(), VK_FALSE);
    vkhlf::PipelineViewportStateCreateInfo viewport(
      { {} }, { {} });   // one dummy viewport and scissor, as dynamic state sets them
    vk::PipelineRasterizationStateCreateInfo rasterization(
      {}, true, false,
      [=]{ switch(rasterizerMode){
        case RasterizerMode::Filled:  return vk::PolygonMode::eFill;
        case RasterizerMode::Points:  return vk::PolygonMode::ePoint;
        case RasterizerMode::Wireframe:  return vk::PolygonMode::eLine;
        default: return vk::PolygonMode::eFill;
        }}(),
      [=]{ switch(faceCullMode){
        case FaceCullMode::Back:  return vk::CullModeFlagBits::eBack;
        case FaceCullMode::Front: return vk::CullModeFlagBits::eFront;
        case FaceCullMode::None:  return vk::CullModeFlagBits::eNone;
        default: return vk::CullModeFlagBits::eNone;
        }}(),
      [=]{ switch(faceDirection){
        case FaceDirection::Clockwise:        return vk::FrontFace::eClockwise;
        case FaceDirection::CounterClockwise: return vk::FrontFace::eCounterClockwise;
        default: return vk::FrontFace::eClockwise;
        }}(),
      false, 0.0f, 0.0f, 0.0f,
      // TODO: Ensure the device supports ` wideLines`.
      line_width);
    vkhlf::PipelineMultisampleStateCreateInfo multisample(
      vk::SampleCountFlagBits::e1, false, 0.0f, nullptr, false, false);
    vk::StencilOpState stencilOpState(
      vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways, 0, 0, 0);
    vk::PipelineDepthStencilStateCreateInfo depthStencil(
      {}, true, true, vk::CompareOp::eLessOrEqual, false, false, stencilOpState, stencilOpState, 0.0f, 0.0f);

    vk::PipelineColorBlendAttachmentState defaultColorBlendAttachment(
      false,
      vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
      vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
    if(target_is_window){
      colorBlendAttachments.push_back(defaultColorBlendAttachment);
    }else{
      for(unsigned int i = 0; i < targetImages.size(); i++){
        colorBlendAttachments.push_back(defaultColorBlendAttachment);
      }
    }

    vkhlf::PipelineColorBlendStateCreateInfo colorBlend(
      false, vk::LogicOp::eNoOp, colorBlendAttachments,
      { 1.0f, 1.0f, 1.0f, 1.0f });
    vkhlf::PipelineDynamicStateCreateInfo dynamic(
      { vk::DynamicState::eViewport, vk::DynamicState::eScissor });

    c_pipeline = global::device->createGraphicsPipeline(
      pipelineCache,
      {},
      { vertexStage, fragmentStage },
      vertexInput,
      assembly,
      nullptr,
      viewport,
      rasterization,
      multisample,
      depthStencil,
      colorBlend,
      dynamic,
      c_pipelineLayout,
      c_renderPass);

    cooked = true;
}

FullQuadPipeline::Impl::Impl() :
  vbo({sga::DataType::Float2}, 3){
  std::vector<std::array<float,2>> vertices = {{-1,-1},{ 3,-1},{-1, 3}};
  vbo.write(vertices);
}

void FullQuadPipeline::Impl::setProgram(const Program& p_){
  auto p = p_.impl;
  if(p && !p->compiled)
    PipelineConfigError("ProgramNotCompiled", "The program passed to the pipeline was not compiled.",
                        "The program passed to a pipeline must be compiled first, using Program::compile() method.").raise();
  if(!p->isFullQuad)
    PipelineConfigError("InvalidProgramType", "Only full quad programs are supported by this pipeline.").raise();

  program = p;
  cooked = false;
  samplers_prepared = descset_prepared = unibuffers_prepared = false;
}

void FullQuadPipeline::Impl::drawFullQuad(){
  drawVBO(vbo);
}


} // namespace sga
