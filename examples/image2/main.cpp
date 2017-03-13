#include <sga.hpp>

int main(){
  sga::init();

  auto window = sga::Window::create(320,240,"Image test");
  
  auto image = sga::Image::create(10,10);
  image->fillWithPink();

  while(window->isOpen()){
    window->nextFrame();
    // Not supported!
    //image->copyOnto(window, 0, 0, 2, 0);
  }
  
  sga::terminate();
}
