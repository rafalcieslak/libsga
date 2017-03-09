#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <sga/exceptions.hpp>

#include <functional>

#include <vkhlf/vkhlf.h>

namespace sga{

void executeOneTimeCommands(std::function<void(std::shared_ptr<vkhlf::CommandBuffer>)>);
void ensurePhysicalDeviceSurfaceSupport(std::shared_ptr<vkhlf::Surface> surface);

void out_msg(std::string text, std::string terminator = "\n");
void out_dbg(std::string text, std::string terminator = "\n");

void realizeException(SGAException* except);

} // namespace sga

#endif // __UTILS_HPP__
