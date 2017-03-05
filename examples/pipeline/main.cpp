#include <sga.hpp>

int main(){
  sga::init();
  auto window = sga::Window::create(480, 360, "Example window");
  auto pipeline = sga::Pipeline::create();

  pipeline->setTarget(window);
  
  while(!window->getShouldClose()){
    window->nextFrame();
    window->limitFPS(5);
    pipeline->drawTestTriangle();
  }
  sga::terminate();
}
