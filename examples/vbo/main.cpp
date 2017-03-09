#include <cmath>

#include <sga.hpp>

struct __attribute__((packed)) CustomData{
  float position[2];
  float color[4];
};

int main(){
  sga::init();
  auto window = sga::Window::create(500, 500, "Example window");
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO<sga::DataType::Float2,
                      sga::DataType::Float4>::create(3);

  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main()
    {
      outColor = inColor;
      gl_Position = vec4(inVertex, 0, 1);
    }
  )");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      outColor = inColor;
      outColor.xyz += vec3(sin(u.time) * 0.4);
    }
  )");

  vertShader->addInput(sga::DataType::Float2, "inVertex");
  vertShader->addInput(sga::DataType::Float4, "inColor");
  vertShader->addOutput(sga::DataType::Float4, "outColor");

  fragShader->addInput(sga::DataType::Float4, "inColor");
  fragShader->addOutput(sga::DataType::Float4, "outColor");

  fragShader->addUniform(sga::DataType::Float, "time");
  
  vertShader->compile();
  fragShader->compile();

  pipeline->setVertexShader(vertShader);
  pipeline->setFragmentShader(fragShader);
  pipeline->setTarget(window);
  
  window->setFPSLimit(60);

  while(window->isOpen()){
    double q = sga::getTime() * 0.8;
    float x1 =  std::sin(q + 0.0000 * M_PI * 2);
    float x2 =  std::sin(q + 0.3333 * M_PI * 2);
    float x3 =  std::sin(q + 0.6666 * M_PI * 2);
    float y1 = -std::cos(q + 0.0000 * M_PI * 2);
    float y2 = -std::cos(q + 0.3333 * M_PI * 2);
    float y3 = -std::cos(q + 0.6666 * M_PI * 2);
    
    std::vector<CustomData> f = {
      { { x1, y1 },{ 1, 0, 0, 1 }, },
      { { x2, y2 },{ 0, 1, 0, 1 }, },
      { { x3, y3 },{ 0, 0, 1, 1 }, },
    };
    vbo->write(f);

    pipeline->setUniform("time", (float)sga::getTime());
  
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}
