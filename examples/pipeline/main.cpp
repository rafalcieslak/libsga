#include <cmath>

#include <sga.hpp>

void updateBgColor(std::shared_ptr<sga::Pipeline> pipeline){
  double q = sga::getTime() * 2.5;
  double r = std::sin(q + 0.00 * M_PI * 2);
  double g = std::sin(q + 0.33 * M_PI * 2);
  double b = std::sin(q + 0.66 * M_PI * 2);
  pipeline->setClearColor(r, g, b);
}

int main(){
  sga::init();
  auto window = sga::Window::create(480, 360, "Example window");
  auto pipeline = sga::Pipeline::create();

  pipeline->setTarget(window);
  
  while(!window->getShouldClose()){
    window->nextFrame();
    window->limitFPS(50);

    updateBgColor(pipeline);
    pipeline->drawTestTriangle();
  }
  sga::terminate();
}
