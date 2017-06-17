#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <sga/exceptions.hpp>
#include <sga/image.hpp>

#include <functional>

#include <vkhlf/vkhlf.h>

namespace sga{

void executeOneTimeCommands(std::function<void(std::shared_ptr<vkhlf::CommandBuffer>)>);
void ensurePhysicalDeviceSurfaceSupport(std::shared_ptr<vkhlf::Surface> surface);

void out_msg(std::string text, std::string terminator = "\n");
void out_dbg(std::string text, std::string terminator = "\n");

void realizeException(SGAException* except);

size_t align(size_t base, unsigned int alignment);

bool isVariableNameValid(std::string);

std::vector<std::string> SplitString(std::string str, std::string delimiter, bool skip_empty);


// TODO: Move all above to inside Utils
class Utils{
public:
  static vk::ClearColorValue imageClearColorToVkClearColorValue(ImageClearColor cc);
  static std::string readEntireFile(std::string path);
};

} // namespace sga

#endif // __UTILS_HPP__
