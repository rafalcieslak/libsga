#include <sga.hpp>

int main(){
  sga::info();
  sga::init(sga::VerbosityLevel::Debug);
  sga::init(sga::VerbosityLevel::Debug);
  sga::info();

  sga::Image(200,400);
  
  sga::cleanup();
}
