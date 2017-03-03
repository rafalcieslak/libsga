#include <sga.hpp>

int main(){
  sga::info();
  sga::init(sga::VerbosityLevel::Debug);
  sga::init(sga::VerbosityLevel::Debug);
  sga::info();

  sga::cleanup();
}
