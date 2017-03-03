#include <sga/image.hpp>

namespace sga{

class Image::Impl{
public:
  Impl(unsigned int width, unsigned int height) :
    width(width), height(height) {
    
  }
private:
  unsigned int width, height;
};

Image::Image(int width, int height) : impl(std::make_unique<Image::Impl>(width, height)) {}
Image::~Image() = default;

} // namespace sga
