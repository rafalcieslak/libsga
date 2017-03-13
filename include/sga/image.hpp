#ifndef __SGA_IMAGE_HPP__
#define __SGA_IMAGE_HPP__

#include "config.hpp"
#include <vector>

namespace sga{

class Window;

class Image{
public:
  ~Image();
  void fillWithPink();
  void testContents();

  void putData(std::vector<uint8_t> data);
  void putDataRaw(unsigned char * data, size_t size);
  std::vector<uint8_t> getData();

  void copyOnto(
    std::shared_ptr<Image> target,
    int source_x = 0, int source_y = 0,
    int target_x = 0, int target_y = 0,
    int width = -1, int height = -1);
  /*void copyOnto(
    std::shared_ptr<Window> target,
    int source_x = 0, int source_y = 0,
    int target_x = 0, int target_y = 0,
    int width = -1, int height = -1);*/

  static std::shared_ptr<Image> create(int width, int height){
    return std::shared_ptr<Image>(new Image(width, height));
  }
  friend class Pipeline;
private:
  Image(int width, int height);
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_IMAGE_HPP__
