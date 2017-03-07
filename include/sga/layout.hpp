#ifndef __SGA_LAYOUT_HPP__
#define __SGA_LAYOUT_HPP__

#include <memory>
#include <vector>
#include <initializer_list>
#include <numeric>

namespace sga{

enum class DataType{
  SByte, SByte2, SByte3, SByte4,
  UByte, UByte2, UByte3, UByte4,
  SInt, SInt2, SInt3, SInt4,
  UInt, UInt2, UInt3, UInt4,
  Float, Float2, Float3, Float4
};

constexpr std::pair<DataType, size_t> map_data_sizes[] = {
    {DataType::SByte, 1},
    {DataType::SByte2, 2},
    {DataType::SByte3, 3},
    {DataType::SByte4, 4},
    {DataType::UByte, 1},
    {DataType::UByte2, 2},
    {DataType::UByte3, 3},
    {DataType::UByte4, 4},
    {DataType::SInt, 4},
    {DataType::SInt2, 8},
    {DataType::SInt3, 12},
    {DataType::SInt4, 16},
    {DataType::UInt, 4},
    {DataType::UInt2, 8},
    {DataType::UInt3, 12},
    {DataType::UInt4, 16},
    {DataType::Float, 4},
    {DataType::Float2, 8},
    {DataType::Float3, 12},
    {DataType::Float4, 16},
};
constexpr auto map_data_sizes_N = sizeof map_data_sizes/sizeof map_data_sizes[0];

constexpr size_t getDataTypeSize(DataType t, int range = map_data_sizes_N){{
    return
      (range == 0) ? throw std::logic_error("DataType unmapped."):
      (map_data_sizes[range - 1].first == t) ? map_data_sizes[range - 1].second:
      getDataTypeSize(t, range - 1);
  };
}

constexpr size_t getTotalDataTypeSize(DataType t){
  return getDataTypeSize(t);
}
template<typename... Ts>
constexpr size_t getTotalDataTypeSize(DataType arg1, Ts... args){
  return getDataTypeSize(arg1) + getTotalDataTypeSize(args...);
}

class DataLayout{
public:
  ~DataLayout();
  DataLayout() {}
  DataLayout(std::initializer_list<DataType> types);
  size_t size() const{
    // TODO: Don't use accumulate in order to strip <numeric> header dependency.
    return std::accumulate(
      layout.begin(), layout.end(),
      (size_t)0,
      [](size_t a, DataType b){
        return a + getDataTypeSize(b);
      });
  }
  std::vector<DataType> layout;

  bool operator ==(const DataLayout& other){
    if(layout.size() != other.layout.size()) return false;
    for(unsigned int i = 0; i < layout.size(); i++)
      if(layout[i] != other.layout[i])
        return false;
    return true;
  }
  bool operator !=(const DataLayout& other){
    return !(*this == other);
  }
  operator bool(){
    return layout.size() > 0;
  }
};

} // namespace sga

#endif // __SGA_LAYOUT_HPP__
