#include <sga/image.hpp>
#include "image.impl.hpp"

namespace sga {

Image::Image(int width, int height, unsigned int ch, ImageFormat format, ImageFilterMode filtermode)
  : impl(std::make_shared<Image::Impl>(width, height, ch, format, filtermode)) {
}

Image::Image(std::string png_path, ImageFormat format, ImageFilterMode filtermode)
  : impl(Image::Impl::createFromPNG(png_path, format, filtermode)) {
}

Image::~Image() = default;

void Image::putDataRaw(unsigned char * data, size_t n, DataType dtype, size_t value_size) {
  impl->putDataRaw(data, n, dtype, value_size);
}

void Image::getDataRaw(unsigned char * data, size_t n, DataType dtype, size_t value_size) {
  impl->getDataRaw(data, n, dtype, value_size);
}

void Image::loadPNG(std::string filepath) {
  impl->loadPNG(filepath);
}

void Image::savePNG(std::string filepath) {
  impl->savePNG(filepath);
}

void Image::setClearColor(ImageClearColor cc) {
  return impl->setClearColor(cc);
}

void Image::clear() {
  return impl->clear();
}

void Image::copyOnto(std::shared_ptr<Image> target,
                     int source_x, int source_y,
                     int target_x, int target_y,
                     int cwidth, int cheight){
  impl->copyOnto(target,source_x,source_y,target_x,target_y,cwidth,cheight);
}

unsigned int Image::getWidth() {
  return impl->getWidth();
}

unsigned int Image::getHeight() {
  return impl->getHeight();
}

unsigned int Image::getChannels() {
  return impl->getChannels();
}

unsigned int Image::getValuesN() {
  return impl->getValuesN();
}

} // namespace sga
