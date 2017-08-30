#ifndef __SCHEDULER_HPP__
#define __SCHEDULER_HPP__

#include <vkhlf/vkhlf.h>

#include <sga/utils.hpp>

namespace sga{

class Scheduler{
public:
  static void initQueue(unsigned int queueFamilyIndex);
  static void releaseQueue();
  
  static void submitAndSync(const char* annotation, std::shared_ptr<vkhlf::CommandBuffer>);
  static void present(std::unique_ptr<vkhlf::FramebufferSwapchain>&);
  static void fullSync();
  
  static void buildSubmitAndSync(const char* annotation, std::function<void(std::shared_ptr<vkhlf::CommandBuffer>)> record_commands);
private:
  static std::shared_ptr<vkhlf::Queue> queue;
};

} // namespace sga

#endif // __SCHEDULER_HPP__
