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

  void putData(std::vector<uint8_t> data){
    putDataRaw(data.data(), data.size(), DataType::UInt, 1);
  }
  void putData(std::vector<uint16_t> data);
  void putData(std::vector<uint32_t> data);
  void putData(std::vector<int8_t> data);
  void putData(std::vector<int16_t> data);
  void putData(std::vector<int32_t> data);
  void putData(std::vector<float> data);
  

  void getData(std::vector<uint8_t>& data){
    getDataRaw(data.data(), data.size(), DataType::UInt, 1);
  }
  void getData(std::vector<uint16_t>& data);
  void getData(std::vector<uint32_t>& data);
  void getData(std::vector<int8_t>& data);
  void getData(std::vector<int16_t>& data);
  void getData(std::vector<int32_t>& data);
  void getData(std::vector<float>& data);
  
  
  unsigned int getWidth();
  unsigned int getHeight();
  unsigned int getChannels();
  unsigned int getElems();
  
  void copyOnto(
    std::shared_ptr<Image> target,
    int source_x = 0, int source_y = 0,
    int target_x = 0, int target_y = 0,
    int width = -1, int height = -1);

  friend class Pipeline;
private:
  Image(int width, int height, unsigned int ch, ImageFormat format);
  
  void putDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t elem_size);
  void getDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t elem_size);
  
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_IMAGE_HPP__
