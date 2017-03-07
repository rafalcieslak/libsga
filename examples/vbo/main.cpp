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
  
  pipeline->setTarget(window);
  
  window->setFPSLimit(60);
  
  while(window->isOpen()){
    double q = sga::getTime() * 3.0;
    double r = std::sin(q + 0.00 * M_PI * 2);
    double g = std::sin(q + 0.33 * M_PI * 2);
    double b = std::sin(q + 0.66 * M_PI * 2);
    uint8_t R = std::max(0.0, r*255);
    uint8_t G = std::max(0.0, g*255);
    uint8_t B = std::max(0.0, b*255);
    
    std::vector<CustomData> f = {
      { {  0.0f, -0.5f },{ R, G, B, 0xFF }, },
      { {  0.5f,  0.5f },{ G, B, R, 0xFF }, },
      { { -0.5f,  0.5f },{ B, R, G, 0xFF }, },
    };
    vbo->write(f);
  
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}

