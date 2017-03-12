#include <sga/utils.hpp>

#include <iostream>
#include <cassert>
#include <vector>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <vkhlf/vkhlf.h>
#include <GLFW/glfw3.h>

#include <sga/config.hpp>
#include "global.hpp"
#include "utils.hpp"

namespace sga{
void info(){
  std::cout << "libSGA " << LIBSGA_VERSION_LONG;

  if(!global::initialized){
    std::cout << ", not initialized." << std::endl;
  }else{
    std::cout << ", initialized." << std::endl;
    // TODO: Print out some useful information about selected device, status etc.
  }
}


static VkBool32 debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char* pMessage, void*)
  {
    VkBool32 critical = VK_FALSE;
    switch (flags)
    {
      case VK_DEBUG_REPORT_INFORMATION_BIT_EXT :
        std::cerr << "Vulkan validation layer INFORMATION: ";
        break;
      case VK_DEBUG_REPORT_WARNING_BIT_EXT :
        std::cerr << "Vulkan validation layer WARNING: ";
        break;
      case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
        std::cerr << "Vulkan validation layer PERFORMANCE WARNING: ";
        break;
      case VK_DEBUG_REPORT_ERROR_BIT_EXT:
        std::cerr << "Vulkan validation layer ERROR: ";
        critical = VK_TRUE;
        break;
      case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
        std::cout << "Vulkan validation layer DEBUG: ";
        break;
      default:
        break;
    }
    if(pMessage)
      std::cerr << pMessage << std::endl;
    return critical;
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

  // Discard devices that are not sutable for presentation
  if(!glfwGetPhysicalDevicePresentationSupport(static_cast<vk::Instance>(*global::instance), static_cast<vk::PhysicalDevice>(*dev), 0))
    return 0;

  // Geometry shader is not really required, but it's a good example of how rateDevice may inspect devices.
  // if(!features.geometryShader) return 0;
  
  score += deviceTypeScores[properties.deviceType];
  return score;
}

static bool pickPhysicalDevice(){
  unsigned int device_count = global::instance->getPhysicalDeviceCount();
  std::cout << "Found " << device_count << " physical devices." << std::endl;
  if(device_count == 0){
    std::cout << "No vulkan-supporting devices found!" << std::endl;
    return false;
  }
  std::vector<int> scores(device_count);
  for(unsigned int i = 0; i < device_count; i++){
    std::shared_ptr<vkhlf::PhysicalDevice> device = global::instance->getPhysicalDevice(i);
    scores[i] = rateDevice(device);
  }

  auto argmax_it = std::max_element(scores.begin(), scores.end());
  if(*argmax_it == 0){
    std::cout << "Vulkan devices found, but none of them supports features required for libSGA to operate" << std::endl;
    return false;
  }
  unsigned int best_device = argmax_it - scores.begin();
  auto pDev = global::instance->getPhysicalDevice(best_device);

  std::string devName = pDev->getProperties().deviceName;
  std::cout << "Picked device: " << devName << std::endl;

  global::physicalDevice = pDev;
  return true;
}

double getTime(){
  return glfwGetTime();
}

void init(VerbosityLevel verbosity, ErrorStrategy strategy){
  if(global::initialized){
    std::cout << "SGA was already initialized!" << std::endl;
    return;
  }

  if(!glfwInit()){
    std::cout << "Failed to initialize GLFW!" << std::endl;
    return;
  }
  glfwSetTime(0.0);

  global::verbosity = verbosity;
  global::error_strategy = strategy;

  // create vulkan instance, set up validation layers and optional debug
  // features, pick a physical device, set up the logical device, querry
  // available memory features, prepare command queue families.

  std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();
  std::cout << "Found " << layerProperties.size() << " instance layers." << std::endl;

  std::vector<std::string> requiredLayers, desiredLayers, enabledLayers;
  std::vector<std::string> enabledInstanceExtensions, enabledDeviceExtensions;

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

  // Gather glfw extensions.
  uint32_t count;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);
  std::copy(extensions, extensions + count, std::back_inserter(enabledInstanceExtensions));

  for(auto i : enabledInstanceExtensions){
    std::cout << i << " ";
  } std::cout << std::endl;

  enabledDeviceExtensions.push_back("VK_KHR_swapchain");
  
  // TODO: Check if enabledInstanceExtensions are available.
  
  global::instance = vkhlf::Instance::create("libSGA", 1, enabledLayers, enabledInstanceExtensions);
  
  // Register validation layers debug output.
  // TODO: Only do this when verbosity is set to debug!
  vk::DebugReportFlagsEXT flags(vk::DebugReportFlagBitsEXT::eWarning |
                                vk::DebugReportFlagBitsEXT::ePerformanceWarning |
                                vk::DebugReportFlagBitsEXT::eError |
                                vk::DebugReportFlagBitsEXT::eDebug
    );
  global::debugReportCallback = global::instance->createDebugReportCallback(flags, &debugReportCallback);

  // Choose a physical device.
  if(!pickPhysicalDevice()){
    throw std::runtime_error("NoDeviceAvailable");
  }

  std::vector<vk::QueueFamilyProperties> props = global::physicalDevice->getQueueFamilyProperties();
  std::vector<unsigned int> indices;
  for (size_t i = 0; i < props.size(); i++)
    if (props[i].queueFlags & vk::QueueFlagBits::eGraphics)
      indices.push_back(vkhlf::checked_cast<uint32_t>(i));
  if(indices.empty()){
    std::cout << "No queue families on the picked physical device support graphic ops?" << std::endl;
    throw std::runtime_error("InvalidPhysDevice");
  }
  // Pick a queue family.
  global::queueFamilyIndex = indices[0];

  global::device = global::physicalDevice->createDevice(vkhlf::DeviceQueueCreateInfo(global::queueFamilyIndex, 1.0f), nullptr, enabledDeviceExtensions);
  std::cout << "Logical device created." << std::endl;

  global::queue = global::device->getQueue(global::queueFamilyIndex, 0);

  // TODO: Prepare memory allocators??
  // eg.
  // m_deviceMemoryAllocatorImage.reset(new vkhlf::DeviceMemoryAllocator(getDevice(), 128 * 1024, nullptr));

  global::commandPool = global::device->createCommandPool(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, global::queueFamilyIndex);
  
  global::initialized = true;
  std::cout << "SGA initialized successfully." << std::endl;
  
}

void terminate(){
  if(!global::initialized)
    return;
  std::cout << "Terminate start." << std::endl;
  global::physicalDevice = nullptr;
  global::device = nullptr;
  global::queue = nullptr;
  global::commandPool = nullptr;
  global::debugReportCallback = nullptr;
  
  if(global::instance.use_count() == 1){
    std::cout << "SGA successfully terminated." << std::endl;
  }else{
    std::cout << "Dead reference to global instance!" << std::endl;
    std::cout << "Could it be that the user program still keeps some live SGA objects?" << std::endl;
  }
  global::instance = nullptr;

  glfwTerminate();
  
  global::initialized = false;
}

void executeOneTimeCommands(std::function<void(std::shared_ptr<vkhlf::CommandBuffer>)> record_commands){
  auto commandBuffer = global::commandPool->allocateCommandBuffer();
  commandBuffer->begin();

  record_commands(commandBuffer);

  commandBuffer->end();
  vkhlf::submitAndWait(global::queue, commandBuffer);
}

void ensurePhysicalDeviceSurfaceSupport(std::shared_ptr<vkhlf::Surface> surface){

  typedef VkResult (*vkGetPhysicalDeviceSurfaceSupportKHR_funtype)(
    VkPhysicalDevice,
    uint32_t,
    VkSurfaceKHR,
    VkBool32*);
  // Satisfy validation layers by checking whether the physical device supports this sufrace.
  // This is pretty much bullshit, because we only choose physical devices that support presentation.
  // However, validation layer insists that not checking per-surface is a STRICT ERROR, and refuses
  // to continue the application. See https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/818
  vkGetPhysicalDeviceSurfaceSupportKHR_funtype gpdsshkr = (vkGetPhysicalDeviceSurfaceSupportKHR_funtype)global::instance->getProcAddress("vkGetPhysicalDeviceSurfaceSupportKHR");
  VkBool32 surfaceSupported;
  gpdsshkr((vk::PhysicalDevice)*global::physicalDevice, global::queueFamilyIndex, (vk::SurfaceKHR)*surface, &surfaceSupported);
  if(!surfaceSupported){
    std::cout << "Internal error: Created sufrace is not supported by the physical device." << std::endl;
    throw std::runtime_error("SurfaceNotSupported");
  }
}

void out_msg(std::string text, std::string terminator){
  if(global::verbosity < VerbosityLevel::Verbose) return;
  std::cerr << "SGA: " << text << terminator;
  std::cerr.flush();
}
void out_dbg(std::string text, std::string terminator){
  if(global::verbosity < VerbosityLevel::Debug) return;
  std::cerr << "SGA DEBUG: " << text << terminator;
  std::cerr.flush();
}

void SGAException::raise(){
  if(global::error_strategy == ErrorStrategy::Message ||
     global::error_strategy == ErrorStrategy::MessageThrow ||
     global::error_strategy == ErrorStrategy::MessageAbort){
    std::cerr << "SGA ERROR: " << name << std::endl;
    std::cerr << info << std::endl;
    if(desc != "") std::cerr << desc << std::endl;
  }
  if(global::error_strategy == ErrorStrategy::Abort ||
     global::error_strategy == ErrorStrategy::MessageAbort){
    abort();
  }
  if(global::error_strategy == ErrorStrategy::Throw ||
     global::error_strategy == ErrorStrategy::MessageThrow){
    this->raise_this();
  }
}

size_t align(size_t base, unsigned int alignment){
  if(base % alignment == 0) return base;
  size_t low = base / alignment;
  return (low+1) * alignment;
}

} // namespace sga
