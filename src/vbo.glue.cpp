#include <sga/vbo.hpp>
#include "vbo.impl.hpp"

namespace sga {

VBO::VBO(DataLayout layout, unsigned int n)
  : impl(std::make_unique<VBO::Impl>(layout, n)){
}

VBO::~VBO() = default;

void VBO::putData(uint8_t* pData, size_t n){
  impl->putData(pData, n);
}

void VBO::putData(uint8_t* pData, size_t n_elem, size_t elem_size){
  impl->putData(pData, n_elem, elem_size);
}

DataLayout VBO::getLayout() const{
  return impl->getLayout();
}

unsigned int VBO::getSize() const{
  return impl->getSize();
}

} // namespace sga
