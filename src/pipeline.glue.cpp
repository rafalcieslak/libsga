#include <sga/pipeline.hpp>
#include "pipeline.impl.hpp"

namespace sga {

Pipeline::Pipeline()
  : impl_(std::make_shared<Pipeline::Impl>()) {
}

Pipeline::Pipeline(pimpl_unique_ptr<Impl> &&impl)
  : impl_(std::move(impl)) {
}

Pipeline::~Pipeline() = default;

void Pipeline::setTarget(const Window& target) {
  impl()->setTarget(target);
}

void Pipeline::setTarget(std::vector<Image> images) {
  impl()->setTarget(images);
}

void Pipeline::drawVBO(const VBO& vbo) {
  impl()->drawVBO(vbo);
}

void Pipeline::clear() {
  impl()->clear();
}

void Pipeline::setProgram(const Program& p) {
  impl()->setProgram(p);
}

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

void Pipeline::setRasterizerMode(sga::RasterizerMode r){
  impl()->setRasterizerMode(r);
}

void Pipeline::setPolygonMode(sga::PolygonMode p){
  impl()->setPolygonMode(p);
}

void Pipeline::setLineWidth(float w){
  impl()->setLineWidth(w);
}

void Pipeline::setBlendModeColor(BlendFactor src, BlendFactor dst, BlendOperation op){
  impl()->setBlendModeColor(src,dst,op);
}
void Pipeline::setBlendModeAlpha(BlendFactor src, BlendFactor dst, BlendOperation op){
  impl()->setBlendModeAlpha(src,dst,op);
}

void Pipeline::resetViewport(){
  impl()->resetViewport();
}

void Pipeline::setViewport(float left, float top, float right, float bottom){
  impl()->setViewport(left, top, right, bottom);
}


FullQuadPipeline::FullQuadPipeline() :
  Pipeline(std::make_unique<FullQuadPipeline::Impl>()){
}

FullQuadPipeline::~FullQuadPipeline() = default;

FullQuadPipeline::Impl* FullQuadPipeline::impl(){
  return dynamic_cast<FullQuadPipeline::Impl*>(Pipeline::impl());
}

void FullQuadPipeline::setProgram(const Program& p) {
  impl()->setProgram(p);
}

void FullQuadPipeline::drawFullQuad() {
  impl()->drawFullQuad();
}

} // namespace sga
