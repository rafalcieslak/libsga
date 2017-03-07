#include <cmath>

#include <sga.hpp>

struct __attribute__ ((packed)) CustomData{
  float position[2];
  uint8_t color[4];
};

int main(){
  sga::init();
  auto window = sga::Window::create(480, 360, "Example window");
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO<sga::DataType::Float2,
                      sga::DataType::UByte4>::create(3);
  
  std::vector<CustomData> f = {
    { {  0.0f, -0.5f },{ 0xFF, 0x00, 0x00, 0xFF }, },
    { {  0.5f,  0.5f },{ 0x00, 0x00, 0xFF, 0xFF }, },
    { { -0.5f,  0.5f },{ 0x00, 0xFF, 0x00, 0xFF }, },
  };
  vbo->write(f);
  
  pipeline->setTarget(window);
  
  window->setFPSLimit(30);
  
  while(window->isOpen()){
    
    double q = sga::getTime() * 2.5;
    double r = std::sin(q + 0.00 * M_PI * 2);
    double g = std::sin(q + 0.33 * M_PI * 2);
    double b = std::sin(q + 0.66 * M_PI * 2);
    pipeline->setClearColor(r, g, b);
  
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}

