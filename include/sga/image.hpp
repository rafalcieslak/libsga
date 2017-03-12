#ifndef __SGA_IMAGE_HPP__
#define __SGA_IMAGE_HPP__

#include "config.hpp"

namespace sga{

class Image{
public:
  Image(int width, int height);
  ~Image();
  void fillWithPink();
  void testContents();

private:
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_IMAGE_HPP__
