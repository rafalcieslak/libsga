#ifndef __SGA_VBO_HPP__
#define __SGA_VBO_HPP__

#include <memory>
#include <vector>
#include <initializer_list>
#include <iostream>
#include "layout.hpp"

namespace sga{

class VBOImpl;

class VBO{
public:
  ~VBO();

  DataLayout getLayout() const;
  size_t getDataSize() const;
  
  // Non-Interleaving write.
  template <typename T>
  void write(std::vector<T> data){
    putData((uint8_t*)data.data(), data.size(), sizeof(T));
  }

  // Interleaving write.
  template <typename... Ts>
  void write(std::tuple<std::vector<Ts>...> data){
    std::cout << "interleaved write UNIMPLEMENTED!!!" << std::endl;;
  }

  // At least 2 args.
  template <typename T, typename... Ts>
  void write(T arg1, Ts... args){
    write(std::tuple<T, Ts...>(arg1, args...));
  }

  static std::shared_ptr<VBO> create(DataLayout layout, unsigned int size) {
    return std::shared_ptr<VBO>(new VBO(layout, size));
  }
  
  friend class Pipeline;
private:
  VBO(DataLayout layout, unsigned int n);
  class Impl;
  std::unique_ptr<Impl> impl;

  void putData(uint8_t* pData, size_t n);
  void putData(uint8_t* pData, size_t n_elem, size_t elem_size);
};

} // namespace sga

#endif // __SGA_VBO_HPP__
