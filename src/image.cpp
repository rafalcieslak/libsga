#include <sga/image.hpp>
#include "image.impl.hpp"

#include <iostream>
#include <cassert>

#include <sga/exceptions.hpp>
#include "window.impl.hpp"
#include "global.hpp"
#include "utils.hpp"

namespace sga{

Image::Image(int width, int height) : impl(std::make_unique<Image::Impl>(width, height)) {}
Image::~Image() = default;

void Image::fillWithPink() {impl->fillWithPink();}
void Image::testContents() {impl->testContents();}
void Image::putData(std::vector<uint8_t> data) {impl->putData(data);}
void Image::putDataRaw(unsigned char* data, size_t size) {impl->putDataRaw(data, size);}
std::vector<uint8_t> Image::getData() {return impl->getData();}
void Image::copyOnto(std::shared_ptr<Image> target,
                     int source_x, int source_y,
                     int target_x, int target_y,
                     int cwidth, int cheight){
  impl->copyOnto(target,source_x,source_y,target_x,target_y,cwidth,cheight);
}
/*
void Image::copyOnto(std::shared_ptr<Window> target,
                     int source_x, int source_y,
                     int target_x, int target_y,
                     int cwidth, int cheight){
  impl->copyOnto(target,source_x,source_y,target_x,target_y,cwidth,cheight);
}
*/


Image::Impl::Impl(unsigned int width, unsigned int height) :
  width(width), height(height) {
  if(!global::initialized){
    std::cout << "libSGA was not initialized!" << std::endl;
    throw std::runtime_error("NotInitialized");
  }
  prepareImage();
}
void Image::Impl::prepareImage(){
  auto FORMAT = vk::Format::eR8G8B8A8Unorm;
  
  image = global::device->createImage(
    vk::ImageCreateFlags(),
    vk::ImageType::e2D,
    FORMAT,
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

  // Clear image.
  std::vector<uint8_t> data(width*height*4, 0);
  putData(data);

  image_view = image->createImageView(vk::ImageViewType::e2D, FORMAT);
  
  std::cout << "Image prepared." << std::endl;
}
void Image::Impl::putData(std::vector<uint8_t> data){
  putDataRaw(data.data(), data.size());
}
void Image::Impl::putDataRaw(unsigned char* data, size_t size){
  unsigned int N = width * height * 4;
  if(N != size){
    std::cout << "Data for Image::putData has " << size << " elements, expected " << N << std::endl;
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

std::vector<uint8_t> Image::Impl::getData(){
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

static void correct_bounds(
  int& source_x, int& source_y,
  int& target_x, int& target_y,
  int& cwidth, int& cheight,
  int width, int height,
  int twidth, int theight){

/*
  std::cout << " ===== " << std::endl;
  std::cout << source_x << " " << source_y << std::endl;
  std::cout << target_x << " " << target_y << std::endl;
  std::cout << cwidth << " " << cheight << std::endl;
*/
  // Correct bounds.
  int off_x = 0, off_y = 0;
  if(source_x < 0){ off_x = -source_x; }
  if(source_y < 0){ off_y = -source_y; }
  source_x += off_x; target_x += off_x;  cwidth -= off_x;
  source_y += off_y; target_y += off_y; cheight -= off_y;
  off_x = off_y = 0;
  if(target_x < 0){ off_x = -target_x; }
  if(target_y < 0){ off_y = -target_y; }
  source_x += off_x; target_x += off_x;  cwidth -= off_x;
  source_y += off_y; target_y += off_y; cheight -= off_y;
  int clip_x = 0, clip_y = 0;
  if(source_x +  cwidth >  (int)width) { clip_x = source_x +  cwidth -  width; }
  if(source_y + cheight > (int)height) { clip_y = source_y + cheight - height; }
  cwidth -= clip_x;
  cheight -= clip_y;
  clip_x = clip_y = 0;
  if(target_x +  cwidth >  twidth) { clip_x = target_x +  cwidth -  twidth; }
  if(target_y + cheight > theight) { clip_y = target_y + cheight - theight; }
  cwidth -= clip_x;
  cheight -= clip_y;
  if(cwidth <= 0 || cheight <= 0) return;
/*
  std::cout << " ===== " << std::endl;
  std::cout << source_x << " " << source_y << std::endl;
  std::cout << target_x << " " << target_y << std::endl;
  std::cout << cwidth << " " << cheight << std::endl;
*/
}

void Image::Impl::copyOnto(std::shared_ptr<Image> target,
    int source_x, int source_y,
    int target_x, int target_y,
    int cwidth, int cheight){
  // Default argument values
  if(cwidth < 0) cwidth = image->getExtent().width;
  if(cheight < 0) cheight = image->getExtent().height;

  auto target_image = target->impl->image;
  int twidth = target_image->getExtent().width;
  int theight = target_image->getExtent().height;

  correct_bounds(source_x, source_y, target_x, target_y,
                 cwidth, cheight, width, height, twidth, theight);
  
  executeOneTimeCommands([&](auto cmdBuffer){
      vkhlf::setImageLayout(
        cmdBuffer, target_image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal);
      
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal);
      
      cmdBuffer->copyImage(
        image, vk::ImageLayout::eTransferSrcOptimal,
        target_image, vk::ImageLayout::eTransferDstOptimal,
        vk::ImageCopy(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(source_x, source_y, 0),
                      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(target_x, target_y, 0),
                      vk::Extent3D(cwidth, cheight, 0)
          )
        );
      
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        
      vkhlf::setImageLayout(
        cmdBuffer, target_image, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
      
    });
}

// The following requires creating the swapchain with TRANSFER_DST bit, which is not supported by some hardware.
/*
void Image::Impl::copyOnto(std::shared_ptr<Window> target,
    int source_x, int source_y,
    int target_x, int target_y,
    int cwidth, int cheight){
  // Default argument values
  if(cwidth < 0) cwidth = image->getExtent().width;
  if(cheight < 0) cheight = image->getExtent().height;

  auto target_image = target->impl->getCurrentImage();
  vk::Extent2D extent = target->impl->getCurrentFramebuffer().second;
  int twidth = extent.width;
  int theight = extent.height;

  correct_bounds(source_x, source_y, target_x, target_y,
                 cwidth, cheight, width, height, twidth, theight);
  
  executeOneTimeCommands([&](auto cmdBuffer){
      vkhlf::setImageLayout(
        cmdBuffer, target_image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal);
      
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferSrcOptimal);
      
      cmdBuffer->copyImage(
        image, vk::ImageLayout::eTransferSrcOptimal,
        target_image, vk::ImageLayout::eTransferDstOptimal,
        vk::ImageCopy(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(source_x, source_y, 0),
                      vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(target_x, target_y, 0),
                      vk::Extent3D(cwidth, cheight, 0)
          )
        );
      
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        
      vkhlf::setImageLayout(
        cmdBuffer, target_image, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
      
    });
}
*/

void Image::Impl::fillWithPink() {
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

void Image::Impl::testContents() {
  auto data = getData();
  for(int i = 0; i < 10*4; i+= 4){
    std::cout << (int)data[i+0] << " " << (int)data[i+1] << " " << (int)data[i+2] << " " << (int)data[i+3] << std::endl;
  }
}

} // namespace sga
