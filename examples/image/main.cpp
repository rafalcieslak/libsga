#include <sga.hpp>

int main(){
  sga::init();

  sga::Image image(10,10);
  image.fillWithPink();
  image.testContents();

  sga::terminate();
}
