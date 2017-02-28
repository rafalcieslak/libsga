#include <iostream>

#include <sga/utils.hpp>

#include "version.hpp"

namespace sga{
void greet(){
  std::cout << "libSGA " << LIBSGA_VERSION_LONG << std::endl;
}
}
