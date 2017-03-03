#include "global.hpp"

namespace sga{

VerbosityLevel impl_global::verbosity;
ErrorStrategy impl_global::error_strategy;
bool impl_global::initialized = false;

std::shared_ptr<vkhlf::Instance> impl_global::instance;
std::shared_ptr<vkhlf::PhysicalDevice> impl_global::physicalDevice;

std::shared_ptr<vkhlf::DebugReportCallback> impl_global::debugReportCallback;

} // namespace sga
