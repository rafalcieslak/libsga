#include "scheduler.hpp"

#include <iostream>

#include "global.hpp"

namespace sga{

std::shared_ptr<vkhlf::Queue> Scheduler::queue;

void Scheduler::initQueue(unsigned int queueFamilyIndex){
  queue = global::device->getQueue(queueFamilyIndex, 0);
}
void Scheduler::releaseQueue(){
  queue = nullptr;
}

void Scheduler::submitAndSync(const char* annotation, std::shared_ptr<vkhlf::CommandBuffer> cmdBuffer){
  //std::cout << "[SCHEDULER] " << annotation;
  auto fence = global::device->createFence(false);
  queue->submit(
    vkhlf::SubmitInfo{
      {},{}, cmdBuffer, {} },
    fence
    );
  //std::cout << " SYNC!" << std::endl;
  fence->wait(UINT64_MAX);
}

void Scheduler::fullSync() {
  queue->waitIdle();
}

void Scheduler::present(std::unique_ptr<vkhlf::FramebufferSwapchain> &swapchain){
  swapchain->present(queue);
}

void Scheduler::buildSubmitAndSync(const char* annotation, std::function<void (std::shared_ptr<vkhlf::CommandBuffer>)> record_commands){
  auto commandBuffer = global::commandPool->allocateCommandBuffer();
  commandBuffer->begin();
  record_commands(commandBuffer);
  commandBuffer->end();
  submitAndSync(annotation, commandBuffer);
}

} // namespace sga
