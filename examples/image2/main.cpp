#include <sga.hpp>

int main(){
  sga::init();

  auto window = sga::Window::create(320,240,"Image test");
  
  auto image = sga::Image::create(10,10);

  while(window->isOpen()){
  }
  
  sga::terminate();
}
