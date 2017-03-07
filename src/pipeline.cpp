#include <sga/pipeline.hpp>
#include "pipeline.impl.hpp"

#include <iostream>
#include <cassert>

#include <vkhlf/vkhlf.h>

#include "global.hpp"
#include "utils.hpp"
#include "window.impl.hpp"
#include "vbo.impl.hpp"

namespace sga{

Pipeline::Pipeline() : impl(std::make_unique<Pipeline::Impl>()) {}
Pipeline::~Pipeline() = default;


void Pipeline::setTarget(std::shared_ptr<Window> target) {impl->setTarget(target);}
void Pipeline::drawTestTriangle() {impl->drawTestTriangle();}
void Pipeline::drawVBO(std::shared_ptr<VBOBase> vbo) {impl->drawVBO(vbo);}
void Pipeline::setClearColor(float r, float g, float b) {impl->setClearColor(r, g, b);}

// ====== IMPL ======

// shaders
static char const *testVertShaderText =
R"(#version 430
layout(location = 0) in vec2 inVertex;
layout(location = 1) in vec4 inColor;
layout(location = 0) out vec4 outColor;
out gl_PerVertex
{
  vec4 gl_Position;
};
void main()
{
  outColor = inColor;
  gl_Position = vec4(inVertex, 0, 1);
}
)";

static char const *testFragShaderText = 
R"(#version 430
layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 outColor;
void main()
{
  outColor = inColor;
}
)";


Pipeline::Impl::Impl(){
  
}
  
void Pipeline::Impl::setTarget(std::shared_ptr<Window> tgt){
    cooked = false;
    target_is_window = true;
    targetWindow = tgt;
    // TODO: Reset shared ptr to target image
}
void Pipeline::Impl::drawTestTriangle(){
  // TODO: When drawin a test triangle, what data layout shall we use?
  
  // Prepare vertex buffer.
  struct TestVertex
  {
    float   position[2];
    uint8_t color[4];
  };
  
  const std::vector<TestVertex> test_vertices =
    {
      { {  0.0f, -0.5f },{ 0xFF, 0x00, 0x00, 0xFF }, },
      { {  0.5f,  0.5f },{ 0x00, 0x00, 0xFF, 0xFF }, },
      { { -0.5f,  0.5f },{ 0x00, 0xFF, 0x00, 0xFF }, },
    };
  
  auto vertexBuffer = impl_global::device->createBuffer(
    test_vertices.size() * sizeof(TestVertex),
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eDeviceLocal,
    nullptr);
  
  executeOneTimeCommands([&](auto cmdBuffer){
      vertexBuffer->update<TestVertex>(
        0,
        { uint32_t(test_vertices.size()), test_vertices.data() },
        cmdBuffer);
    });

  drawBuffer(vertexBuffer, test_vertices.size());
}
void Pipeline::Impl::drawVBO(std::shared_ptr<VBOBase> vbo){
  if(vbo->getLayout() != vertexInputLayout){
    std::cout << "ERROR: VBO layout does not match pipeline input layout!" << std::endl;
    // TODO: More verbose output?
    return;
  }
  
  drawBuffer(vbo->impl->buffer, vbo->size);
}

void Pipeline::Impl::drawBuffer(std::shared_ptr<vkhlf::Buffer> buffer, unsigned int n){  
  cook();
  
  std::shared_ptr<vkhlf::Framebuffer> framebuffer;
  vk::Extent2D extent;
  std::tie(framebuffer, extent) = targetWindow->impl->getCurrentFramebuffer();
  
  
  auto cmdBuffer = impl_global::commandPool->allocateCommandBuffer();
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
  
  auto fence = impl_global::device->createFence(false);
  impl_global::queue->submit(
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
    std::shared_ptr<vkhlf::DescriptorSetLayout> descriptorSetLayout = impl_global::device->createDescriptorSetLayout({});
    // pipeline layout
    std::shared_ptr<vkhlf::PipelineLayout> pipelineLayout = impl_global::device->createPipelineLayout(descriptorSetLayout, nullptr);
    
    // init shaders
    std::shared_ptr<vkhlf::ShaderModule> testVertexShader = impl_global::device->createShaderModule(
      vkhlf::compileGLSLToSPIRV(vk::ShaderStageFlagBits::eVertex, testVertShaderText));
    std::shared_ptr<vkhlf::ShaderModule> testFragmentShader = impl_global::device->createShaderModule(
      vkhlf::compileGLSLToSPIRV(vk::ShaderStageFlagBits::eFragment, testFragShaderText));

    // Take renderpass and framebuffer
    if(target_is_window){
      c_renderPass = targetWindow->impl->renderPass;
    }else{
      std::cout << "UNIMPLMENTED [1]" << std::endl;
    }
    
    // init pipeline
    std::shared_ptr<vkhlf::PipelineCache> pipelineCache = impl_global::device->createPipelineCache(0, nullptr);
    vkhlf::PipelineShaderStageCreateInfo vertexStage(
      vk::ShaderStageFlagBits::eVertex,   testVertexShader, "main");
    vkhlf::PipelineShaderStageCreateInfo fragmentStage(
      vk::ShaderStageFlagBits::eFragment, testFragmentShader, "main");

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
    for(DataType dt : vertexInputLayout.layout){
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
    
    c_pipeline = impl_global::device->createGraphicsPipeline(
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
