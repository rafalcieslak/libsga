#include <cmath>

#include <sga.hpp>

struct __attribute__((packed)) CustomData{
  float position[2];
  float color[4];
};

std::vector<CustomData> vertices = {
  { { 0.0,         -1.0 }, { 1, 0, 0, 1 }, },
  { { 0.8660254037, 0.5 }, { 0, 1, 0, 1 }, },
  { {-0.8660254037, 0.5 }, { 0, 0, 1, 1 }, },
};

int main(){
  sga::init();
  auto window = sga::Window::create(500, 500, "Example window");
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO<sga::DataType::Float2,
                      sga::DataType::Float4>::create(3);
  
  vbo->write(vertices);
  
  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main()
    {
      mat2 rotate = mat2( cos(u.angle), sin(u.angle), 
                         -sin(u.angle), cos(u.angle));
      vec2 pos = rotate * inVertex;
      gl_Position = vec4(pos, 0, 1);
      outColor = inColor;
    }
  )");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      outColor = inColor;
    }
  )");

  vertShader->addInput(sga::DataType::Float2, "inVertex");
  vertShader->addInput(sga::DataType::Float4, "inColor");
  vertShader->addOutput(sga::DataType::Float4, "outColor");

  fragShader->addInput(sga::DataType::Float4, "inColor");
  fragShader->addOutput(sga::DataType::Float4, "outColor");

  vertShader->addUniform(sga::DataType::Float, "angle");
  
  vertShader->compile();
  fragShader->compile();

  pipeline->setVertexShader(vertShader);
  pipeline->setFragmentShader(fragShader);
  pipeline->setTarget(window);
  
  window->setFPSLimit(60);

  while(window->isOpen()){
    float angle = sga::getTime() * 0.4;
    pipeline->setUniform("angle", angle);
  
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}
