#include <iostream>
#include <cassert>
#include <vector>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <vkhlf/vkhlf.h>

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
}


static VkBool32 debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char* pMessage, void*)
  {
    switch (flags)
    {
      case VK_DEBUG_REPORT_INFORMATION_BIT_EXT :
        std::cout << "INFORMATION: " << pMessage << std::endl;
        return VK_FALSE;
      case VK_DEBUG_REPORT_WARNING_BIT_EXT :
        std::cerr << "WARNING: ";
        break;
      case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
        std::cerr << "PERFORMANCE WARNING: ";
        break;
      case VK_DEBUG_REPORT_ERROR_BIT_EXT:
        std::cerr << "ERROR: ";
        break;
      case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
        std::cout << "DEBUG: " << pMessage << std::endl;
        return VK_FALSE;
      default:
        std::cerr << "unknown flag (" << flags << "): ";
        break;
    }
    if(pMessage)
      std::cerr << pMessage << std::endl;
    return VK_TRUE;
}

static std::map<vk::PhysicalDeviceType, int> deviceTypeScores{
  {vk::PhysicalDeviceType::eDiscreteGpu, 1000},
  {vk::PhysicalDeviceType::eIntegratedGpu, 200},
  {vk::PhysicalDeviceType::eCpu, 50},
  {vk::PhysicalDeviceType::eVirtualGpu, 100},
  {vk::PhysicalDeviceType::eOther, 0},
};

static int rateDevice(std::shared_ptr<vkhlf::PhysicalDevice> dev){
  int score = 0;
  auto properties = dev->getProperties();
  // auto features = dev->getFeatures();

  // Geometry shader is not really required, but it's a good example of how rateDevice may inspect devices.
  // if(!features.geometryShader) return 0;
  
  score += deviceTypeScores[properties.deviceType];
  return score;
}

static bool pickPhysicalDevice(){
  unsigned int device_count = impl_global::instance->getPhysicalDeviceCount();
  std::cout << "Found " << device_count << " physical devices." << std::endl;
  if(device_count == 0){
    std::cout << "No vulkan-supporting devices found!" << std::endl;
    return false;
  }
  std::vector<int> scores(device_count);
  for(unsigned int i = 0; i < device_count; i++){
    std::shared_ptr<vkhlf::PhysicalDevice> device = impl_global::instance->getPhysicalDevice(i);
    scores[i] = rateDevice(device);
  }

  auto argmax_it = std::max_element(scores.begin(), scores.end());
  if(*argmax_it == 0){
    std::cout << "Vulkan devices found, but none of them supports features required for libSGA to operate" << std::endl;
    return false;
  }
  unsigned int best_device = argmax_it - scores.begin();
  auto pDev = impl_global::instance->getPhysicalDevice(best_device);

  std::string devName = pDev->getProperties().deviceName;
  std::cout << "Picked device: " << devName << std::endl;

  impl_global::physicalDevice = pDev;
  return true;
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

  std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();
  std::cout << "Found " << layerProperties.size() << " instance layers." << std::endl;

  std::vector<std::string> requiredExtensions, requiredLayers;
  std::vector<std::string> desiredExtensions, desiredLayers;
  std::vector<std::string> enabledExtensions, enabledLayers;

  /*
    Enable debug validation layers when running the program via command-line.

  desiredLayers.insert( desiredLayers.end(), {
      "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_image",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_swapchain",
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_GOOGLE_unique_objects"
        });
  */

  // Check layer availability.
  for(auto& layer : requiredLayers){
    auto it = std::find_if(layerProperties.begin(), layerProperties.end(), [&](vk::LayerProperties lp){return lp.layerName == layer;});
    if(it == layerProperties.end()){
      // A required layer is not available!
      std::cout << "A required layer " << layer << " is not available!" << std::endl;
      throw std::runtime_error("LayerNotAvailable");
    }
    enabledLayers.push_back(layer);
  }
  for(auto& layer : desiredLayers){
    auto it = std::find_if(layerProperties.begin(), layerProperties.end(), [&](vk::LayerProperties lp){return lp.layerName == layer;});
    if(it == layerProperties.end()){
      // A desired layer is not available!
      std::cout << "A desired layer " << layer << " is not available, skipping." << std::endl;
      continue;
    }
    enabledLayers.push_back(layer);
  }
  
  impl_global::instance = vkhlf::Instance::create("libSGA", 1, enabledLayers, enabledExtensions);
  
  // Register validation layers debug output.
  // TODO: Only do this when verbosity is set to debug!
  vk::DebugReportFlagsEXT flags(vk::DebugReportFlagBitsEXT::eWarning |
                                vk::DebugReportFlagBitsEXT::ePerformanceWarning |
                                vk::DebugReportFlagBitsEXT::eError |
                                vk::DebugReportFlagBitsEXT::eDebug
    );
  impl_global::debugReportCallback = impl_global::instance->createDebugReportCallback(flags, &debugReportCallback);

  // Choose a physical device.
  if(!pickPhysicalDevice()){
    throw std::runtime_error("NoDeviceAvailable");
  }

  std::vector<vk::QueueFamilyProperties> props = impl_global::physicalDevice->getQueueFamilyProperties();
  std::vector<unsigned int> indices;
  for (size_t i = 0; i < props.size(); i++)
    if (props[i].queueFlags & vk::QueueFlagBits::eGraphics)
      indices.push_back(vkhlf::checked_cast<uint32_t>(i));
  if(indices.empty()){
    std::cout << "No queue families on the picked physical device support graphic ops?" << std::endl;
    throw std::runtime_error("InvalidPhysDevice");
  }
  // Pick a queue family.
  unsigned int queueFamilyIndex = indices[0];

  impl_global::device = impl_global::physicalDevice->createDevice(vkhlf::DeviceQueueCreateInfo(queueFamilyIndex, 1.0f), nullptr, enabledExtensions);

  std::cout << "Logical device created." << std::endl;
  
  impl_global::initialized = true;
  std::cout << "SGA initialized successfully." << std::endl;
  
}

void cleanup(){
  impl_global::physicalDevice = nullptr;
  impl_global::debugReportCallback = nullptr;
  if(impl_global::instance.use_count() == 1)
    std::cout << "SGA successfully deinitialized." << std::endl;
  impl_global::instance = nullptr;
  impl_global::initialized = false;
}

}
