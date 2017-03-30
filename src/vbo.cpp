#include <sga/vbo.hpp>
#include "vbo.impl.hpp"

#include <map>

#include <sga/exceptions.hpp>
#include "utils.hpp"
#include "global.hpp"

namespace sga{

DataLayout::DataLayout(std::initializer_list<DataType> l) :
  layout(l) {
}
DataLayout::~DataLayout() = default;

VBO::VBO(DataLayout layout, unsigned int n)
  : impl(std::make_unique<VBO::Impl>(layout, n))
{}
VBO::~VBO() = default;
void VBO::putData(uint8_t* pData, size_t n){
  impl->putData(pData, n);
}
void VBO::putData(uint8_t* pData, size_t n_elem, size_t elem_size){
  impl->putData(pData, n_elem, elem_size);
}
DataLayout VBO::getLayout() const{
  return impl->layout;
}
size_t VBO::getDataSize() const{
  return impl->layout.byteSize();
}
unsigned int VBO::getSize() const{ return impl->getSize(); }

VBO::Impl::Impl(DataLayout layout, unsigned int s)
  : layout(layout), size(s) {

  buffer = global::device->createBuffer(
    layout.byteSize() * size,
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eDeviceLocal,
    nullptr);
}

unsigned int VBO::Impl::getSize() const{
  return size;
}

void VBO::Impl::putData(uint8_t *pData, size_t n_elem, size_t elem_size){
  if(elem_size != layout.byteSize()){
    DataFormatError("VBODataFormatMismatch", "The size of data element used to write into a VBO does not match the element size of the VBO").raise();
  };
  
  if(n_elem != size){
    VBOSizeError("VBOWriteSizeMismatch", "Error: VBO size does not match written data size!", "The current size of a VBO does not match the size of the object used to write data to the VBO. If your input data is correct, consider using VBO::resize()").raise();
    // TODO: Shouldn't VBO resize happen automatically?
  }
  putData(pData, n_elem * elem_size);
}

void VBO::Impl::putData(uint8_t *pData, size_t n){
  size_t offset = 0, size = n;
  std::shared_ptr<vkhlf::Buffer> stagingBuffer = global::device->createBuffer(
    size,
    vk::BufferUsageFlagBits::eTransferSrc,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eHostVisible,
    nullptr);
  auto devmem = stagingBuffer->get<vkhlf::DeviceMemory>();
  void * pMapped = devmem->map(offset, size);
  memcpy(pMapped, pData, size);
  devmem->flush(offset, size);
  devmem->unmap();
  
  executeOneTimeCommands([&](auto cmdBuffer){
      cmdBuffer->copyBuffer(stagingBuffer, buffer, vk::BufferCopy(0, 0, size));
    });
}

} // namespace sga
