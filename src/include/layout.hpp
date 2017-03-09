#ifndef __LAYOUT_HPP__
#define __LAYOUT_HPP__

#include <sga/layout.hpp>

namespace sga{

std::string getDataTypeGLSL(DataType);
size_t getAnnotatedDataLayoutSize(const std::vector<std::pair<DataType, std::string>>&);

} // namespace sga

#endif // __LAYOUT_HPP__
