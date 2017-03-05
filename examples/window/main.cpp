#include <sga.hpp>

int main(){
  sga::init();
  sga::Window window(420, 240, "Example window");
  window.setFPSLimit(5);
  while(window.isOpen()){
    window.nextFrame();
  }
  sga::terminate();
}
