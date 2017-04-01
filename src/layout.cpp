#include "layout.hpp"

#include <map>
#include <numeric>

#include "utils.hpp"

namespace sga{

unsigned int getDataTypeSize(DataType dt){
  // TODO: Unordered map?
  static std::map<DataType, unsigned int> map = {
    {DataType::SInt, 4},
    {DataType::UInt, 4},
    {DataType::SInt2, 8},
    {DataType::UInt2, 8},
    {DataType::SInt3, 12},
    {DataType::UInt3, 12},
    {DataType::SInt4, 16},
    {DataType::UInt4, 16},
    {DataType::Float, 4},
    {DataType::Float2, 8},
    {DataType::Float3, 12},
    {DataType::Float4, 16},
    {DataType::Double, 8},
    {DataType::Mat3, 48},
    {DataType::Mat4, 64},
  };
  return map[dt];
}

std::string getDataTypeGLSLName(DataType dt){
  // TODO: Unordered map?
  static std::map<DataType, std::string> map = {
    {DataType::SInt, "int"},
    {DataType::UInt, "uint"},
    {DataType::SInt2, "ivec2"},
    {DataType::UInt2, "uvec2"},
    {DataType::SInt3, "ivec3"},
    {DataType::UInt3, "uvec3"},
    {DataType::SInt4, "ivec4"},
    {DataType::UInt4, "uvec4"},
    {DataType::Float, "float"},
    {DataType::Float2, "vec2"},
    {DataType::Float3, "vec3"},
    {DataType::Float4, "vec4"},
    {DataType::Double, "double"},
    {DataType::Mat3, "mat3"},
    {DataType::Mat4, "mat4"},
  };
  return map[dt];
}

unsigned int getDataTypeGLSLstd140Alignment(DataType dt){
  // TODO: Unordered map?
  static std::map<DataType, unsigned int> map = {
    {DataType::SInt,  4},
    {DataType::UInt,  4},
    {DataType::SInt2, 8},
    {DataType::UInt2, 8},
    {DataType::SInt3, 16},
    {DataType::UInt3, 16},
    {DataType::SInt4, 16},
    {DataType::UInt4, 16},
    {DataType::Float, 4},
    {DataType::Float2, 8},
    {DataType::Float3, 16},
    {DataType::Float4, 16},
    {DataType::Double, 8},
    {DataType::Mat3, 16},
    {DataType::Mat4, 16},
  };
  return map[dt];
}

size_t getAnnotatedDataLayoutSize(const std::vector<std::pair<DataType, std::string>>& layout){
  size_t total = 0;
  for(const auto& p : layout)
    total += getDataTypeSize(p.first);
  return total;
}

size_t getAnnotatedDataLayoutUBOSize(const std::vector<std::pair<DataType, std::string>>& layout){
  size_t total = 0;
  for(const auto& p : layout){
    total = align(total, getDataTypeGLSLstd140Alignment(p.first));
    total += getDataTypeSize(p.first);
  }
  return total;
}

size_t DataLayout::byteSize() const{
  return std::accumulate(
    layout.begin(), layout.end(),
    (size_t)0,
    [](size_t a, DataType b){
      return a + getDataTypeSize(b);
    });
}

size_t DataLayout::UBOsize() const{
  return std::accumulate(
    layout.begin(), layout.end(),
    (size_t)0,
    [](size_t a, DataType b){
      return align(a, getDataTypeGLSLstd140Alignment(b)) + getDataTypeSize(b);
    });
}

DataLayout& DataLayout::extend(DataType dt){
  layout.push_back(dt); return *this;
}

bool DataLayout::operator ==(const DataLayout& other){
  if(layout.size() != other.layout.size()) return false;
  for(unsigned int i = 0; i < layout.size(); i++)
    if(layout[i] != other.layout[i])
      return false;
  return true;
}
bool DataLayout::operator !=(const DataLayout& other){
  return !(*this == other);
}
DataLayout::operator bool(){
    return layout.size() > 0;
}


} // namespace sga
