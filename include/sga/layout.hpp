#ifndef __SGA_LAYOUT_HPP__
#define __SGA_LAYOUT_HPP__

#include <memory>
#include <vector>
#include <initializer_list>
#include <numeric>

namespace sga{

enum class DataType{
  Int,
  UInt,
  Float, Float2, Float3, Float4,
  Double
};

constexpr std::pair<DataType, size_t> map_data_sizes[] = {
    {DataType::Int, 4},
    {DataType::UInt, 4},
    {DataType::Float, 4},
    {DataType::Float2, 8},
    {DataType::Float3, 12},
    {DataType::Float4, 16},
    {DataType::Double, 8},
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

  DataLayout& extend(DataType dt){
    layout.push_back(dt); return *this;
  }

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
