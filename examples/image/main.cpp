#include <sga.hpp>

int main(){
  sga::init();

  auto image = sga::Image::create(10,10);
  auto image2 = sga::Image::create(10,10);

  sga::terminate();
}
