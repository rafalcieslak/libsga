#include <memory>
#include <sga.hpp>

int main(){
  sga::init();
  auto window = std::make_shared<sga::Window>(480, 360, "Example window");
  auto pipeline = std::make_shared<sga::Pipeline>();

  pipeline->setTarget(window);
  
  while(!window->getShouldClose()){
    window->nextFrame();
    window->limitFPS(5);

    pipeline->drawTestTriangle();


  }
  sga::terminate();
}
