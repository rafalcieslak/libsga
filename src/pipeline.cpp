#include <sga/pipeline.hpp>
#include "pipeline.impl.hpp"

#include <iostream>
#include <cassert>

#include <vkhlf/vkhlf.h>

#include <sga/exceptions.hpp>
#include "global.hpp"
#include "utils.hpp"
#include "window.impl.hpp"
#include "vbo.impl.hpp"
#include "shader.impl.hpp"
#include "image.impl.hpp"
#include "layout.hpp"

namespace sga{

Pipeline::Pipeline() : impl(std::make_unique<Pipeline::Impl>()) {}
Pipeline::~Pipeline() = default;

void Pipeline::setTarget(std::shared_ptr<Window> target) {impl->setTarget(target);}
void Pipeline::drawVBO(std::shared_ptr<VBO> vbo) {impl->drawVBO(vbo);}
void Pipeline::setClearColor(float r, float g, float b) {impl->setClearColor(r, g, b);}
void Pipeline::setProgram(std::shared_ptr<Program> p) {impl->setProgram(p);}

void Pipeline::setUniform(DataType dt, std::string name, char* pData, size_t size){
  impl->setUniform(dt, name, pData, size);
}

void Pipeline::setSampler(std::string s, std::shared_ptr<Image> i,
                          SamplerInterpolation in, SamplerWarpMode wm){
  impl->setSampler(s,i,in,wm);
}

void Pipeline::setFaceCull(FaceCullMode fcm, FaceDirection fd){
  impl->setFaceCull(fcm,fd);
}

// ====== IMPL ======

Pipeline::Impl::Impl(){
  
}
  
void Pipeline::Impl::setTarget(std::shared_ptr<Window> tgt){
  cooked = false;
  target_is_window = true;
  targetWindow = tgt;
  // TODO: Reset shared ptr to target image
}


void Pipeline::Impl::setClearColor(float r, float g, float b){
  clear_color = {r,g,b};
}

void Pipeline::Impl::setFaceCull(FaceCullMode fcm, FaceDirection fd){
  faceCullMode = fcm;
  faceDirection = fd;
  cooked = false;
}


void Pipeline::Impl::setProgram(std::shared_ptr<Program> p){
  if(p && !p->impl->compiled)
    PipelineConfigError("ProgramNotCompiled", "The program  passed to the pipeline was not compiled.",
                        "The program passed to a pipeline must be compiled first, using Program::compile() method.").raise();
  program = p;
  cooked = false;
  samplers_prepared = descset_prepared = unibuffers_prepared = false;
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
  const auto& off_map = program->impl->c_uniformOffsets;
  auto it = off_map.find(name);
  if(it == off_map.end())
    PipelineConfigError("NoUniform", "Uniform \"" + name + "\" does not exist.").raise();
  if(it->second.second != dt)
    DataFormatError("UniformDataTypeMimatch", "The data type of uniform " + name + " is different than the value written to it.").raise();
  
  size_t offset = it->second.first;
  memcpy(b_uniformArea + offset, pData, size);
}

void Pipeline::Impl::setSampler(std::string name, std::shared_ptr<Image> image, SamplerInterpolation interpolation, SamplerWarpMode warp_mode){
  if(!program)
    PipelineConfigError("NoProgram", "Cannot set uniforms when no program is set.").raise();

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
  
  auto& sdata = it->second;
  sdata.image = image;
  sdata.sampler = global::device->createSampler(
    filter, filter,
    vk::SamplerMipmapMode::eNearest,
    amode, amode, amode,
    0.0f, false, 0.0f, false,
    vk::CompareOp::eNever, 0.0f, 0.0f,
    vk::BorderColor::eFloatOpaqueWhite, false);

  
  std::vector<vkhlf::WriteDescriptorSet> wdss;
  wdss.push_back(vkhlf::WriteDescriptorSet(
                   d_descriptorSet, sdata.bindno, 0, 1,
                   vk::DescriptorType::eCombinedImageSampler,
                   vkhlf::DescriptorImageInfo(sdata.sampler, image->impl->image_view, vk::ImageLayout::eGeneral),
                   nullptr
                   ));
  global::device->updateDescriptorSets(wdss, nullptr);
  std::cout << "Updated!" << std::endl;
}

void Pipeline::Impl::updateStandardUniforms(){
  float time = getTime();
  setUniform(DataType::Float, "sgaTime", (char*)&time, sizeof(time), true);
  
  vk::Extent2D extent = targetWindow->impl->getCurrentFramebuffer().second;
  float e[2] = {(float)extent.width, (float)extent.height};
  setUniform(DataType::Float2, "sgaResolution", (char*)&e, sizeof(e), true);
}

void Pipeline::Impl::drawVBO(std::shared_ptr<VBO> vbo){
  if(!ensureValidity()) return;

  // Extra VBO-specific validity check
  if(vbo->getLayout() != program->impl->c_inputLayout){
    PipelineConfigError("VertexLayoutMismatch", "VBO layout does not match pipeline input layout!").raise();
  }
  
  cook();
  updateStandardUniforms();
  drawBuffer(vbo->impl->buffer, vbo->getDataSize());
}

bool Pipeline::Impl::ensureValidity(){
  if(!program){
    PipelineConfigError("ProgramNotSet", "This pipeline is not ready for rendering, the program was not set.").raise();
  }
  if(!targetWindow){
    PipelineConfigError("RenderTargetMissing", "The pipeline is not ready for rendering, target surface not set.").raise();
  }
  return true;
}

void Pipeline::Impl::drawBuffer(std::shared_ptr<vkhlf::Buffer> buffer, unsigned int n){
  std::shared_ptr<vkhlf::Framebuffer> framebuffer;
  vk::Extent2D extent;
  std::tie(framebuffer, extent) = targetWindow->impl->getCurrentFramebuffer();

  // Ensure all samplers are set.
  for(const auto & s: s_samplers){
    if(!s.second.sampler)
      PipelineConfigError("SamplerNotSet", "This pipeline cannot render, sampler \"" + s.first + "\" was not bound to an image.").raise();
  }
  
  auto cmdBuffer = global::commandPool->allocateCommandBuffer();
  cmdBuffer->begin();
  
  // Update uniform buffers.
  std::shared_ptr<vkhlf::Buffer> unisb = global::device->createBuffer(
    b_uniformSize,
    vk::BufferUsageFlagBits::eTransferSrc,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eHostVisible,
    nullptr);
  auto sbdm = unisb->get<vkhlf::DeviceMemory>();
  void* pMapped = sbdm->map(0, b_uniformSize);
  memcpy(pMapped, b_uniformArea, b_uniformSize);
  sbdm->flush(0, b_uniformSize); sbdm->unmap();

  cmdBuffer->copyBuffer(unisb, b_uniformBuffer, vk::BufferCopy(0, 0, b_uniformSize));
  
  cmdBuffer->beginRenderPass(
    c_renderPass, framebuffer,
    vk::Rect2D({ 0, 0 }, extent),
    { vk::ClearValue(clear_color),
      vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0)) },
    vk::SubpassContents::eInline);
  cmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, c_pipeline);
  cmdBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, c_pipelineLayout, 0, {d_descriptorSet}, nullptr);
  cmdBuffer->bindVertexBuffer(0, buffer, 0);
  cmdBuffer->setViewport(0, vk::Viewport(0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f));
  cmdBuffer->setScissor(0, vk::Rect2D({ 0, 0 }, extent));
  cmdBuffer->draw(uint32_t(n), 1, 0, 0);
  cmdBuffer->endRenderPass();
  cmdBuffer->end();
  
  auto fence = global::device->createFence(false);
  global::queue->submit(
    vkhlf::SubmitInfo{
      {},{}, cmdBuffer, {} },
    fence
    );
  fence->wait(UINT64_MAX);
  
  if(target_is_window){
    targetWindow->impl->currentFrameRendered = true;
   }
}

void Pipeline::Impl::prepare_unibuffers(){
  if(unibuffers_prepared) return;

  b_uniformSize = program->impl->c_uniformSize;
  // Prepare uniform buffers.
  b_uniformBuffer = global::device->createBuffer(
    b_uniformSize,
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eDeviceLocal);
  if(b_uniformArea != nullptr) delete[] b_uniformArea;
  b_uniformArea = new char[b_uniformSize];
  
  unibuffers_prepared = true;
}

void Pipeline::Impl::prepare_samplers(){
  if(samplers_prepared) return;
  for(const auto& s : program->impl->c_samplerBindings){
    s_samplers[s.first] = SamplerData(s.second);
  }
  samplers_prepared = true;
}
void Pipeline::Impl::prepare_descset(){
  if(descset_prepared) return;

  prepare_unibuffers();

  unsigned int samplerno = program->impl->c_samplerBindings.size();

  
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
                   vkhlf::DescriptorBufferInfo(b_uniformBuffer, 0, b_uniformSize)));
    global::device->updateDescriptorSets(wdss, nullptr);
  
  descset_prepared = true;
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
      c_renderPass = targetWindow->impl->renderPass;
    }else{
      std::cout << "UNIMPLMENTED [1]" << std::endl;
    }
    
    // init pipeline
    std::shared_ptr<vkhlf::PipelineCache> pipelineCache = global::device->createPipelineCache(0, nullptr);

    // Use shaders
    vkhlf::PipelineShaderStageCreateInfo vertexStage(
      vk::ShaderStageFlagBits::eVertex,   program->impl->c_VS_shader, "main");
    vkhlf::PipelineShaderStageCreateInfo fragmentStage(
      vk::ShaderStageFlagBits::eFragment, program->impl->c_FS_shader, "main");

    // Prepare input bindings according to vertexInputLayout.
    static std::map<DataType, vk::Format> dataTypeLayout = {
      {DataType::Int,  vk::Format::eR32G32B32A32Sint},
      {DataType::UInt,  vk::Format::eR32G32B32A32Uint},
      {DataType::Float,  vk::Format::eR32Sfloat},
      {DataType::Float2, vk::Format::eR32G32Sfloat},
      {DataType::Float3, vk::Format::eR32G32B32Sfloat},
      {DataType::Float4, vk::Format::eR32G32B32A32Sfloat},
      {DataType::Double,  vk::Format::eR64Sfloat},
    };
    std::vector<vk::VertexInputAttributeDescription> attribs;
    size_t offset = 0, n = 0;
    for(DataType dt : program->impl->c_inputLayout.layout){
      attribs.push_back(vk::VertexInputAttributeDescription(
                          n, 0, dataTypeLayout[dt], offset));
      n++;
      offset += getDataTypeSize(dt);
    }
    vk::VertexInputBindingDescription binding(0, offset, vk::VertexInputRate::eVertex);
    vkhlf::PipelineVertexInputStateCreateInfo vertexInput(binding, attribs);
    
    
    vk::PipelineInputAssemblyStateCreateInfo assembly(
      {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);
    vkhlf::PipelineViewportStateCreateInfo viewport(
      { {} }, { {} });   // one dummy viewport and scissor, as dynamic state sets them
    vk::PipelineRasterizationStateCreateInfo rasterization(
      {}, true, false,
      vk::PolygonMode::eFill,
      [=]{ switch(faceCullMode){
        case FaceCullMode::Back:  return vk::CullModeFlagBits::eBack;
        case FaceCullMode::Front: return vk::CullModeFlagBits::eFront;
        case FaceCullMode::None:  return vk::CullModeFlagBits::eNone;
        }}(),
      [=]{ switch(faceDirection){
        case FaceDirection::Clockwise:        return vk::FrontFace::eClockwise;
        case FaceDirection::CounterClockwise: return vk::FrontFace::eCounterClockwise;
        }}(),
      false, 0.0f, 0.0f, 0.0f, 1.0f);
    vkhlf::PipelineMultisampleStateCreateInfo multisample(
      vk::SampleCountFlagBits::e1, false, 0.0f, nullptr, false, false);
    vk::StencilOpState stencilOpState(
      vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways, 0, 0, 0);
    vk::PipelineDepthStencilStateCreateInfo depthStencil(
      {}, true, true, vk::CompareOp::eLessOrEqual, false, false, stencilOpState, stencilOpState, 0.0f, 0.0f);
    vk::PipelineColorBlendAttachmentState colorBlendAttachment(
      false,
      vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
      vk::BlendFactor::eZero, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    vkhlf::PipelineColorBlendStateCreateInfo colorBlend(
      false, vk::LogicOp::eNoOp, colorBlendAttachment,
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

} // namespace sga
