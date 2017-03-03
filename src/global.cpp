#include "global.hpp"

namespace sga{

VerbosityLevel impl_global::verbosity;
ErrorStrategy impl_global::error_strategy;
bool impl_global::initialized = false;

std::shared_ptr<vkhlf::Instance> impl_global::instance;
std::shared_ptr<vkhlf::PhysicalDevice> impl_global::physicalDevice;
std::shared_ptr<vkhlf::Device> impl_global::device;
std::shared_ptr<vkhlf::Queue> impl_global::queue;
std::shared_ptr<vkhlf::CommandPool> impl_global::commandPool;

unsigned int impl_global::queueFamilyIndex;

std::shared_ptr<vkhlf::DebugReportCallback> impl_global::debugReportCallback;

} // namespace sga
