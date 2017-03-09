#include <sga/image.hpp>

#include <iostream>
#include <stdexcept>
#include <cassert>

#include <vkhlf/vkhlf.h>

#include "global.hpp"
#include "utils.hpp"

namespace sga{

class Image::Impl{
public:
  Impl(unsigned int width, unsigned int height) :
    width(width), height(height) {
    if(!global::initialized){
      std::cout << "libSGA was not initialized!" << std::endl;
      throw std::runtime_error("NotInitialized");
    }
    prepareImage();
  }
private:
  unsigned int width, height;
  std::shared_ptr<vkhlf::Image> image;
  void prepareImage(){
    image = global::device->createImage(
      vk::ImageCreateFlags(),
      vk::ImageType::e2D,
      vk::Format::eR8G8B8A8Unorm,
      vk::Extent3D(width, height, 1),
      1,
      1,
      vk::SampleCountFlagBits::e1,
      vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eColorAttachment | 
        vk::ImageUsageFlagBits::eSampled ,
      vk::SharingMode::eExclusive,
      std::vector<uint32_t>(), // queue family indices
      vk::ImageLayout::ePreinitialized,
      vk::MemoryPropertyFlagBits::eDeviceLocal,
      nullptr, nullptr
      );

    
    executeOneTimeCommands([&](auto cmdBuffer){
        vkhlf::setImageLayout(
          cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::ePreinitialized, vk::ImageLayout::eShaderReadOnlyOptimal);
      });
    
    std::cout << "Image prepared." << std::endl;
  }
public:
  void putData(std::vector<uint8_t> data){
    unsigned int N = data.size();
    unsigned int size = width * height * 4;
    if(N != size){
      std::cout << "Data for Image::putData has " << N << " elements, expected " << size << std::endl;
      return;
    }
    
    executeOneTimeCommands([&](auto cmdBuffer){
        auto stagingImage = image->get<vkhlf::Device>()->createImage(
          {},
          image->getType(),
          image->getFormat(),
          image->getExtent(),
          image->getMipLevels(),
          image->getArrayLayers(),
          image->getSamples(),
          vk::ImageTiling::eLinear,
          vk::ImageUsageFlagBits::eTransferSrc,
          image->getSharingMode(),
          image->getQueueFamilyIndices(),
          vk::ImageLayout::ePreinitialized,
          vk::MemoryPropertyFlagBits::eHostVisible,
          nullptr, image->get<vkhlf::Allocator>());
        
        vkhlf::setImageLayout(
          cmdBuffer, stagingImage, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::ePreinitialized, vk::ImageLayout::eGeneral);
        
        uint8_t* pdata = reinterpret_cast<uint8_t*>( stagingImage->get<vkhlf::DeviceMemory>()->map(0, size) );
        vk::SubresourceLayout layout = stagingImage->getSubresourceLayout(vk::ImageAspectFlagBits::eColor, 0, 0);

        unsigned int n = 0;
        for (size_t y = 0; y < height; y++){
          uint8_t * rowPtr = pdata;
          for (size_t x = 0; x < width; x++){
            rowPtr[0] = data[n + 0]; // red
            rowPtr[1] = data[n + 1]; // green
            rowPtr[2] = data[n + 2]; // blue
            rowPtr[3] = data[n + 3]; // alpha
            rowPtr += 4;
            n += 4;
            }
          pdata += layout.rowPitch;
        }
        assert(n == size);

        stagingImage->get<vkhlf::DeviceMemory>()->flush(0, size);
        stagingImage->get<vkhlf::DeviceMemory>()->unmap();

        vkhlf::setImageLayout(
          cmdBuffer, stagingImage, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
        vkhlf::setImageLayout(
          cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        cmdBuffer->copyImage(
          stagingImage, vk::ImageLayout::eTransferSrcOptimal,
          image, vk::ImageLayout::eTransferDstOptimal,
          vk::ImageCopy(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
                        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
                        image->getExtent()
            )
          );
        vkhlf::setImageLayout(
          cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
      }); // execute one time commands
  }

  std::vector<uint8_t> getData(){
    unsigned int size = width * height * 4;
    std::vector<uint8_t> data(size);
    
    auto stagingImage = image->get<vkhlf::Device>()->createImage(
      {},
      image->getType(),
      image->getFormat(),
      image->getExtent(),
      image->getMipLevels(),
      image->getArrayLayers(),
      image->getSamples(),
      vk::ImageTiling::eLinear,
      vk::ImageUsageFlagBits::eTransferDst,
      image->getSharingMode(),
      image->getQueueFamilyIndices(),
      vk::ImageLayout::ePreinitialized,
      vk::MemoryPropertyFlagBits::eHostVisible,
      nullptr, image->get<vkhlf::Allocator>());
        
    executeOneTimeCommands([&](auto cmdBuffer){
        
        
        vkhlf::setImageLayout(
          cmdBuffer, stagingImage, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferDstOptimal);
        
        vkhlf::setImageLayout(
          cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal);
        
        cmdBuffer->copyImage(
          image, vk::ImageLayout::eTransferSrcOptimal,
          stagingImage, vk::ImageLayout::eTransferDstOptimal,
          vk::ImageCopy(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
                        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
                        image->getExtent()
            )
          );

        vkhlf::setImageLayout(
          cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        
        vkhlf::setImageLayout(
          cmdBuffer, stagingImage, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
        
      });

    
    uint8_t* pdata = reinterpret_cast<uint8_t*>( stagingImage->get<vkhlf::DeviceMemory>()->map(0, size) );
    vk::SubresourceLayout layout = stagingImage->getSubresourceLayout(vk::ImageAspectFlagBits::eColor, 0, 0);
    
    unsigned int n = 0;
    for (size_t y = 0; y < height; y++){
      uint8_t * rowPtr = pdata;
      for (size_t x = 0; x < width; x++){
        data[n + 0] = rowPtr[0]; // red
        data[n + 1] = rowPtr[1]; // green
        data[n + 2] = rowPtr[2]; // blue
        data[n + 3] = rowPtr[3]; // alpha
        rowPtr += 4;
        n += 4;
      }
      pdata += layout.rowPitch;
    }
    assert(n == size);
    
    stagingImage->get<vkhlf::DeviceMemory>()->unmap();
    
    return data;
  }
  
  void fillWithPink() {
    unsigned int size = width * height * 4;
    std::vector<uint8_t> data(size);
    for(unsigned int i = 0; i < size; i += 4){
      data[i + 0] = 255;
      data[i + 1] = 0;
      data[i + 2] = 255;
      data[i + 3] = 255;
    }
    putData(data);
    std::cout << "Image filled with pink!" << std::endl;
  }

  
  void testContents() {
    auto data = getData();
    for(int i = 0; i < 10*4; i+= 4){
      std::cout << (int)data[i+0] << " " << (int)data[i+1] << " " << (int)data[i+2] << " " << (int)data[i+4] << std::endl;
    }
  }
};

Image::Image(int width, int height) : impl(std::make_unique<Image::Impl>(width, height)) {}
Image::~Image() = default;

void Image::fillWithPink() {impl->fillWithPink();}
void Image::testContents() {impl->testContents();}

} // namespace sga
