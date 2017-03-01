#include <iostream>
#include <cassert>

#include <vulkan/vulkan.h>

#include <sga/utils.hpp>

#include "version.hpp"

namespace sga{
void greet(){
  std::cout << "libSGA " << LIBSGA_VERSION_LONG << std::endl;

  uint32_t tmp1;
  VkResult test = vkEnumerateInstanceLayerProperties(&tmp1, NULL);
  assert(test == VK_SUCCESS);
}
}
