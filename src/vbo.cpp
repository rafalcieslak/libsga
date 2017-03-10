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

VBOBase::VBOBase(unsigned int ds, unsigned int n)
  : impl(std::make_unique<VBOBase::Impl>(ds, n)),
    size(n) {}
VBOBase::~VBOBase() = default;
void VBOBase::putData(uint8_t* pData, size_t n){
  impl->putData(pData, n);
}
void VBOBase::putData(uint8_t* pData, size_t n_elem, size_t elem_size){
  impl->putData(pData, n_elem, elem_size);
}

VBOBase::Impl::Impl(unsigned int ds, unsigned int s)
  : datasize(ds), size(s) {

  buffer = global::device->createBuffer(
    datasize * size,
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eDeviceLocal,
    nullptr);
}

void VBOBase::Impl::putData(uint8_t *pData, size_t n_elem, size_t elem_size){
  if(elem_size != datasize){
    DataFormatError("VBODataFormatMismatch", "The size of data element used to write into a VBO does not match the element size of the VBO").raise();
  };
  
  if(n_elem != size){
    VBOSizeError("VBOWriteSizeMismatch", "Error: VBO size does not match written data size!", "The current size of a VBO does not match the size of the object used to write data to the VBO. If your input data is correct, consider using VBO::resize()").raise();
    // TODO: Shouldn't VBO resize happen automatically?
  }
  putData(pData, n_elem * elem_size);
}

void VBOBase::Impl::putData(uint8_t *pData, size_t n){
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
