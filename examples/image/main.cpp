#include <sga.hpp>

int main(){
  sga::init();

  auto image = sga::Image::create(10,10);
  auto image2 = sga::Image::create(10,10);
  image->fillWithPink();
  image->copyOnto(image2, 0, 0, 2, 0);
  image2->testContents();

  sga::terminate();
}
