#include "scheduler.hpp"

#include <iostream>
#include <functional>

#include "global.hpp"

namespace sga{

// There is a lot of tests for this debug variable, but branch prediction
// optimizes it out.
static bool trace_scheduler = false;

std::shared_ptr<vkhlf::Queue> Scheduler::queue = nullptr;

std::vector<std::shared_ptr<void>> Scheduler::references_till_next_sync;
std::shared_ptr<vkhlf::Semaphore> Scheduler::last_chain_semaphore = nullptr;

std::shared_ptr<vkhlf::CommandBuffer> Scheduler::current_command_buffer = nullptr;

void Scheduler::initQueue(unsigned int queueFamilyIndex){
  queue = global::device->getQueue(queueFamilyIndex, 0);
}
void Scheduler::releaseQueue(){
  queue = nullptr;
}

void Scheduler::submitSynced(const char* annotation, std::shared_ptr<vkhlf::CommandBuffer> cmdBuffer){
  sync();
  if(trace_scheduler) std::cout << "[SCHEDULER] " << annotation << " SYNCED!" << std::endl;
  auto fence = global::device->createFence(false);
  queue->submit(
    vkhlf::SubmitInfo{
      {}, {}, cmdBuffer, {} },
    fence
    );
  fence->wait(UINT64_MAX);
}

void Scheduler::buildAndSubmitSynced(const char* annotation, std::function<void (std::shared_ptr<vkhlf::CommandBuffer>)> record_commands){
  auto commandBuffer = global::commandPool->allocateCommandBuffer();
  commandBuffer->begin();
  record_commands(commandBuffer);
  commandBuffer->end();
  submitSynced(annotation, commandBuffer);
}

void Scheduler::presentSynced(std::unique_ptr<vkhlf::FramebufferSwapchain> &swapchain){
  sync();
  swapchain->present(queue);
  if(trace_scheduler) std::cout << "[SCHEDULER] Presenting surface SYNCED!" << std::endl;
  queue->waitIdle();
}

void Scheduler::scheduleChained(const char *annotation, std::shared_ptr<vkhlf::CommandBuffer> cmdBuffer){
  if(!last_chain_semaphore){
    // First link in the chain.
    if(trace_scheduler) std::cout << "[SCHEDULER] " << annotation << " CHAIN START!" << std::endl;
    auto new_semaphore = global::device->createSemaphore();
    queue->submit( vkhlf::SubmitInfo{
        {}, {}, cmdBuffer, {new_semaphore} }, nullptr
      );
    last_chain_semaphore = new_semaphore;
  }else{
    if(trace_scheduler) std::cout << "[SCHEDULER] " << annotation << " CHAIN!" << std::endl;
    // Subsequent link in the chain.
    auto new_semaphore = global::device->createSemaphore();
    queue->submit( vkhlf::SubmitInfo{
        {last_chain_semaphore}, {vk::PipelineStageFlagBits::eAllGraphics}, cmdBuffer, {new_semaphore} }, nullptr
      );
    last_chain_semaphore = new_semaphore;
  }
}

void Scheduler::sync(){
  finalizeChainedCmdBuffer();

  if(last_chain_semaphore){
    if(trace_scheduler) std::cout << "[SCHEDULER] CHAIN SYNC!" << std::endl;
    // Make a fence that waits for the semaphore and wait for the fence.
    // Twisted, but nicely compatible with other chain links.
    auto fence = global::device->createFence(false);
    queue->submit( vkhlf::SubmitInfo{
        {last_chain_semaphore}, {vk::PipelineStageFlagBits::eAllCommands}, nullptr, {} }, fence
      );
    fence->wait(UINT64_MAX);
    last_chain_semaphore = nullptr;
  }
  references_till_next_sync.clear();
}

void Scheduler::borrowChainableCmdBuffer(const char* annotation, std::function<void(std::shared_ptr<vkhlf::CommandBuffer>)> action){
  if(!current_command_buffer){
    if(trace_scheduler) std::cout << "[SCHEDULER] " << annotation << " CHAIN buffer first" << std::endl;
    current_command_buffer = global::commandPool->allocateCommandBuffer();
    current_command_buffer->begin();
  }else{
    if(trace_scheduler) std::cout << "[SCHEDULER] " << annotation << " CHAIN subsequent" << std::endl;
  }
  action(current_command_buffer);
}

void Scheduler::finalizeChainedCmdBuffer(){
  if(current_command_buffer){
    current_command_buffer->end();
    scheduleChained("Chained draw command buffer", current_command_buffer);
    current_command_buffer = nullptr;
  }
}

void Scheduler::appendChainedResource(std::shared_ptr<void> r){
  references_till_next_sync.push_back(r);
}

} // namespace sga
