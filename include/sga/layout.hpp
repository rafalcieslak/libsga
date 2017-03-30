#ifndef __SGA_LAYOUT_HPP__
#define __SGA_LAYOUT_HPP__

#include <memory>
#include <vector>
#include <initializer_list>

namespace sga{

enum class DataType{
  Int,
  UInt,
  Float, Float2, Float3, Float4,
  Double,
  Mat3, Mat4,
};

class DataLayout{
public:
  ~DataLayout();
  DataLayout() {}
  DataLayout(std::initializer_list<DataType> types);
  size_t byteSize() const;
  
  // TODO: This should be hidden from API.
  size_t UBOsize() const;
  
  std::vector<DataType> layout;

  DataLayout& extend(DataType dt);

  bool operator ==(const DataLayout& other);
  bool operator !=(const DataLayout& other);
  operator bool();
};

} // namespace sga

#endif // __SGA_LAYOUT_HPP__
