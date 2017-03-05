#include <sga.hpp>

int main(){
  sga::init();
  sga::Window window(420, 240, "Example window");
  while(!window.getShouldClose()){
    window.nextFrame();
    window.limitFPS(5);
  }
  sga::terminate();
}