#include <cmath>

#include <sga.hpp>

struct CustomData{
  float position[2];
  float color[3];
};

std::vector<CustomData> vertices = {
  { {-1.0,-1.0}, { 1, 0, 0}, },
  { { 1.0,-1.0}, { 1, 1, 0}, },
  { {-1.0, 1.0}, { 0, 0, 1}, },
  { { 1.0, 1.0}, { 0, 1, 0}, },
};

std::vector<int> indices = {
  0, 1, 2, 3, 2, 1
};

int main(){
  sga::init();

  sga::VBO vbo(
    {sga::DataType::Float2,
     sga::DataType::Float3},
    vertices.size());
  vbo.write(vertices);

  sga::IBO ibo(indices.size());
  ibo.write(indices);

  sga::VertexShader vertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      gl_Position = vec4(inVertex * 0.8, 0, 1);
      outColor = vec4(inColor, 1);
    })");
  sga::FragmentShader fragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      outColor = inColor;
    })");

  vertShader.addInput(sga::DataType::Float2, "inVertex");
  vertShader.addInput(sga::DataType::Float3, "inColor");
  vertShader.addOutput(sga::DataType::Float4, "outColor");

  fragShader.addInput(sga::DataType::Float4, "inColor");
  fragShader.addOutput(sga::DataType::Float4, "outColor");

  sga::Program program = sga::Program::createAndCompile(vertShader, fragShader);

  sga::Window window(800, 600, "Index buffer demo");
  window.setFPSLimit(60);

  sga::Pipeline pipeline;
  pipeline.setProgram(program);
  pipeline.setTarget(window);

  while(window.isOpen()){
    pipeline.clear();
    pipeline.drawIndexed(vbo, ibo);
    window.nextFrame();
  }
  sga::terminate();
}
