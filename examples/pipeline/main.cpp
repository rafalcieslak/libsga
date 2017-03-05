#include <cmath>

#include <sga.hpp>

int main(){
  sga::init();
  auto window = sga::Window::create(480, 360, "Example window");
  auto pipeline = sga::Pipeline::create();

  pipeline->setTarget(window);
  
  window->setFPSLimit(30);
  
  while(window->isOpen()){
    
    double q = sga::getTime() * 2.5;
    double r = std::sin(q + 0.00 * M_PI * 2);
    double g = std::sin(q + 0.33 * M_PI * 2);
    double b = std::sin(q + 0.66 * M_PI * 2);
    pipeline->setClearColor(r, g, b);
  
    pipeline->drawTestTriangle();
    window->nextFrame();
  }
  sga::terminate();
}
