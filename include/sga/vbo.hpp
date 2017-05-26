#ifndef __SGA_VBO_HPP__
#define __SGA_VBO_HPP__

#include <vector>
#include <initializer_list>
#include <iostream>

#include "config.hpp"
#include "layout.hpp"

namespace sga{

class VBOImpl;

class VBO{
public:
  SGA_API VBO(DataLayout layout, unsigned int n);
  SGA_API ~VBO();

  SGA_API DataLayout getLayout() const;
  SGA_API size_t getDataSize() const;
  SGA_API unsigned int getSize() const;
  
  // Non-Interleaving write.
  template <typename T>
  SGA_API void write(std::vector<T> data){
    putData((uint8_t*)data.data(), data.size(), sizeof(T));
  }

  // Interleaving write.
  template <typename... Ts>
  SGA_API void write(std::tuple<std::vector<Ts>...> data){
    std::cout << "interleaved write UNIMPLEMENTED!!!" << std::endl;;
  }

  // At least 2 args.
  template <typename T, typename... Ts>
  SGA_API void write(T arg1, Ts... args){
    write(std::tuple<T, Ts...>(arg1, args...));
  }

  friend class Pipeline;
  friend class FullQuadPipeline;
private:
  class Impl;
  pimpl_unique_ptr<Impl> impl;

  SGA_API void putData(uint8_t* pData, size_t n);
  SGA_API void putData(uint8_t* pData, size_t n_elem, size_t elem_size);
};

} // namespace sga

#endif // __SGA_VBO_HPP__
