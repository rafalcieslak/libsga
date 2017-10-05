#include "vbo.impl.hpp"

#include <map>

#include <sga/exceptions.hpp>
#include "utils.hpp"
#include "global.hpp"
#include "scheduler.hpp"

namespace sga{

DataLayout::DataLayout(std::initializer_list<DataType> l) :
  layout(l) {
}
DataLayout::~DataLayout() = default;

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

DataLayout VBO::Impl::getLayout() const{
  return layout;
}
unsigned int VBO::Impl::getSize() const{
  return size;
}

void VBO::Impl::putData(uint8_t *pData, size_t n_elem, size_t elem_size){
  if(elem_size != layout.byteSize()){
    DataFormatError("VBODataFormatMismatch", "The size of data element used to write into a VBO does not match the element size of the VBO").raise();
  };

  if(n_elem != size){
    SizeError("VBOWriteSizeMismatch", "Error: VBO size does not match written data size!", "The current size of a VBO does not match the size of the object used to write data to the VBO. If your input data is correct, consider using VBO::resize()").raise();
    // TODO: Shouldn't VBO resize happen automatically?
    // TODO: Is VBO::resize() even implemented?
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

  Scheduler::buildAndSubmitSynced("Putting VBO data", [&](auto cmdBuffer){
      cmdBuffer->copyBuffer(stagingBuffer, buffer, vk::BufferCopy(0, 0, size));
    });
}


IBO::Impl::Impl(unsigned int n)
  : n(n){

  buffer = global::device->createBuffer(
    sizeof(uint16_t) * n,
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eDeviceLocal,
    nullptr);
}

unsigned int IBO::Impl::getSize() const{
  return n;
}

void IBO::Impl::putData(uint8_t *pData, unsigned int elem_n){
  if(elem_n != n){
    SizeError("IBOWriteSizeMismatch", "IBO size does not match the number of elements written to the IBO").raise();
  };

  size_t offset = 0, size = sizeof(uint16_t) * n;
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

  Scheduler::buildAndSubmitSynced("Putting IBO data", [&](auto cmdBuffer){
      cmdBuffer->copyBuffer(stagingBuffer, buffer, vk::BufferCopy(0, 0, size));
    });
}


} // namespace sga
