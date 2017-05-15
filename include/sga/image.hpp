#ifndef __SGA_IMAGE_HPP__
#define __SGA_IMAGE_HPP__

#include "config.hpp"
#include "layout.hpp"
#include <vector>

namespace sga{

class Window;

enum class ImageFormat{
  SInt8,
  NInt8,
  UInt8,
  SInt16,
  UInt16,
  SInt32,
  UInt32,
  Float,
};

enum class ImageFilterMode{
  None,
  MipMapped,
  Anisotropic
};

class Image{
public:
  SGA_API static std::shared_ptr<Image> create(int width, int height, unsigned int ch = 4, ImageFormat format = ImageFormat::NInt8, ImageFilterMode filtermode = ImageFilterMode::None){
    return std::shared_ptr<Image>(new Image(width, height, ch, format, filtermode));
  }
  SGA_API static std::shared_ptr<Image> createFromPNG(std::string png_path, ImageFormat format = ImageFormat::NInt8, ImageFilterMode filtermode = ImageFilterMode::None){
    return std::shared_ptr<Image>(new Image(png_path, format, filtermode));
  }
  SGA_API ~Image();

  // TODO: Refactor these to use sizeof(datatype)
  SGA_API void putData(const std::vector<uint8_t>& data){
    putDataRaw((uint8_t*)data.data(), data.size(), DataType::UInt, 1);
  }
  SGA_API void putData(const std::vector<uint16_t>& data){
    putDataRaw((uint8_t*)data.data(), data.size()*2, DataType::UInt, 2);
  }
  SGA_API void putData(const std::vector<uint32_t>& data){
    putDataRaw((uint8_t*)data.data(), data.size()*4, DataType::UInt, 4);
  }
  SGA_API void putData(const std::vector<int8_t>& data){
    putDataRaw((uint8_t*)data.data(), data.size(), DataType::SInt, 1);
  }
  SGA_API void putData(const std::vector<int16_t>& data){
    putDataRaw((uint8_t*)data.data(), data.size()*2, DataType::SInt, 2);
  }
  SGA_API void putData(const std::vector<int32_t>& data){
    putDataRaw((uint8_t*)data.data(), data.size()*4, DataType::SInt, 4);
  }
  SGA_API void putData(const std::vector<float>& data){
    putDataRaw((uint8_t*)data.data(), data.size()*4, DataType::Float, 4);
  }
  

  SGA_API void getData(std::vector<uint8_t>& data){
    getDataRaw((uint8_t*)data.data(), data.size(), DataType::UInt, 1);
  }
  SGA_API void getData(std::vector<uint16_t>& data){
    getDataRaw((uint8_t*)data.data(), data.size()*2, DataType::UInt, 2);
  }
  SGA_API void getData(std::vector<uint32_t>& data){
    getDataRaw((uint8_t*)data.data(), data.size()*4, DataType::UInt, 4);
  }
  SGA_API void getData(std::vector<int8_t>& data){
    getDataRaw((uint8_t*)data.data(), data.size(), DataType::SInt, 1);
  }
  SGA_API void getData(std::vector<int16_t>& data){
    getDataRaw((uint8_t*)data.data(), data.size()*2, DataType::SInt, 2);
  }
  SGA_API void getData(std::vector<int32_t>& data){
    getDataRaw((uint8_t*)data.data(), data.size()*4, DataType::SInt, 4);
  }
  SGA_API void getData(std::vector<float>& data){
    getDataRaw((uint8_t*)data.data(), data.size()*4, DataType::Float, 4);
  }

  SGA_API void loadPNG(std::string filepath);
  SGA_API void savePNG(std::string filepath);
  
  SGA_API unsigned int getWidth();
  SGA_API unsigned int getHeight();
  SGA_API unsigned int getChannels();
  SGA_API unsigned int getValuesN();

  SGA_API void clear();
  
  SGA_API void copyOnto(
    std::shared_ptr<Image> target,
    int source_x = 0, int source_y = 0,
    int target_x = 0, int target_y = 0,
    int width = -1, int height = -1);

  friend class Pipeline;
private:
  SGA_API Image(int width, int height, unsigned int ch, ImageFormat format, ImageFilterMode filtermode);
  SGA_API Image(std::string png_path, ImageFormat format, ImageFilterMode filtermode);
  
  SGA_API void putDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size);
  SGA_API void getDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size);
  
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_IMAGE_HPP__
