#include <sga/vbo.hpp>
#include "vbo.impl.hpp"

#include <map>

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

VBOBase::Impl::Impl(unsigned int ds, unsigned int s)
  : datasize(ds), size(s) {

  buffer = impl_global::device->createBuffer(
    datasize * size,
    vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
    vk::SharingMode::eExclusive,
    nullptr,
    vk::MemoryPropertyFlagBits::eDeviceLocal,
    nullptr);
}

void VBOBase::Impl::putData(uint8_t *pData, size_t n){
  size_t offset = 0, size = n;
  std::shared_ptr<vkhlf::Buffer> stagingBuffer = impl_global::device->createBuffer(
    n,
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
