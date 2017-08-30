#include "scheduler.hpp"

#include <iostream>

#include "global.hpp"

namespace sga{

bool trace_scheduler = true;

std::shared_ptr<vkhlf::Queue> Scheduler::queue;

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
      {},{}, cmdBuffer, {} },
    fence
    );
  fence->wait(UINT64_MAX);
}

void Scheduler::presentSynced(std::unique_ptr<vkhlf::FramebufferSwapchain> &swapchain){
  sync();
  swapchain->present(queue);
  if(trace_scheduler) std::cout << "[SCHEDULER] Presenting surface SYNCED!" << std::endl;
  queue->waitIdle();
}

void Scheduler::sync(){

}

void Scheduler::buildAndSubmitSynced(const char* annotation, std::function<void (std::shared_ptr<vkhlf::CommandBuffer>)> record_commands){
  auto commandBuffer = global::commandPool->allocateCommandBuffer();
  commandBuffer->begin();
  record_commands(commandBuffer);
  commandBuffer->end();
  submitSynced(annotation, commandBuffer);
}

} // namespace sga
