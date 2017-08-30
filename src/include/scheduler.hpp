#ifndef __SCHEDULER_HPP__
#define __SCHEDULER_HPP__

#include <vkhlf/vkhlf.h>

#include <sga/utils.hpp>

namespace sga{

class Scheduler{
public:
  static void initQueue(unsigned int queueFamilyIndex);
  static void releaseQueue();

  // Ensures the provided command buffer actions are performed NOW. This means
  // CPU will sleep until GPU is done with previously scheduled actions, and
  // will then also wait for this one to be completed.
  static void submitSynced(const char* annotation, std::shared_ptr<vkhlf::CommandBuffer>);
  static void buildAndSubmitSynced(const char* annotation, std::function<void(std::shared_ptr<vkhlf::CommandBuffer>)> record_commands);

  // Convenience wrapper for calling FramebufferSwapchain::present synchronized.
  static void presentSynced(std::unique_ptr<vkhlf::FramebufferSwapchain>&);
  
private:
  static std::shared_ptr<vkhlf::Queue> queue;

  // Waits until all scheduled actions are finished.
  static void sync();
};

} // namespace sga

#endif // __SCHEDULER_HPP__
