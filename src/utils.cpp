#include <iostream>
#include <cassert>

#include <vulkan/vulkan.h>

#include <sga/utils.hpp>

#include "version.hpp"
#include "global.hpp"

namespace sga{
void info(){
  std::cout << "libSGA " << LIBSGA_VERSION_LONG;

  if(!impl_global::initialized){
    std::cout << ", not initialized." << std::endl;
  }else{
    std::cout << ", initialized." << std::endl;
    // TODO: Print out some useful information about selected device, status etc.
  }
  
  uint32_t tmp1;
  VkResult test = vkEnumerateInstanceLayerProperties(&tmp1, NULL);
  assert(test == VK_SUCCESS);
}

void init(VerbosityLevel verbosity, ErrorStrategy strategy){
  if(impl_global::initialized){
    std::cout << "SGA was already initialized!" << std::endl;
    return;
  }
  
  impl_global::verbosity = verbosity;
  impl_global::error_strategy = strategy;

  // create vulkan instance, set up validation layers and optional debug
  // features, pick a physical device, set up the logical device, querry
  // available memory features, prepare command queue families.
  
  impl_global::initialized = true;
  std::cout << "SGA initialized successfully." << std::endl;
}
}
