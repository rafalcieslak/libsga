#include <cmath>

#include <sga.hpp>

extern char const *testVertShaderText;
extern char const *testFragShaderText;

struct __attribute__((packed)) CustomData{
  float position[2];
  uint8_t color[4];
};

int main(){
  sga::init();
  auto window = sga::Window::create(480, 360, "Example window");
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO<sga::DataType::Float2,
                      sga::DataType::UByte4>::create(3);

  auto vertShader = sga::VertexShader::createFromSource(
    testVertShaderText,
    {sga::DataType::Float2, sga::DataType::UByte4},
    {sga::DataType::UByte4});
  auto fragShader = sga::FragmentShader::createFromSource(
    testFragShaderText,
    {sga::DataType::UByte4},
    {sga::DataType::UByte4});

  pipeline->setVertexShader(vertShader);
  pipeline->setFragmentShader(fragShader);
  pipeline->setTarget(window);
  
  window->setFPSLimit(60);

  while(window->isOpen()){
    double q = sga::getTime() * 3.0;
    double r = std::sin(q + 0.00 * M_PI * 2);
    double g = std::sin(q + 0.33 * M_PI * 2);
    double b = std::sin(q + 0.66 * M_PI * 2);
    uint8_t R = std::max(0.0, std::pow(r, 0.8)*255);
    uint8_t G = std::max(0.0, std::pow(g, 0.8)*255);
    uint8_t B = std::max(0.0, std::pow(b, 0.8)*255);
    
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


char const *testVertShaderText =
R"(#version 430
layout(location = 0) in vec2 inVertex;
layout(location = 1) in vec4 inColor;
layout(location = 0) out vec4 outColor;
void main()
{
  outColor = inColor;
  gl_Position = vec4(inVertex, 0, 1);
}
)";

char const *testFragShaderText = 
R"(#version 430
layout(location = 0) in vec4 inColor;
layout(location = 0) out vec4 outColor;
void main()
{
  outColor = inColor;
}
)";
