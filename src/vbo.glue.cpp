#include <sga/vbo.hpp>
#include "vbo.impl.hpp"

namespace sga {

VBO::VBO(DataLayout layout, unsigned int n)
  : impl(std::make_unique<VBO::Impl>(layout, n)){
}

VBO::~VBO() = default;

DataLayout VBO::getLayout() const{
  return impl->getLayout();
}

unsigned int VBO::getSize() const{
  return impl->getSize();
}

void VBO::putData(uint8_t* pData, size_t n){
  impl->putData(pData, n);
}

void VBO::putData(uint8_t* pData, size_t n_elem, size_t elem_size){
  impl->putData(pData, n_elem, elem_size);
}



IBO::IBO(unsigned int n)
  : impl(std::make_unique<IBO::Impl>(n)){
}

IBO::~IBO() = default;

unsigned int IBO::getSize() const{
  return impl->getSize();
}

void IBO::putData(uint8_t* pData, unsigned int elem_n){
  impl->putData(pData, elem_n);
}

} // namespace sga
