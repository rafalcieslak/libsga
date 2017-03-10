#include <cmath>

#include <sga.hpp>

struct __attribute__((packed)) CustomData{
  float position[2];
  float fragPos[2];
};

std::vector<CustomData> vertices = {
  { {-1, -1 }, { 0, 0 }, },
  { { 3, -1 }, { 2, 0 }, },
  { {-1,  3 }, { 0, 2 }, },
};

int main(){
  sga::init(sga::VerbosityLevel::Debug, sga::ErrorStrategy::MessageThrow);
  auto window = sga::Window::create(800, 600, "Example window");
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO::create(
    {sga::DataType::Float2,
     sga::DataType::Float2}, 3);
  
  vbo->write(vertices);
  
  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main()
    {
      gl_Position = vec4(inVertex, 0, 1);
      outFragPos = inFragPos;
    }
  )");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      outColor = vec4(inFragPos, 0, 1);
      vec2 screenPos = inFragPos * u.sgaResolution;
      if( int(screenPos.x) % 50 < 3 ){
        outColor.rg = vec2(0);
        outColor.b += 0.5;
      }
      if( int(screenPos.y) % 50 < 3 ){
        outColor.rg = vec2(0);
        outColor.b += 0.5;
      }
    }
  )");

  vertShader->addInput(sga::DataType::Float2, "inVertex");
  vertShader->addInput(sga::DataType::Float2, "inFragPos");
  vertShader->addOutput(sga::DataType::Float2, "outFragPos");

  fragShader->addInput(sga::DataType::Float2, "inFragPos");
  fragShader->addOutput(sga::DataType::Float4, "outColor");

  vertShader->compile();
  fragShader->compile();

  pipeline->setVertexShader(vertShader);
  pipeline->setFragmentShader(fragShader);
  pipeline->setTarget(window);
  
  window->setFPSLimit(60);

  while(window->isOpen()){
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}
