#ifndef __GLOBAL_HPP__
#define __GLOBAL_HPP__

#include <vkhlf/vkhlf.h>

#include <sga/utils.hpp>

namespace sga{

class impl_global{
public:
  //TODO: error strategy is very inconsistent, this settning is ignored!
  static ErrorStrategy error_strategy;
  static VerbosityLevel verbosity;
  static bool initialized;

  static std::shared_ptr<vkhlf::Instance> instance;
  static std::shared_ptr<vkhlf::PhysicalDevice> physicalDevice;
  static std::shared_ptr<vkhlf::Device> device;
  
  // We keep a reference to the debug report callback so that it stays alive with the instance!
  static std::shared_ptr<vkhlf::DebugReportCallback> debugReportCallback;
};

} // namespace sga

#endif // __GLOBAL_HPP__
