#include <iostream>

#include "version.hpp"

namespace sga{
void greet(){
  std::cout << "libSGA " << LIBSGA_VERSION_LONG << std::endl;
}
}

int main(){
  sga::greet();
}
