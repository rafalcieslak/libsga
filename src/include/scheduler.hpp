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

  // Schedules a command buffer to execute. There is no explicit control as to
  // when it will execute, but it's guaranteed to run only after the previous chained buffer has finished.
  static void scheduleChained(const char* annotation, std::shared_ptr<vkhlf::CommandBuffer>);
  // The function action may record commands to the buffer it receives as an
  // argument. These commands will be grouped together with other subsequent
  // uses of borrowChainableCmdBuffer. This way it is possible to merge a lot of
  // draw commands into a single buffer. There is no control over the time when
  // the completed buffer will be executed, but it will finish before the next
  // synced action.
  static void borrowChainableCmdBuffer(const char* annotation, std::function<void(std::shared_ptr<vkhlf::CommandBuffer>)> action);

  // Stores a reference to a resource as long as the current chain is executing.
  static void appendChainedResource(std::shared_ptr<void>);

private:
  static void finalizeChainedCmdBuffer();

  // Waits until all scheduled actions are finished.
  static void sync();

  // This is the main command queue used by SGA! No other classes access
  // it. This way the Scheduler has full control over synchronizing all
  // commands.
  static std::shared_ptr<vkhlf::Queue> queue;

  // This container keeps references for objects used within the current chain.
  // They will be released on the next chain sync.
  static std::vector<std::shared_ptr<void>> references_till_next_sync;

  static std::shared_ptr<vkhlf::Semaphore> last_chain_semaphore;

  static std::shared_ptr<vkhlf::CommandBuffer> current_command_buffer;
};

} // namespace sga

#endif // __SCHEDULER_HPP__
