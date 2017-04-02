#include <sga/image.hpp>
#include "image.impl.hpp"

#include <iostream>
#include <cassert>

#include <sga/exceptions.hpp>
#include "window.impl.hpp"
#include "global.hpp"
#include "utils.hpp"

namespace sga{

Image::Image(int width, int height, unsigned int ch, ImageFormat format) : impl(std::make_unique<Image::Impl>(width, height, ch, format)) {}
Image::~Image() = default;

void Image::putDataRaw(char * data, unsigned int n, DataType dtype, size_t value_size) {impl->putDataRaw(data, n, dtype, value_size);}
void Image::getDataRaw(char * data, unsigned int n, DataType dtype, size_t value_size) {impl->getDataRaw(data, n, dtype, value_size);}

void Image::copyOnto(std::shared_ptr<Image> target,
                     int source_x, int source_y,
                     int target_x, int target_y,
                     int cwidth, int cheight){
  impl->copyOnto(target,source_x,source_y,target_x,target_y,cwidth,cheight);
}
unsigned int Image::getWidth() {return impl->getWidth();}
unsigned int Image::getHeight() {return impl->getHeight();}
unsigned int Image::getChannels() {return impl->getChannels();}
unsigned int Image::getValuesN() {return impl->getValuesN();}

Image::Impl::Impl(unsigned int width, unsigned int height, unsigned int ch, ImageFormat f) :
  width(width), height(height), channels(ch) {
  if(!global::initialized){
    SystemError("NotInitialized", "libSGA was not initialized, please call sga::init() first!").raise();
  }
  if(ch == 0)
    ImageFormatError("ZeroChannelImage", "An image must have at least one channel.").raise();
  if(ch > 4)
    ImageFormatError("TooManyChannels", "An image must have at most four channels.").raise();
  
  userFormat = f;
  format = getFormatProperties(channels, userFormat);
  
  image = global::device->createImage(
    vk::ImageCreateFlags(),
    vk::ImageType::e2D,
    format.vkFormat,
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

  // Switch to any layout that may be restored (Preinitialized cannot be switched to), so that this->withLayout works correctly.
  current_layout = vk::ImageLayout::ePreinitialized;
  switchLayout(vk::ImageLayout::eGeneral);

  // Clear image.
  std::vector<uint8_t> data(N_pixels() * format.pixelSize, 0);
  putDataRaw((char*)data.data(), data.size(), format.transferDataType, format.pixelSize/channels);

  image_view = image->createImageView(vk::ImageViewType::e2D, format.vkFormat);
  
  out_dbg("Image prepared.");
}

const FormatProperties& getFormatProperties(unsigned int channels, ImageFormat format){
#define csR vk::ComponentSwizzle::eR
#define csG vk::ComponentSwizzle::eG
#define csB vk::ComponentSwizzle::eB
#define csA vk::ComponentSwizzle::eA
#define cs1 vk::ComponentSwizzle::eOne
#define cs0 vk::ComponentSwizzle::eZero
  static std::map<std::pair<unsigned int, ImageFormat>, FormatProperties> m = {
    {{1,ImageFormat::SInt8},  {vk::Format::eR8Sint,    DataType::SInt,  DataType::SInt,  1, 1, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::NInt8},  {vk::Format::eR8Unorm,   DataType::Float, DataType::UInt,  1, 1, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::UInt8},  {vk::Format::eR8Uint,    DataType::UInt,  DataType::UInt,  1, 1, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::SInt16}, {vk::Format::eR16Sint,   DataType::SInt,  DataType::SInt,  2, 2, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::UInt16}, {vk::Format::eR16Uint,   DataType::UInt,  DataType::UInt,  2, 2, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::SInt32}, {vk::Format::eR32Sint,   DataType::SInt,  DataType::SInt,  4, 4, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::UInt32}, {vk::Format::eR32Uint,   DataType::UInt,  DataType::UInt,  4, 4, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::Float},  {vk::Format::eR32Sfloat, DataType::Float, DataType::Float, 4, 4, {csR, cs0, cs0, cs1}}},
                              
    {{2,ImageFormat::SInt8},  {vk::Format::eR8G8Sint,     DataType::SInt2,  DataType::SInt,  2, 2, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::NInt8},  {vk::Format::eR8G8Unorm,    DataType::Float2, DataType::UInt,  2, 2, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::UInt8},  {vk::Format::eR8G8Uint,     DataType::UInt2,  DataType::UInt,  2, 2, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::SInt16}, {vk::Format::eR16G16Sint,   DataType::SInt2,  DataType::SInt,  4, 4, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::UInt16}, {vk::Format::eR16G16Uint,   DataType::UInt2,  DataType::UInt,  4, 4, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::SInt32}, {vk::Format::eR32G32Sint,   DataType::SInt2,  DataType::SInt,  8, 8, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::UInt32}, {vk::Format::eR32G32Uint,   DataType::UInt2,  DataType::UInt,  8, 8, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::Float},  {vk::Format::eR32G32Sfloat, DataType::Float2, DataType::Float, 8, 8, {csR, csG, cs0, cs1}}},
                              
    {{3,ImageFormat::SInt8},  {vk::Format::eR8G8B8A8Sint,       DataType::SInt3,  DataType::SInt,   3,  4, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::NInt8},  {vk::Format::eR8G8B8A8Unorm,      DataType::Float3, DataType::UInt,   3,  4, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::UInt8},  {vk::Format::eR8G8B8A8Uint,       DataType::UInt3,  DataType::UInt,   3,  4, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::SInt16}, {vk::Format::eR16G16B16A16Sint,   DataType::SInt3,  DataType::SInt,   6,  8, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::UInt16}, {vk::Format::eR16G16B16A16Uint,   DataType::UInt3,  DataType::UInt,   6,  8, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::SInt32}, {vk::Format::eR32G32B32A32Sint,   DataType::SInt3,  DataType::SInt,  12, 16, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::UInt32}, {vk::Format::eR32G32B32A32Uint,   DataType::UInt3,  DataType::UInt,  12, 16, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::Float},  {vk::Format::eR32G32B32A32Sfloat, DataType::Float3, DataType::Float, 12, 16, {csR, csG, csB, cs1}}},
                                                                                                  
    {{4,ImageFormat::SInt8},  {vk::Format::eR8G8B8A8Sint,       DataType::SInt4,  DataType::SInt,   4,  4, {csR, csG, csB, csA}}},
    {{4,ImageFormat::NInt8},  {vk::Format::eR8G8B8A8Unorm,      DataType::Float4, DataType::UInt,   4,  4, {csR, csG, csB, csA}}},
    {{4,ImageFormat::UInt8},  {vk::Format::eR8G8B8A8Uint,       DataType::UInt4,  DataType::UInt,   4,  4, {csR, csG, csB, csA}}},
    {{4,ImageFormat::SInt16}, {vk::Format::eR16G16B16A16Sint,   DataType::SInt4,  DataType::SInt,   8,  8, {csR, csG, csB, csA}}},
    {{4,ImageFormat::UInt16}, {vk::Format::eR16G16B16A16Uint,   DataType::UInt4,  DataType::UInt,   8,  8, {csR, csG, csB, csA}}},
    {{4,ImageFormat::SInt32}, {vk::Format::eR32G32B32A32Sint,   DataType::SInt4,  DataType::SInt,  16, 16, {csR, csG, csB, csA}}},
    {{4,ImageFormat::UInt32}, {vk::Format::eR32G32B32A32Uint,   DataType::UInt4,  DataType::UInt,  16, 16, {csR, csG, csB, csA}}},
    {{4,ImageFormat::Float},  {vk::Format::eR32G32B32A32Sfloat, DataType::Float4, DataType::Float, 16, 16, {csR, csG, csB, csA}}},
  };
  auto it = m.find({channels,format});
  if(it == m.end()){
    // If the implementation is correct, this should not happen.
    ImageFormatError("InvalidFormat", "This combination of channel and format is invalid");
  }
  return it->second;
}

void Image::Impl::switchLayout(vk::ImageLayout target_layout){
  if(target_layout == current_layout) return;

  //out_dbg("Image requires layout switch.");
  executeOneTimeCommands([&](auto cmdBuffer){
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        current_layout, target_layout);
    });

  current_layout = target_layout;
}

void Image::Impl::withLayout(vk::ImageLayout il, std::function<void()> f){
  auto orig_layout = current_layout;
  switchLayout(il);
  f();
  switchLayout(orig_layout);
}

void Image::Impl::putDataRaw(char * data, unsigned int n, DataType dtype, size_t value_size){
  if(n != N_pixels() * format.pixelSize )
    ImageFormatError("InvalidPutDataSize", "Data for Image::putData has " + std::to_string(n) + " values, expected " + std::to_string(N_pixels() * format.pixelSize) + ".").raise();
  if(dtype != format.transferDataType || value_size * channels != format.pixelSize)
    //TODO: State what would be the right variable to use for this image format
    ImageFormatError("InvalidPutDataType", "Data for Image::putData has type that does not match image format.").raise();

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
      
  executeOneTimeCommands([&](auto cmdBuffer){
      vkhlf::setImageLayout(
        cmdBuffer, stagingImage, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::ePreinitialized, vk::ImageLayout::eGeneral);
    });

  size_t data_size = width * height * format.stride;
  char* mapped_data = (char*)stagingImage->get<vkhlf::DeviceMemory>()->map(0, data_size);
  

  // TODO: It might be better to test subresource layout (rowPitch, stride, etc) instead of using custom hardcoded values.
  // vk::SubresourceLayout layout = stagingImage->getSubresourceLayout(vk::ImageAspectFlagBits::eColor, 0, 0);

  // This pointer walks over mapped memory.
  uint8_t* __restrict__ q_mapped = reinterpret_cast<uint8_t*>(mapped_data);
  // This pointer walks over host memory.
  uint8_t* __restrict__ q_data = reinterpret_cast<uint8_t*>(data);
    
  if(value_size == 1){
    for (size_t y = 0; y < height; y++){
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint8_t*)q_mapped;
        auto p_data = (uint8_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_mapped[c] = p_data[c];
        }
        q_mapped += format.stride;
        q_data += format.pixelSize;
      }
    }
  }else if(value_size == 2){
    for (size_t y = 0; y < height; y++){
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint16_t*)q_mapped;
        auto p_data = (uint16_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_mapped[c] = p_data[c];
        }
        q_mapped += format.stride;
        q_data += format.pixelSize;
      }
    }
  }else if(value_size == 4){
    for (size_t y = 0; y < height; y++){
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint32_t*)q_mapped;
        auto p_data = (uint32_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_mapped[c] = p_data[c];
        }
        q_mapped += format.stride;
        q_data += format.pixelSize;
      }
    }
  }else{
    assert(false);
  }
  
  stagingImage->get<vkhlf::DeviceMemory>()->flush(0, data_size);
  stagingImage->get<vkhlf::DeviceMemory>()->unmap();

  withLayout(vk::ImageLayout::eTransferDstOptimal, [&](){
      executeOneTimeCommands([&](auto cmdBuffer){
          // Switch staging image layout
          vkhlf::setImageLayout(
            cmdBuffer, stagingImage, vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
          // Perform copy
          cmdBuffer->copyImage(
            stagingImage, vk::ImageLayout::eTransferSrcOptimal,
            image, vk::ImageLayout::eTransferDstOptimal,
            vk::ImageCopy(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
                          vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
                          image->getExtent()
              )
            );
        }); // execute one time commands
    }); // with layout
}

void Image::Impl::getDataRaw(char * data, unsigned int n, DataType dtype, size_t value_size){
  if(n != N_pixels() * format.pixelSize )
    ImageFormatError("InvalidGetDataSize", "Data for Image::getData has " + std::to_string(n) + " values, expected " + std::to_string(N_pixels() * format.pixelSize) + ".").raise();
  if(dtype != format.transferDataType || value_size * channels != format.pixelSize)
    //TODO: State what would be the right variable to use for this image format
    ImageFormatError("InvalidGetDataType", "Data for Image::getData has type that does not match image format.").raise();
  
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


  withLayout(vk::ImageLayout::eTransferSrcOptimal, [&](){
      executeOneTimeCommands([&](auto cmdBuffer){
          vkhlf::setImageLayout(
            cmdBuffer, stagingImage, vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferDstOptimal);
          cmdBuffer->copyImage(
            image, vk::ImageLayout::eTransferSrcOptimal,
            stagingImage, vk::ImageLayout::eTransferDstOptimal,
            vk::ImageCopy(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
                          vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0),
                          image->getExtent()
              )
            );
          vkhlf::setImageLayout(
            cmdBuffer, stagingImage, vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
        }); // one time commands
    }); // with layout



  size_t data_size = width * height * format.stride;
  char* mapped_data = (char*)stagingImage->get<vkhlf::DeviceMemory>()->map(0, data_size);
  

  // TODO: It might be better to test subresource layout (rowPitch, stride, etc) instead of using custom hardcoded values.
  // vk::SubresourceLayout layout = stagingImage->getSubresourceLayout(vk::ImageAspectFlagBits::eColor, 0, 0);

  // This pointer walks over mapped memory.
  uint8_t* __restrict__ q_mapped = reinterpret_cast<uint8_t*>(mapped_data);
  // This pointer walks over host memory.
  uint8_t* __restrict__ q_data = reinterpret_cast<uint8_t*>(data);
    
  if(value_size == 1){
    for (size_t y = 0; y < height; y++){
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint8_t*)q_mapped;
        auto p_data = (uint8_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_data[c] = p_mapped[c];
        }
        q_mapped += format.stride;
        q_data += format.pixelSize;
      }
    }
  }else if(value_size == 2){
    for (size_t y = 0; y < height; y++){
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint16_t*)q_mapped;
        auto p_data = (uint16_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_data[c] = p_mapped[c];
        }
        q_mapped += format.stride;
        q_data += format.pixelSize;
      }
    }
  }else if(value_size == 4){
    for (size_t y = 0; y < height; y++){
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint32_t*)q_mapped;
        auto p_data = (uint32_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_data[c] = p_mapped[c];
        }
        q_mapped += format.stride;
        q_data += format.pixelSize;
      }
    }
  }else{
    assert(false);
  }
  
  
  stagingImage->get<vkhlf::DeviceMemory>()->unmap();
}

static void correct_bounds(
  int& source_x, int& source_y,
  int& target_x, int& target_y,
  int& cwidth, int& cheight,
  int width, int height,
  int twidth, int theight){

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
      
      auto source_orig_layout = current_layout;
      auto target_orig_layout = target->impl->current_layout;
      
      vkhlf::setImageLayout(
        cmdBuffer, target_image, vk::ImageAspectFlagBits::eColor,
        target_orig_layout, vk::ImageLayout::eTransferDstOptimal);

      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        source_orig_layout, vk::ImageLayout::eTransferSrcOptimal);
      
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
        vk::ImageLayout::eTransferSrcOptimal, source_orig_layout);
        
      vkhlf::setImageLayout(
        cmdBuffer, target_image, vk::ImageAspectFlagBits::eColor,
          vk::ImageLayout::eTransferDstOptimal, target_orig_layout);
      
    });
}

} // namespace sga
