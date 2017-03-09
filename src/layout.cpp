#include "layout.hpp"

#include <map>

namespace sga{

std::string getDataTypeGLSL(DataType dt){
  // TODO: Unordered map?
  static std::map<DataType, std::string> map = {
    {DataType::Int, "int"},
    {DataType::UInt, "uint"},
    {DataType::Float, "float"},
    {DataType::Float2, "vec2"},
    {DataType::Float3, "vec3"},
    {DataType::Float4, "vec4"},
    {DataType::Double, "double"},
  };
  return map[dt];
}

size_t getAnnotatedDataLayoutSize(const std::vector<std::pair<DataType, std::string>>& layout){
  size_t total = 0;
  for(const auto& p : layout)
    total += getDataTypeSize(p.first);
  return total;
}

} // namespace sga
