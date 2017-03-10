#ifndef __LAYOUT_HPP__
#define __LAYOUT_HPP__

#include <sga/layout.hpp>

namespace sga{

std::string getDataTypeGLSLName(DataType);
unsigned int getDataTypeSize(DataType dt);
unsigned int getDataTypeGLSLstd140Alignment(DataType dt);
size_t getAnnotatedDataLayoutSize(const std::vector<std::pair<DataType, std::string>>&);
size_t getAnnotatedDataLayoutUBOSize(const std::vector<std::pair<DataType, std::string>>&);
} // namespace sga

#endif // __LAYOUT_HPP__
