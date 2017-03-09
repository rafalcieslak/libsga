#include <sga/pipeline.hpp>
#include "pipeline.impl.hpp"

#include <iostream>
#include <cassert>

#include <vkhlf/vkhlf.h>

#include "global.hpp"
#include "utils.hpp"
#include "window.impl.hpp"
#include "vbo.impl.hpp"
#include "shader.impl.hpp"

namespace sga{

Pipeline::Pipeline() : impl(std::make_unique<Pipeline::Impl>()) {}
Pipeline::~Pipeline() = default;


void Pipeline::setTarget(std::shared_ptr<Window> target) {impl->setTarget(target);}
void Pipeline::drawVBO(std::shared_ptr<VBOBase> vbo) {impl->drawVBO(vbo);}
void Pipeline::setClearColor(float r, float g, float b) {impl->setClearColor(r, g, b);}
void Pipeline::setFragmentShader(std::shared_ptr<FragmentShader> shader) {impl->setFragmentShader(shader);}
void Pipeline::setVertexShader(std::shared_ptr<VertexShader> shader) {impl->setVertexShader(shader);}

// ====== IMPL ======

Pipeline::Impl::Impl(){
  
}
  
void Pipeline::Impl::setTarget(std::shared_ptr<Window> tgt){
  cooked = false;
  target_is_window = true;
  targetWindow = tgt;
  // TODO: Reset shared ptr to target image
}

void Pipeline::Impl::setVertexShader(std::shared_ptr<VertexShader> vs){
  cooked = false;
  vertexShader = vs;
}
void Pipeline::Impl::setFragmentShader(std::shared_ptr<FragmentShader> fs){
  cooked = false;
  fragmentShader = fs;
}

void Pipeline::Impl::drawVBO(std::shared_ptr<VBOBase> vbo){
  if(!ensureValidity()) return;

  // Extra VBO-specific validity check
  if(vbo->getLayout() != vertexShader->impl->inputLayout){
    std::cout << "ERROR: VBO layout does not match pipeline input layout!" << std::endl;
    return;
  }
  
  cook();
  drawBuffer(vbo->impl->buffer, vbo->size);
}

bool Pipeline::Impl::ensureValidity(){
  // TODO: More verbose output?
  if(!vertexShader){
    std::cout << "Vertex shader not set." << std::endl; return false;
  }
  if(!fragmentShader){
    std::cout << "Fragment shader not set." << std::endl; return false;
  }
  if(vertexShader->impl->outputLayout != fragmentShader->impl->inputLayout){
    std::cout << "Vertex shader output layout does not match fragment shader input layout" << std::endl; return false;
  }
  if(!targetWindow){
    std::cout << "Target window not set" << std::endl; return false;
  }
  return true;
}

void Pipeline::Impl::drawBuffer(std::shared_ptr<vkhlf::Buffer> buffer, unsigned int n){
  std::shared_ptr<vkhlf::Framebuffer> framebuffer;
  vk::Extent2D extent;
  std::tie(framebuffer, extent) = targetWindow->impl->getCurrentFramebuffer();
  
  
  auto cmdBuffer = global::commandPool->allocateCommandBuffer();
  cmdBuffer->begin();
  cmdBuffer->beginRenderPass(c_renderPass, framebuffer,
                             vk::Rect2D({ 0, 0 }, extent),
                             { vk::ClearValue(clear_color), vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0)) },
                             vk::SubpassContents::eInline);
  cmdBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, c_pipeline);
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

void Pipeline::Impl::setClearColor(float r, float g, float b){
  clear_color = {r,g,b};
}

void Pipeline::Impl::cook(){
    if(cooked) return;

    // Prepare vkPipeline etc.
    std::cout << "Cooking a pipeline." << std::endl;

    // The following part is a complete mess, and currently assumes no customization can be done.
    // It will be gradually cleaned up.

    // descriptor set layout
    std::shared_ptr<vkhlf::DescriptorSetLayout> descriptorSetLayout = global::device->createDescriptorSetLayout({});
    // pipeline layout
    std::shared_ptr<vkhlf::PipelineLayout> pipelineLayout = global::device->createPipelineLayout(descriptorSetLayout, nullptr);

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
      vk::ShaderStageFlagBits::eVertex,   vertexShader->impl->shader, "main");
    vkhlf::PipelineShaderStageCreateInfo fragmentStage(
      vk::ShaderStageFlagBits::eFragment, fragmentShader->impl->shader, "main");

    // Prepare input bindings according to vertexInputLayout.
    static std::map<DataType, vk::Format> dataTypeLayout = {
      {DataType::SByte,  vk::Format::eR8G8B8A8Snorm},
      {DataType::SByte2, vk::Format::eR8G8B8A8Snorm},
      {DataType::SByte3, vk::Format::eR8G8B8A8Snorm},
      {DataType::SByte4, vk::Format::eR8G8B8A8Snorm},
      {DataType::UByte,  vk::Format::eR8G8B8A8Unorm},
      {DataType::UByte2, vk::Format::eR8G8B8A8Unorm},
      {DataType::UByte3, vk::Format::eR8G8B8A8Unorm},
      {DataType::UByte4, vk::Format::eR8G8B8A8Unorm},
      {DataType::SInt,  vk::Format::eR32G32B32A32Sint},
      {DataType::SInt2, vk::Format::eR32G32B32A32Sint},
      {DataType::SInt3, vk::Format::eR32G32B32A32Sint},
      {DataType::SInt4, vk::Format::eR32G32B32A32Sint},
      {DataType::UInt,  vk::Format::eR32G32B32A32Uint},
      {DataType::UInt2, vk::Format::eR32G32B32A32Uint},
      {DataType::UInt3, vk::Format::eR32G32B32A32Uint},
      {DataType::UInt4, vk::Format::eR32G32B32A32Uint},
      {DataType::Float,  vk::Format::eR32G32B32A32Sfloat},
      {DataType::Float2, vk::Format::eR32G32B32A32Sfloat},
      {DataType::Float3, vk::Format::eR32G32B32A32Sfloat},
      {DataType::Float4, vk::Format::eR32G32B32A32Sfloat},
    };
    std::vector<vk::VertexInputAttributeDescription> attribs;
    size_t offset = 0, n = 0;
    for(DataType dt : vertexShader->impl->inputLayout.layout){
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
      {}, true, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);
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
      pipelineLayout,
      c_renderPass);
  
    cooked = true;
}

} // namespace sga
