#include <sga/image.hpp>
#include "image.impl.hpp"

#include <iostream>
#include <cassert>
#include <cmath>

#include <sga/exceptions.hpp>
#include "stbi.hpp"
#include "window.impl.hpp"
#include "global.hpp"
#include "utils.hpp"
#include "scheduler.hpp"

namespace sga{

Image::Image(int width, int height, unsigned int ch, ImageFormat format, ImageFilterMode filtermode) : impl(std::make_shared<Image::Impl>(width, height, ch, format, filtermode)) {}
Image::Image(std::string png_path, ImageFormat format, ImageFilterMode filtermode) : impl(Image::Impl::createFromPNG(png_path, format, filtermode)) {}
Image::~Image() = default;

void Image::putDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size) {impl->putDataRaw(data, n, dtype, value_size);}
void Image::getDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size) {impl->getDataRaw(data, n, dtype, value_size);}
void Image::loadPNG(std::string filepath) {impl->loadPNG(filepath);}
void Image::savePNG(std::string filepath) {impl->savePNG(filepath);}
void Image::setClearColor(ImageClearColor cc) {return impl->setClearColor(cc);}
void Image::clear() {return impl->clear();}
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

Image::Impl::Impl(unsigned int width, unsigned int height, unsigned int ch, ImageFormat f, ImageFilterMode filtermode) :
  width(width), height(height), channels(ch), filtermode(filtermode), clearColor(f,ch) {
  if(!global::initialized){
    SystemError("NotInitialized", "libSGA was not initialized, please call sga::init() first!").raise();
  }
  if(ch == 0)
    ImageFormatError("ZeroChannelImage", "An image must have at least one channel.").raise();
  if(ch > 4)
    ImageFormatError("TooManyChannels", "An image must have at most four channels.").raise();
  
  userFormat = f;
  format = getFormatProperties(channels, userFormat);

  if(hasMipmaps() && !format.supports_blit){
    ImageFormatError("MipmapsUnsupported", "This format does not support mipmaps").raise();
  }
  
  unsigned int mipsno = hasMipmaps() ? getDesiredMipsNo() : 1;
  image = global::device->createImage(
    vk::ImageCreateFlags(),
    vk::ImageType::e2D,
    format.vkFormat,
    vk::Extent3D(width, height, 1),
    mipsno,
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
  putDataRaw(data.data(), data.size(), format.transferDataType, format.pixelSize/channels);

  regenerateMips();

  vk::ComponentMapping components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
  vk::ImageSubresourceRange subresRange = { vk::ImageAspectFlagBits::eColor, 0, mipsno, 0, 1 };
  image_view = image->createImageView(vk::ImageViewType::e2D, format.vkFormat, components, subresRange);
  
  out_dbg("Image prepared.");

  //TODO: Do not let the user use an image that has no data.
}

const FormatProperties& getFormatProperties(unsigned int channels, ImageFormat format){
#define csR vk::ComponentSwizzle::eR
#define csG vk::ComponentSwizzle::eG
#define csB vk::ComponentSwizzle::eB
#define csA vk::ComponentSwizzle::eA
#define cs1 vk::ComponentSwizzle::eOne
#define cs0 vk::ComponentSwizzle::eZero
  static std::map<std::pair<unsigned int, ImageFormat>, FormatProperties> m = {
    {{1,ImageFormat::SInt8},  {vk::Format::eR8Sint,    DataType::SInt,  DataType::SInt,  1, 1, {csR, cs0, cs0, cs1}, true}},
    {{1,ImageFormat::NInt8},  {vk::Format::eR8Unorm,   DataType::Float, DataType::UInt,  1, 1, {csR, cs0, cs0, cs1}, true}},
    {{1,ImageFormat::UInt8},  {vk::Format::eR8Uint,    DataType::UInt,  DataType::UInt,  1, 1, {csR, cs0, cs0, cs1}, true}},
    {{1,ImageFormat::SInt16}, {vk::Format::eR16Sint,   DataType::SInt,  DataType::SInt,  2, 2, {csR, cs0, cs0, cs1}, true}},
    {{1,ImageFormat::UInt16}, {vk::Format::eR16Uint,   DataType::UInt,  DataType::UInt,  2, 2, {csR, cs0, cs0, cs1}, true}},
    {{1,ImageFormat::SInt32}, {vk::Format::eR32Sint,   DataType::SInt,  DataType::SInt,  4, 4, {csR, cs0, cs0, cs1}, true}},
    {{1,ImageFormat::UInt32}, {vk::Format::eR32Uint,   DataType::UInt,  DataType::UInt,  4, 4, {csR, cs0, cs0, cs1}, true}},
    {{1,ImageFormat::Float},  {vk::Format::eR32Sfloat, DataType::Float, DataType::Float, 4, 4, {csR, cs0, cs0, cs1}, true}},
                              
    {{2,ImageFormat::SInt8},  {vk::Format::eR8G8Sint,     DataType::SInt2,  DataType::SInt,  2, 2, {csR, csG, cs0, cs1}, true}},
    {{2,ImageFormat::NInt8},  {vk::Format::eR8G8Unorm,    DataType::Float2, DataType::UInt,  2, 2, {csR, csG, cs0, cs1}, true}},
    {{2,ImageFormat::UInt8},  {vk::Format::eR8G8Uint,     DataType::UInt2,  DataType::UInt,  2, 2, {csR, csG, cs0, cs1}, true}},
    {{2,ImageFormat::SInt16}, {vk::Format::eR16G16Sint,   DataType::SInt2,  DataType::SInt,  4, 4, {csR, csG, cs0, cs1}, true}},
    {{2,ImageFormat::UInt16}, {vk::Format::eR16G16Uint,   DataType::UInt2,  DataType::UInt,  4, 4, {csR, csG, cs0, cs1}, true}},
    {{2,ImageFormat::SInt32}, {vk::Format::eR32G32Sint,   DataType::SInt2,  DataType::SInt,  8, 8, {csR, csG, cs0, cs1}, true}},
    {{2,ImageFormat::UInt32}, {vk::Format::eR32G32Uint,   DataType::UInt2,  DataType::UInt,  8, 8, {csR, csG, cs0, cs1}, true}},
    {{2,ImageFormat::Float},  {vk::Format::eR32G32Sfloat, DataType::Float2, DataType::Float, 8, 8, {csR, csG, cs0, cs1}, true}},
                              
    {{3,ImageFormat::SInt8},  {vk::Format::eR8G8B8A8Sint,       DataType::SInt3,  DataType::SInt,   3,  4, {csR, csG, csB, cs1}, true}},
    {{3,ImageFormat::NInt8},  {vk::Format::eR8G8B8A8Unorm,      DataType::Float3, DataType::UInt,   3,  4, {csR, csG, csB, cs1}, true}},
    {{3,ImageFormat::UInt8},  {vk::Format::eR8G8B8A8Uint,       DataType::UInt3,  DataType::UInt,   3,  4, {csR, csG, csB, cs1}, true}},
    {{3,ImageFormat::SInt16}, {vk::Format::eR16G16B16A16Sint,   DataType::SInt3,  DataType::SInt,   6,  8, {csR, csG, csB, cs1}, true}},
    {{3,ImageFormat::UInt16}, {vk::Format::eR16G16B16A16Uint,   DataType::UInt3,  DataType::UInt,   6,  8, {csR, csG, csB, cs1}, true}},
    {{3,ImageFormat::SInt32}, {vk::Format::eR32G32B32A32Sint,   DataType::SInt3,  DataType::SInt,  12, 16, {csR, csG, csB, cs1}, true}},
    {{3,ImageFormat::UInt32}, {vk::Format::eR32G32B32A32Uint,   DataType::UInt3,  DataType::UInt,  12, 16, {csR, csG, csB, cs1}, true}},
    {{3,ImageFormat::Float},  {vk::Format::eR32G32B32A32Sfloat, DataType::Float3, DataType::Float, 12, 16, {csR, csG, csB, cs1}, true}},

    {{4,ImageFormat::SInt8},  {vk::Format::eR8G8B8A8Sint,       DataType::SInt4,  DataType::SInt,   4,  4, {csR, csG, csB, csA}, true}},
    {{4,ImageFormat::NInt8},  {vk::Format::eR8G8B8A8Unorm,      DataType::Float4, DataType::UInt,   4,  4, {csR, csG, csB, csA}, true}},
    {{4,ImageFormat::UInt8},  {vk::Format::eR8G8B8A8Uint,       DataType::UInt4,  DataType::UInt,   4,  4, {csR, csG, csB, csA}, true}},
    {{4,ImageFormat::SInt16}, {vk::Format::eR16G16B16A16Sint,   DataType::SInt4,  DataType::SInt,   8,  8, {csR, csG, csB, csA}, true}},
    {{4,ImageFormat::UInt16}, {vk::Format::eR16G16B16A16Uint,   DataType::UInt4,  DataType::UInt,   8,  8, {csR, csG, csB, csA}, true}},
    {{4,ImageFormat::SInt32}, {vk::Format::eR32G32B32A32Sint,   DataType::SInt4,  DataType::SInt,  16, 16, {csR, csG, csB, csA}, true}},
    {{4,ImageFormat::UInt32}, {vk::Format::eR32G32B32A32Uint,   DataType::UInt4,  DataType::UInt,  16, 16, {csR, csG, csB, csA}, true}},
    {{4,ImageFormat::Float},  {vk::Format::eR32G32B32A32Sfloat, DataType::Float4, DataType::Float, 16, 16, {csR, csG, csB, csA}, true}},
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
  vk::ImageSubresourceRange subresRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
  Scheduler::buildAndSubmitSynced("Switching image layout", [&](auto cmdBuffer){
      vkhlf::setImageLayout(
        cmdBuffer, image, subresRange, current_layout, target_layout);
    });

  current_layout = target_layout;
}

void Image::Impl::withLayout(vk::ImageLayout il, std::function<void()> f){
  auto orig_layout = current_layout;
  switchLayout(il);
  f();
  switchLayout(orig_layout);
}

void Image::Impl::putDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size){
  if(n != N_pixels() * format.pixelSize)
    ImageFormatError("InvalidPutDataSize", "Data for Image::putData has " + std::to_string(n) + " values, expected " + std::to_string(N_pixels() * format.pixelSize) + ".").raise();
  if(dtype != format.transferDataType || value_size * channels != format.pixelSize)
    //TODO: State what would be the right variable to use for this image format
    ImageFormatError("InvalidPutDataType", "Data for Image::putData has type that does not match image format.").raise();

  auto stagingImage = image->get<vkhlf::Device>()->createImage(
    {},
    image->getType(),
    image->getFormat(),
    image->getExtent(),
    1,
    image->getArrayLayers(),
    image->getSamples(),
    vk::ImageTiling::eLinear,
    vk::ImageUsageFlagBits::eTransferSrc,
    image->getSharingMode(),
    image->getQueueFamilyIndices(),
    vk::ImageLayout::ePreinitialized,
    vk::MemoryPropertyFlagBits::eHostVisible,
    nullptr, image->get<vkhlf::Allocator>());
      
  Scheduler::buildAndSubmitSynced("Preparing staging image layout", [&](auto cmdBuffer){
      vkhlf::setImageLayout(
        cmdBuffer, stagingImage, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::ePreinitialized, vk::ImageLayout::eGeneral);
    });

  size_t data_size = width * height * format.stride;
  char* mapped_data = (char*)stagingImage->get<vkhlf::DeviceMemory>()->map(0, data_size);
  
  vk::SubresourceLayout layout = stagingImage->getSubresourceLayout(vk::ImageAspectFlagBits::eColor, 0, 0);

  // This pointer walks over mapped memory.
  uint8_t* RESTRICT q_mapped_row = reinterpret_cast<uint8_t*>(mapped_data);
  uint8_t* RESTRICT q_mapped_px;
  // This pointer walks over host memory.
  uint8_t* RESTRICT q_data = reinterpret_cast<uint8_t*>(data);
    
  if(value_size == 1){
    for (size_t y = 0; y < height; y++){
      q_mapped_px = q_mapped_row;
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint8_t*)q_mapped_px;
        auto p_data = (uint8_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_mapped[c] = p_data[c];
        }
        q_mapped_px += format.stride;
        q_data += format.pixelSize;
      }
      q_mapped_row += layout.rowPitch;
    }
  }else if(value_size == 2){
    for (size_t y = 0; y < height; y++){
      q_mapped_px = q_mapped_row;
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint16_t*)q_mapped_px;
        auto p_data = (uint16_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_mapped[c] = p_data[c];
        }
        q_mapped_px += format.stride;
        q_data += format.pixelSize;
      }
      q_mapped_row += layout.rowPitch;
    }
  }else if(value_size == 4){
    for (size_t y = 0; y < height; y++){
      q_mapped_px = q_mapped_row;
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint32_t*)q_mapped_px;
        auto p_data = (uint32_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_mapped[c] = p_data[c];
        }
        q_mapped_px += format.stride;
        q_data += format.pixelSize;
      }
      q_mapped_row += layout.rowPitch;
    }
  }else{
    assert(false);
  }
  
  stagingImage->get<vkhlf::DeviceMemory>()->flush(0, data_size);
  stagingImage->get<vkhlf::DeviceMemory>()->unmap();

  withLayout(vk::ImageLayout::eTransferDstOptimal, [&](){
      Scheduler::buildAndSubmitSynced("Copying staging image to main image", [&](auto cmdBuffer){
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

  regenerateMips();
}

void Image::Impl::getDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size){
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
      Scheduler::buildAndSubmitSynced("Copying main image to staging image", [&](auto cmdBuffer){
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
  
  vk::SubresourceLayout layout = stagingImage->getSubresourceLayout(vk::ImageAspectFlagBits::eColor, 0, 0);

  // This pointer walks over mapped memory.
  uint8_t* RESTRICT q_mapped_row = reinterpret_cast<uint8_t*>(mapped_data);
  uint8_t* RESTRICT q_mapped_px;
  // This pointer walks over host memory.
  uint8_t* RESTRICT q_data = reinterpret_cast<uint8_t*>(data);
    
  if(value_size == 1){
    for (size_t y = 0; y < height; y++){
      q_mapped_px = q_mapped_row;
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint8_t*)q_mapped_px;
        auto p_data = (uint8_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_data[c] = p_mapped[c];
        }
        q_mapped_px += format.stride;
        q_data += format.pixelSize;
      }
      q_mapped_row += layout.rowPitch;
    }
  }else if(value_size == 2){
    for (size_t y = 0; y < height; y++){
      q_mapped_px = q_mapped_row;
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint16_t*)q_mapped_px;
        auto p_data = (uint16_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_data[c] = p_mapped[c];
        }
        q_mapped_px += format.stride;
        q_data += format.pixelSize;
      }
      q_mapped_row += layout.rowPitch;
    }
  }else if(value_size == 4){
    for (size_t y = 0; y < height; y++){
      q_mapped_px = q_mapped_row;
      for (size_t x = 0; x < width; x++){
        auto p_mapped = (uint32_t*)q_mapped_px;
        auto p_data = (uint32_t*)q_data;
        for(size_t c = 0; c < channels; c++){
          p_data[c] = p_mapped[c];
        }
        q_mapped_px += format.stride;
        q_data += format.pixelSize;
      }
      q_mapped_row += layout.rowPitch;
    }
  }else{
    assert(false);
  }
  
  stagingImage->get<vkhlf::DeviceMemory>()->unmap();
}

void Image::Impl::setClearColor(ImageClearColor cc){
  if(cc.getComponents() != channels)
    ImageFormatError("ClearChannelMismatch", "The clear color used for clearning this image has " + std::to_string(cc.getComponents()) + " values, while the image has " + std::to_string(channels) + " channels.").raise();
  // TODO: Print out human-readable format name!
  if(cc.getFormat() != userFormat)
    ImageFormatError("ClearFormatMismatch", "The clear color used for clearning this image uses a different format than the image itself.").raise();

  clearColor = cc;
}

void Image::Impl::clear(){
  Scheduler::buildAndSubmitSynced("Clearing image", [&](std::shared_ptr<vkhlf::CommandBuffer> cmdBuffer){
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
      
      vk::ClearColorValue vcc = Utils::imageClearColorToVkClearColorValue(clearColor);
      cmdBuffer->clearColorImage(image, vk::ImageLayout::eTransferDstOptimal, vcc);
      
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        vk::ImageLayout::eTransferDstOptimal, current_layout);
    });
  regenerateMips();
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
  
  Scheduler::buildAndSubmitSynced("Copying from image to image", [&](auto cmdBuffer){
      
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

  target->impl->regenerateMips();
}


std::unique_ptr<Image::Impl> Image::Impl::createFromPNG(std::string filepath, ImageFormat format, ImageFilterMode filtermode){
  int w, h, n;
  unsigned char* imagedata = stbi_load(filepath.c_str(), &w, &h, &n, 0);
  if(!imagedata)
    FileAccessError("ImageFileOpenFailed", "Failed to open PNG image \"" + filepath + "\": " + stbi_failure_reason()).raise();

  auto res = std::make_unique<Image::Impl>(w, h, n, format, filtermode);
  res->loadPNGInternal(imagedata);
  return res;
}


void Image::Impl::loadPNG(std::string filepath){
  // Read image data.
  int w, h, n;
  unsigned char* imagedata = stbi_load(filepath.c_str(), &w, &h, &n, 0);
  if(!imagedata)
    FileAccessError("ImageFileOpenFailed", "Failed to open PNG image \"" + filepath + "\": " + stbi_failure_reason()).raise();
  if((unsigned int)w != width || (unsigned int)h != height)
    ImageFormatError("ImageOpenSizeMismatch", "The loaded image has different size than the target Image").raise();
  if((unsigned int)n != channels)
    ImageFormatError("ImageOpenChannelsMismatch", "The loaded image has different channel number than the target Image").raise();
  loadPNGInternal(imagedata);
}

void Image::Impl::loadPNGInternal(uint8_t* stbi_data){
  if(userFormat != ImageFormat::NInt8 && userFormat != ImageFormat::UInt8)
    ImageFormatError("LoadPNGFormatMismatch", "Loading images from PNG supports only NInt8 and UInt8 images.").raise();
  putDataRaw(stbi_data, width * height * channels, DataType::UInt, 1);
}

void Image::Impl::savePNG(std::string filepath){
  if(userFormat != ImageFormat::NInt8 && userFormat != ImageFormat::UInt8)
    ImageFormatError("LoadPNGFormatMismatch", "Saving images to PNG supports only NInt8 and UInt8 images.").raise();

  // TODO: Support floats?
  
  std::vector<uint8_t> out(N_values());
  getDataRaw(out.data(), getValuesN(), DataType::UInt, 1);
  int res = stbi_write_png(filepath.c_str(), width, height, channels, out.data(), 0);
  if (res != 0)
    FileAccessError("ImageFileWriteFailed", "Writing to file \"" + filepath + "\" failed.");
}

unsigned int Image::Impl::getDesiredMipsNo() const{
  return std::floor(std::log2(std::max(width, height))) + 1;
}

void Image::Impl::regenerateMips(){
  if(!hasMipmaps())
    return;

  unsigned int mipsno = getDesiredMipsNo();
  out_dbg("Regenerating image mipmaps (" + std::to_string(mipsno) + " levels)");

  switchLayout(vk::ImageLayout::eTransferSrcOptimal);

  Scheduler::buildAndSubmitSynced("Regenerating image mips", [&](auto cmdBuffer){
      for (unsigned int i = 1; i < mipsno; i++){
        vk::ImageBlit imageBlit;
        
        // Source
        imageBlit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = i-1;
        imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
        imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
        imageBlit.srcOffsets[1].z = 1;
        
        // Destination
        imageBlit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[1].x = int32_t(width >> i);
        imageBlit.dstOffsets[1].y = int32_t(height >> i);
        imageBlit.dstOffsets[1].z = 1;
        
        vk::ImageSubresourceRange mipSubresRange = { vk::ImageAspectFlagBits::eColor, i, 1, 0, 1 };
        
        // Transiton current mip level to transfer dest
        vkhlf::setImageLayout(cmdBuffer, image, mipSubresRange, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        cmdBuffer->blitImage(
          image, vk::ImageLayout::eTransferSrcOptimal,
          image, vk::ImageLayout::eTransferDstOptimal,
          {imageBlit}, vk::Filter::eLinear);
        
        // Transiton current mip level to transfer source for read in next iteration
        vkhlf::setImageLayout(cmdBuffer, image, mipSubresRange, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal);
      }

      // Transition all mip levels to shader read.
      vk::ImageSubresourceRange allSubresRange = { vk::ImageAspectFlagBits::eColor, 0, mipsno, 0, 1 };
      vkhlf::setImageLayout(cmdBuffer, image, allSubresRange, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
      current_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
    }); // Scheduler::buildSubmitAndSync
}

} // namespace sga
