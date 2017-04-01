#ifndef __SGA_IMAGE_HPP__
#define __SGA_IMAGE_HPP__

#include "config.hpp"
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
  
  void fillWithPink();
  void testContents();

  void putData(std::vector<uint8_t> data);
  void putDataRaw(unsigned char * data, size_t size);
  std::vector<uint8_t> getData();

  unsigned int getWidth();
  unsigned int getHeight();
  
  void copyOnto(
    std::shared_ptr<Image> target,
    int source_x = 0, int source_y = 0,
    int target_x = 0, int target_y = 0,
    int width = -1, int height = -1);

  friend class Pipeline;
private:
  Image(int width, int height, unsigned int ch, ImageFormat format);
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_IMAGE_HPP__
