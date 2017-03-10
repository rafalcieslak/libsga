#ifndef __VBO_IMPL_HPP__
#define __VBO_IMPL_HPP__

#include <sga/vbo.hpp>

#include <vkhlf/vkhlf.h>

namespace sga{

class VBOBase::Impl{
public:
  Impl(unsigned int datasize, unsigned int size);
  
  void putData(uint8_t* pData, size_t n);
  void putData(uint8_t* pData, size_t n_elem, size_t elem_size);
private:
public: // TODO getter?
  std::shared_ptr<vkhlf::Buffer> buffer;

private:
  const unsigned int datasize;
  unsigned int size;
};

} // namespace sga

#endif // __VBO_IMPL_HPP__
