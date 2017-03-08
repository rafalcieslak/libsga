#include <sga.hpp>

int main(){
  sga::init();
  auto window = sga::Window::create(420, 240, "Example window");
  window->setFPSLimit(5);
  while(window->isOpen()){
    window->nextFrame();
  }
  sga::terminate();
}
