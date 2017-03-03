#include <sga.hpp>

int main(){
  sga::info();
  sga::init(sga::VerbosityLevel::Debug);

  sga::Image image(10,10);
  image.fillWithPink();
  image.testContents();

  sga::info();
  
  sga::cleanup();
}
