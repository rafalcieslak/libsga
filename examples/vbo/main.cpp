#include <cmath>

#include <sga.hpp>

struct __attribute__((packed)) CustomData{
  float position[3];
  float color[4];
};

std::vector<CustomData> vertices = {
  // Smaller triangle.
  { { 0.0,         -1.0, 0.1 }, { 1, 0, 0, 1 }, },
  { { 0.8660254037, 0.5, 0.1 }, { 0, 1, 0, 1 }, },
  { {-0.8660254037, 0.5, 0.1 }, { 0, 0, 1, 1 }, },
  
  // Slightly larger triangle
  { { 0.0          * 1.2,-1.0 * 1.2, 0.2 }, { 0.4, 0.2, 0, 1 }, },
  { { 0.8660254037 * 1.2, 0.5 * 1.2, 0.2 }, { 0, 0.4, 0.2, 1 }, },
  { {-0.8660254037 * 1.2, 0.5 * 1.2, 0.2 }, { 0.2, 0, 0.4, 1 }, },
};

int main(){
  sga::init();
  auto window = sga::Window::create(500, 500, "Example window");
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO::create(
    {sga::DataType::Float3,
     sga::DataType::Float4}, 6);
  
  vbo->write(vertices);
  
  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main()
    {
      mat3 rotate = mat3( cos(u.angle), sin(u.angle), 0, 
                         -sin(u.angle), cos(u.angle), 0,
                                     0,            0, 1);
      vec3 pos = rotate * inVertex;
      gl_Position = vec4(pos, 1);
      outColor = inColor;
    }
  )");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      outColor = inColor + 0.2 * texture(some_texture, vec2(0, 0));
    }
  )");

  vertShader->addInput(sga::DataType::Float3, "inVertex");
  vertShader->addInput(sga::DataType::Float4, "inColor");
  vertShader->addOutput(sga::DataType::Float4, "outColor");

  fragShader->addInput(sga::DataType::Float4, "inColor");
  fragShader->addOutput(sga::DataType::Float4, "outColor");

  vertShader->addUniform(sga::DataType::Float, "angle");
  vertShader->addSampler("some_texture");

  auto program = sga::Program::create();
  program->setVertexShader(vertShader);
  program->setFragmentShader(fragShader);
  program->compile();

  auto texture = sga::Image::create(64,64);
  texture->fillWithPink();
  
  pipeline->setProgram(program);
  pipeline->setTarget(window);
  pipeline->setSampler("some_texture", texture);
  
  window->setFPSLimit(60);

  while(window->isOpen()){
    float angle = sga::getTime() * 0.4;
    pipeline->setUniform("angle", angle);
  
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}
