#ifndef __IMAGE_IMPL_HPP__
#define __IMAGE_IMPL_HPP__

#include <sga/image.hpp>

#include <vkhlf/vkhlf.h>

namespace sga{

class Window;

class Image::Impl{
public:
  Impl(unsigned int width, unsigned int height);
  
  void putData(std::vector<uint8_t> data);
  std::vector<uint8_t> getData();

  void copyOnto(
    std::shared_ptr<Image> target,
    int source_x = 0, int source_y = 0,
    int target_x = 0, int target_y = 0,
    int width = -1, int height = -1);
  
  void copyOnto(
    std::shared_ptr<Window> target,
    int source_x = 0, int source_y = 0,
    int target_x = 0, int target_y = 0,
    int width = -1, int height = -1);
  
  void fillWithPink();
  void testContents();
private:
  unsigned int width, height;
  std::shared_ptr<vkhlf::Image> image;
  void prepareImage();
};

} // namespace sga

#endif // __IMAGE_IMPL_HPP__
