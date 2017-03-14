#ifndef __SGA_CONFIG_HPP__
#define __SGA_CONFIG_HPP__

#define LIBSGA_VERSION_SHORT "v0.0-debug"
#define LIBSGA_VERSION_LONG  "v0.0-58-g9054a93-dirty-debug"

#ifdef SGA_USE_GLM
#include <glm/glm.hpp>
#endif

#include <memory>
#if __cpp_lib_experimental_propagate_const >= 201505
  #include <experimental/propagate_const>
  template <typename T>
  using pimpl_unique_ptr = std::experimental::propagate_const<std::unique_ptr<T>>;
#else
  template <typename T>
  using pimpl_unique_ptr = std::unique_ptr<T>;
#endif

#endif // __SGA_CONFIG_HPP__
