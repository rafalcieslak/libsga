#ifndef __SGA_IMAGE_HPP__
#define __SGA_IMAGE_HPP__

#include "config.hpp"
#include "layout.hpp"
#include <vector>

namespace sga{

class Window;

/** @brief The type of values in an sga::Image.
 * Value formats supported by images. These specify the kind of data to be used
 * for each channel of each pixel of an sga::Image. */
enum class ImageFormat{
  SInt8, /// Signed 8-bit integers [-128..127]
  NInt8, /// Unsigned 8-bit integers normalized to [0..1] range. These values
         /// are used as floats [0..1] in shaders, but unsigned integers
         /// [0..255] in application. sga::Window always uses this format.
  UInt8, /// Unsigned 8-bit integers [0..255]
  SInt16, /// Signed 16-bit integers [-32768..32767]
  UInt16, /// Unsigned 16-bit integers [0..65536]
  SInt32, /// Signed 32-bit integers [-2147483648..2147483647]
  UInt32, /// Unsigned 32-bit ingegers [0..4294967296]
  Float,  /// Signed 32-bit IEEE floating point numbers
};

class Utils;
struct ImageClearColor{

  static ImageClearColor NInt8(int r){ return ImageClearColor(ImageFormat::NInt8, 1).setUInt32(r,0,0,255); }
  static ImageClearColor NInt8(int r, int g){ return ImageClearColor(ImageFormat::NInt8, 2).setUInt32(r,g,0,255); }
  static ImageClearColor NInt8(int r, int g, int b){ return ImageClearColor(ImageFormat::NInt8, 3).setUInt32(r,g,b,255); }
  static ImageClearColor NInt8(int r, int g, int b, int a){ return ImageClearColor(ImageFormat::NInt8, 4).setUInt32(r,g,b,a); }

  ImageClearColor(ImageFormat f, unsigned int c) : format(f), components(c), uint32{0,0,0,0} {}
  friend class Utils;
  ImageFormat getFormat() {return format;}
  unsigned int getComponents() {return components;}
private:
  ImageClearColor& setUInt32(unsigned int r, unsigned int g, unsigned int b, unsigned int a){
    uint32[0] = r; uint32[1] = g; uint32[2] = b; uint32[3] = a;
    return *this;
  }
  ImageFormat format;
  unsigned int components;
  union{
    uint32_t uint32[4];
    int32_t int32[4];
    float float32[4];
  };
};

enum class ImageFilterMode{
  None,
  MipMapped,
  Anisotropic
};

/** An sga::Image represents an area of video memory that is interpreted as an
 * image. It is commonly used for providing textures to samplers, and as a
 * source for renred pipelines. Images may be loaded from and saved to files.
 * Each Image has a fixed size, to resize an image you need to create a new one.
 * Images also have a constant number of channels and format, which describes
 * contents type (see ImageFrormat).
 *
 * Image is a reference type, so any copies of an instance refer to the same
 * underlying object.*/
class Image{
public:
  /** Creates a new Image, preparing video memory for it.
   * @param width The horizontal size of the image (in px)
   * @param height The vertical size of the image (in px)
   * @param channels The number of channels in the image (1, 2, 3 or 4).
   * @param format The sga::ImageFormat this image should use. This specifies
   * the type of data used for a single value in an image. For example, if you
   * choose UInt8 and use 4 channels, then each pixel will take 32 bits (8 bits
   * per component) and will be interpreted as an unsigned integer. For details,
   * see sga::ImageFormat.
   * @param filtermode If you want the image to use mipmaps or anisotropic
   * filterning, the mipmaps will be automatically generated after each image
   * modification if you enable mipmaps or anisotropic filtering with this
   * option. */
  SGA_API Image(int width, int height, unsigned int channels = 4,
                ImageFormat format = ImageFormat::NInt8,
                ImageFilterMode filtermode = ImageFilterMode::None);

  SGA_API static Image createFromPNG(std::string png_path, ImageFormat format = ImageFormat::NInt8, ImageFilterMode filtermode = ImageFilterMode::None){
    return Image(png_path, format, filtermode);
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

  SGA_API void setClearColor(ImageClearColor cc);
  SGA_API void clear();

  SGA_API void copyOnto(
    std::shared_ptr<Image> target,
    int source_x = 0, int source_y = 0,
    int target_x = 0, int target_y = 0,
    int width = -1, int height = -1);

  friend class Pipeline;
private:
  SGA_API Image(std::string png_path, ImageFormat format, ImageFilterMode filtermode);

  SGA_API void putDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size);
  SGA_API void getDataRaw(unsigned char * data, unsigned int n, DataType dtype, size_t value_size);

  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_IMAGE_HPP__
