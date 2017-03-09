#include <cmath>

#include <sga.hpp>

extern char const *testVertShaderText;
extern char const *testFragShaderText;

struct __attribute__((packed)) CustomData{
  float position[2];
  float color[4];
};

int main(){
  sga::init();
  auto window = sga::Window::create(480, 360, "Example window");
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO<sga::DataType::Float2,
                      sga::DataType::Float4>::create(3);

  auto vertShader = sga::VertexShader::createFromSource(testVertShaderText);
  auto fragShader = sga::FragmentShader::createFromSource(testFragShaderText);

  vertShader->addInput(sga::DataType::Float2, "inVertex");
  vertShader->addInput(sga::DataType::Float4, "inColor");
  vertShader->addOutput(sga::DataType::Float4, "outColor");

  fragShader->addInput(sga::DataType::Float4, "inColor");
  fragShader->addOutput(sga::DataType::Float4, "outColor");
  
  vertShader->compile();
  fragShader->compile();

  pipeline->setVertexShader(vertShader);
  pipeline->setFragmentShader(fragShader);
  pipeline->setTarget(window);
  
  window->setFPSLimit(60);

  while(window->isOpen()){
    double q = sga::getTime() * 3.0;
    float r = std::sin(q + 0.00 * M_PI * 2);
    float g = std::sin(q + 0.33 * M_PI * 2);
    float b = std::sin(q + 0.66 * M_PI * 2);
    float R = std::max(0.0, std::pow(r, 0.8));
    float G = std::max(0.0, std::pow(g, 0.8));
    float B = std::max(0.0, std::pow(b, 0.8));
    
    std::vector<CustomData> f = {
      { {  0.0f, -0.5f },{ R, G, B, 1.0 }, },
      { {  0.5f,  0.5f },{ G, B, R, 1.0 }, },
      { { -0.5f,  0.5f },{ B, R, G, 1.0 }, },
    };
    vbo->write(f);
  
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}


char const *testVertShaderText = R"(
void main()
{
  outColor = inColor;
  gl_Position = vec4(inVertex, 0, 1);
}
)";

char const *testFragShaderText = R"(
void main()
{
  outColor = inColor;
}
)";
