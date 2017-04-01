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
unsigned int Image::getWidth() {return impl->getWidth();}
unsigned int Image::getHeight() {return impl->getHeight();}


Image::Impl::Impl(unsigned int width, unsigned int height, unsigned int ch, ImageFormat f) :
  width(width), height(height) {
  if(!global::initialized){
    SystemError("NotInitialized", "libSGA was not initialized, please call sga::init() first!").raise();
  }
  if(ch == 0)
    ImageFormatError("ZeroChannelImage", "An image must have at least one channel.").raise();
  if(ch > 4)
    ImageFormatError("TooManyChannels", "An image must have at most four channels.").raise();
  
  channels = ch;
  userFormat = f;
  format = getFormatProperties(channels, userFormat);
  N_elems = width * height * ch;
  
  prepareImage();
}

const FormatProperties& getFormatProperties(unsigned int channels, ImageFormat format){
#define csR vk::ComponentSwizzle::eR
#define csG vk::ComponentSwizzle::eG
#define csB vk::ComponentSwizzle::eB
#define csA vk::ComponentSwizzle::eA
#define cs1 vk::ComponentSwizzle::eOne
#define cs0 vk::ComponentSwizzle::eZero
  static std::map<std::pair<unsigned int, ImageFormat>, FormatProperties> m = {
    {{1,ImageFormat::SInt8},  {vk::Format::eR8Sint,    DataType::SInt,  1, 1, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::NInt8},  {vk::Format::eR8Unorm,   DataType::Float, 1, 1, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::UInt8},  {vk::Format::eR8Uint,    DataType::UInt,  1, 1, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::SInt16}, {vk::Format::eR16Sint,   DataType::SInt,  2, 2, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::UInt16}, {vk::Format::eR16Uint,   DataType::UInt,  2, 2, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::SInt32}, {vk::Format::eR32Sint,   DataType::SInt,  4, 4, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::UInt32}, {vk::Format::eR32Uint,   DataType::UInt,  4, 4, {csR, cs0, cs0, cs1}}},
    {{1,ImageFormat::Float},  {vk::Format::eR32Sfloat, DataType::Float, 4, 4, {csR, cs0, cs0, cs1}}},
                              
    {{2,ImageFormat::SInt8},  {vk::Format::eR8G8Sint,     DataType::SInt2,  2, 2, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::NInt8},  {vk::Format::eR8G8Unorm,    DataType::Float2, 2, 2, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::UInt8},  {vk::Format::eR8G8Uint,     DataType::UInt2,  2, 2, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::SInt16}, {vk::Format::eR16G16Sint,   DataType::SInt2,  4, 4, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::UInt16}, {vk::Format::eR16G16Uint,   DataType::UInt2,  4, 4, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::SInt32}, {vk::Format::eR32G32Sint,   DataType::SInt2,  8, 8, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::UInt32}, {vk::Format::eR32G32Uint,   DataType::UInt2,  8, 8, {csR, csG, cs0, cs1}}},
    {{2,ImageFormat::Float},  {vk::Format::eR32G32Sfloat, DataType::Float2, 8, 8, {csR, csG, cs0, cs1}}},
                              
    {{3,ImageFormat::SInt8},  {vk::Format::eR8G8B8A8Sint,       DataType::SInt3,   3,  4, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::NInt8},  {vk::Format::eR8G8B8A8Unorm,      DataType::Float3,  3,  4, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::UInt8},  {vk::Format::eR8G8B8A8Uint,       DataType::UInt3,   3,  4, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::SInt16}, {vk::Format::eR16G16B16A16Sint,   DataType::SInt3,   6,  8, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::UInt16}, {vk::Format::eR16G16B16A16Uint,   DataType::UInt3,   6,  8, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::SInt32}, {vk::Format::eR32G32B32A32Sint,   DataType::SInt3,  12, 16, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::UInt32}, {vk::Format::eR32G32B32A32Uint,   DataType::UInt3,  12, 16, {csR, csG, csB, cs1}}},
    {{3,ImageFormat::Float},  {vk::Format::eR32G32B32A32Sfloat, DataType::Float3, 12, 16, {csR, csG, csB, cs1}}},
                              
    {{4,ImageFormat::SInt8},  {vk::Format::eR8G8B8A8Sint,       DataType::SInt4,   4,  4, {csR, csG, csB, csA}}},
    {{4,ImageFormat::NInt8},  {vk::Format::eR8G8B8A8Unorm,      DataType::Float4,  4,  4, {csR, csG, csB, csA}}},
    {{4,ImageFormat::UInt8},  {vk::Format::eR8G8B8A8Uint,       DataType::UInt4,   4,  4, {csR, csG, csB, csA}}},
    {{4,ImageFormat::SInt16}, {vk::Format::eR16G16B16A16Sint,   DataType::SInt4,   8,  8, {csR, csG, csB, csA}}},
    {{4,ImageFormat::UInt16}, {vk::Format::eR16G16B16A16Uint,   DataType::UInt4,   8,  8, {csR, csG, csB, csA}}},
    {{4,ImageFormat::SInt32}, {vk::Format::eR32G32B32A32Sint,   DataType::SInt4,  16, 16, {csR, csG, csB, csA}}},
    {{4,ImageFormat::UInt32}, {vk::Format::eR32G32B32A32Uint,   DataType::UInt4,  16, 16, {csR, csG, csB, csA}}},
    {{4,ImageFormat::Float},  {vk::Format::eR32G32B32A32Sfloat, DataType::Float4, 16, 16, {csR, csG, csB, csA}}},
  };
  auto it = m.find({channels,format});
  if(it == m.end()){
    // If the implementation is correct, this should not happen.
    ImageFormatError("InvalidFormat", "This combination of channel and format is invalid");
  }
  return it->second;
}

void Image::Impl::prepareImage(){
  
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
  std::vector<uint8_t> data(width*height*4, 0);
  putData(data);

  image_view = image->createImageView(vk::ImageViewType::e2D, format.vkFormat);
  
  out_dbg("Image prepared.");
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

void Image::Impl::putData(std::vector<uint8_t> data){
  putDataRaw(data.data(), data.size());
}
void Image::Impl::putDataRaw(unsigned char* data, size_t size){
  if(N_elems != size){
    std::cout << "Data for Image::putData has " << size << " elements, expected " << N_elems << std::endl;
    return;
  }
  
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

      auto orig_layout = current_layout;
      vkhlf::setImageLayout(
        cmdBuffer, image, vk::ImageAspectFlagBits::eColor,
        current_layout, vk::ImageLayout::eTransferSrcOptimal);
      
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
        vk::ImageLayout::eTransferSrcOptimal, orig_layout);
        
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
