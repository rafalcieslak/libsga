#include "global.hpp"

namespace sga{

VerbosityLevel global::verbosity;
ErrorStrategy global::error_strategy;
bool global::initialized = false;

std::shared_ptr<vkhlf::Instance> global::instance;
std::shared_ptr<vkhlf::PhysicalDevice> global::physicalDevice;
std::shared_ptr<vkhlf::Device> global::device;
std::shared_ptr<vkhlf::Queue> global::queue;
std::shared_ptr<vkhlf::CommandPool> global::commandPool;

unsigned int global::queueFamilyIndex;

std::shared_ptr<vkhlf::DebugReportCallback> global::debugReportCallback;

} // namespace sga
