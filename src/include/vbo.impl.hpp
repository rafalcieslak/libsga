#ifndef __VBO_IMPL_HPP__
#define __VBO_IMPL_HPP__

#include <sga/vbo.hpp>

#include <vkhlf/vkhlf.h>

namespace sga{

class VBO::Impl{
public:
  Impl(DataLayout layout, unsigned int size);

  SGA_API DataLayout getLayout() const;
  SGA_API unsigned int getSize() const;

  // TODO: Make sure data is written before any draw is performed.
  void putData(uint8_t* pData, size_t n);
  void putData(uint8_t* pData, size_t n_elem, size_t elem_size);

  friend class Pipeline;
private:
  std::shared_ptr<vkhlf::Buffer> buffer;
  DataLayout layout;
  unsigned int size;
};

class IBO::Impl{
public:
  Impl(unsigned int n);

  unsigned int getSize() const;

  // TODO: Make sure data is written before any draw is performed.
  void putData(uint8_t* pData, unsigned int elem_n);

  friend class Pipeline;
private:
  std::shared_ptr<vkhlf::Buffer> buffer;
  unsigned int n;
};

} // namespace sga

#endif // __VBO_IMPL_HPP__
