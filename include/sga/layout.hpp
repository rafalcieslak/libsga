#ifndef __SGA_LAYOUT_HPP__
#define __SGA_LAYOUT_HPP__

#include "config.hpp"

#include <memory>
#include <vector>
#include <initializer_list>

namespace sga{

enum class DataType{
  SInt, SInt2, SInt3, SInt4,
  UInt, UInt2, UInt3, UInt4,
  Float, Float2, Float3, Float4,
  Double,
  Mat3, Mat4,
};

class DataLayout{
public:
  SGA_API ~DataLayout();
  SGA_API DataLayout() {}
  SGA_API DataLayout(std::initializer_list<DataType> types);
  SGA_API size_t byteSize() const;
  
  // TODO: This should be hidden from API.
  SGA_API size_t UBOsize() const;
  
  std::vector<DataType> layout;

  SGA_API DataLayout& extend(DataType dt);

  SGA_API bool operator ==(const DataLayout& other);
  SGA_API bool operator !=(const DataLayout& other);
  SGA_API operator bool();
};

} // namespace sga

#endif // __SGA_LAYOUT_HPP__
