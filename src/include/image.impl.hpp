#ifndef __IMAGE_IMPL_HPP__
#define __IMAGE_IMPL_HPP__

#include <sga/image.hpp>
#include <sga/layout.hpp>
#include <functional>

#include <vkhlf/vkhlf.h>

namespace sga{

class Window;

struct FormatProperties{
  vk::Format vkFormat;
  DataType shaderDataType;
  DataType transferDataType;

  // The size of 1 pixel (incorporates the number of channels).
  size_t pixelSize;

  // Offset between subsequent pixels.
  size_t stride;
  std::array<vk::ComponentSwizzle, 4> swizzle;
};

const FormatProperties& getFormatProperties(unsigned int channels, ImageFormat format);

class Image::Impl{
public:
  Impl(unsigned int width, unsigned int height, unsigned int channels, ImageFormat format);
  
  void putDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size);
  void getDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size);

  void loadPNG(std::string filepath);
  void loadPNGInternal(uint8_t* stbi_data);
  void savePNG(std::string filepath);
  
  void clear();
  
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

  unsigned int getWidth() {return width;}
  unsigned int getHeight() {return height;}
  unsigned int getChannels() {return channels;}
  unsigned int getValuesN() {return width * height * channels;}

  void withLayout(vk::ImageLayout il, std::function<void()> f);
  
  void switchLayout(vk::ImageLayout il);

  static std::unique_ptr<Image::Impl> createFromPNG(std::string png_path, ImageFormat format);
  
  friend class Pipeline;
private:
  const unsigned int width, height;
  const unsigned int channels;
  ImageFormat userFormat;
  FormatProperties format;

  // The number of pixels in this image.
  unsigned int N_pixels() const {return width * height;}
  // The number of values in this image. 
  unsigned int N_values() const {return width * height * channels;}
  
  vk::ImageLayout current_layout;
  
  std::shared_ptr<vkhlf::Image> image;
  std::shared_ptr<vkhlf::ImageView> image_view;
  void prepareImage();
};

} // namespace sga

#endif // __IMAGE_IMPL_HPP__
