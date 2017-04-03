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

class Image{
public:
  static std::shared_ptr<Image> create(int width, int height, unsigned int ch = 4, ImageFormat format = ImageFormat::NInt8){
    return std::shared_ptr<Image>(new Image(width, height, ch, format));
  }
  ~Image();

  void putData(const std::vector<uint8_t>& data){
    putDataRaw((char*)data.data(), data.size(), DataType::UInt, 1);
  }
  void putData(const std::vector<uint16_t>& data){
    putDataRaw((char*)data.data(), data.size(), DataType::UInt, 2);
  }
  void putData(const std::vector<uint32_t>& data){
    putDataRaw((char*)data.data(), data.size(), DataType::UInt, 4);
  }
  void putData(const std::vector<int8_t>& data){
    putDataRaw((char*)data.data(), data.size(), DataType::SInt, 1);
  }
  void putData(const std::vector<int16_t>& data){
    putDataRaw((char*)data.data(), data.size(), DataType::SInt, 2);
  }
  void putData(const std::vector<int32_t>& data){
    putDataRaw((char*)data.data(), data.size(), DataType::SInt, 4);
  }
  void putData(const std::vector<float>& data){
    putDataRaw((char*)data.data(), data.size(), DataType::Float, 4);
  }
  

  void getData(std::vector<uint8_t>& data){
    getDataRaw((char*)data.data(), data.size(), DataType::UInt, 1);
  }
  void getData(std::vector<uint16_t>& data){
    getDataRaw((char*)data.data(), data.size(), DataType::UInt, 2);
  }
  void getData(std::vector<uint32_t>& data){
    getDataRaw((char*)data.data(), data.size(), DataType::UInt, 4);
  }
  void getData(std::vector<int8_t>& data){
    getDataRaw((char*)data.data(), data.size(), DataType::SInt, 1);
  }
  void getData(std::vector<int16_t>& data){
    getDataRaw((char*)data.data(), data.size(), DataType::SInt, 2);
  }
  void getData(std::vector<int32_t>& data){
    getDataRaw((char*)data.data(), data.size(), DataType::SInt, 4);
  }
  void getData(std::vector<float>& data){
    getDataRaw((char*)data.data(), data.size(), DataType::Float, 4);
  }
  
  
  unsigned int getWidth();
  unsigned int getHeight();
  unsigned int getChannels();
  unsigned int getValuesN();

  void clear();
  
  void copyOnto(
    std::shared_ptr<Image> target,
    int source_x = 0, int source_y = 0,
    int target_x = 0, int target_y = 0,
    int width = -1, int height = -1);

  friend class Pipeline;
private:
  Image(int width, int height, unsigned int ch, ImageFormat format);
  
  void putDataRaw(char * data, unsigned int n, DataType dtype, size_t value_size);
  void getDataRaw(char * data, unsigned int n, DataType dtype, size_t value_size);
  
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_IMAGE_HPP__
