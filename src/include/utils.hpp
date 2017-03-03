#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <functional>

#include <vkhlf/vkhlf.h>

namespace sga{

void executeOneTimeCommands(std::function<void(std::shared_ptr<vkhlf::CommandBuffer>)>);

} // namespace sga

#endif // __UTILS_HPP__
