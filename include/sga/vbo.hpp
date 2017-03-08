#ifndef __SGA_VBO_HPP__
#define __SGA_VBO_HPP__

#include <memory>
#include <vector>
#include <initializer_list>
#include <iostream>
#include "layout.hpp"

namespace sga{

class VBOImpl;

class VBOBase{
public:
  VBOBase(unsigned int datasize, unsigned int n);
  ~VBOBase();

  virtual DataLayout getLayout() const = 0;
  virtual size_t getDataSize() const = 0;
  
  friend class Pipeline;
protected:
  class Impl;
  std::unique_ptr<Impl> impl;

  unsigned int size;
  void putData(uint8_t* pData, size_t n);
};

template<DataType... Layout>
class VBO : public VBOBase{
public:
  // Non-Interleaving write.
  template <typename T>
  void write(std::vector<T> data){
    static_assert(sizeof(T) == getTotalDataTypeSize(Layout...), "VBO Layout does not match written data layout!");
    if(data.size() != size){
      std::cout << "Error: VBO size does not match written data size!" << std::endl;
    }else{
      putData((uint8_t*)data.data(), data.size() * sizeof(T));
    }
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

  DataLayout getLayout() const override{
    return DataLayout({Layout...});
  }
  
  size_t getDataSize() const override{
    return getTotalDataTypeSize(Layout...);
  }

  static std::shared_ptr<VBO<Layout...>> create(unsigned int size) {
    return std::shared_ptr<VBO<Layout...>>(new VBO<Layout...>(size));
  }
private:
  VBO(unsigned int n) : VBOBase(getDataSize(), n) {}
};

} // namespace sga

#endif // __SGA_VBO_HPP__
