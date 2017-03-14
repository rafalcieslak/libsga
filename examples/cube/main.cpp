#define GLM_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#define SGA_USE_GLM
#include <sga.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

struct VertData{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texuv;
};

std::vector<VertData> vertices = {
  // Green
  {{ 1,-1,-1}, { 0, 0,-1}, {0    , 0  }},
  {{-1, 1,-1}, { 0, 0,-1}, {0.333, 0.5}},
  {{ 1, 1,-1}, { 0, 0,-1}, {0    , 0.5}},
  
  {{ 1,-1,-1}, { 0, 0,-1}, {0    , 0  }},
  {{-1,-1,-1}, { 0, 0,-1}, {0.333, 0  }},
  {{-1, 1,-1}, { 0, 0,-1}, {0.333, 0.5}},
  
  // Yellow
  {{ 1,-1, 1}, { 0, 1, 0}, {0.333, 0.0}},
  {{-1,-1,-1}, { 0, 1, 0}, {0.666, 0.5}},
  {{ 1,-1,-1}, { 0, 1, 0}, {0.333, 0.5}},
  
  
  {{ 1,-1, 1}, { 0, 1, 0}, {0.333, 0.0}},
  {{-1,-1, 1}, { 0, 1, 0}, {0.666, 0.5}},
  {{-1,-1,-1}, { 0, 1, 0}, {0.666, 0.5}},
  

};

int main(){
  sga::init();

  auto window = sga::Window::create(800, 600, "Cube");

  auto vbo = sga::VBO::create({
      sga::DataType::Float3,
      sga::DataType::Float3,
      sga::DataType::Float2},
    vertices.size());
  
  vbo->write(vertices);

  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      gl_Position = u.MVP * vec4(in_position,1);
      out_normal = (u.MVP * vec4(in_position,0) ).xyz;
      out_texuv = in_texuv;
    }
  )");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      out_color = texture(tex, in_texuv);
    }
  )");

  vertShader->addInput(sga::DataType::Float3, "in_position");
  vertShader->addInput(sga::DataType::Float3, "in_normal");
  vertShader->addInput(sga::DataType::Float2, "in_texuv");
  vertShader->addOutput(sga::DataType::Float3, "out_normal");
  vertShader->addOutput(sga::DataType::Float2, "out_texuv");
  vertShader->addUniform(sga::DataType::Mat4, "MVP");

  fragShader->addInput(sga::DataType::Float3, "in_normal");
  fragShader->addInput(sga::DataType::Float2, "in_texuv");
  fragShader->addOutput(sga::DataType::Float4, "out_color");
  fragShader->addSampler("tex");

  // Read image
  int w,h,n;
  unsigned char* data = stbi_load("data/cube.png", &w, &h, &n, 4);
  if(!data){
    std::cout << "Opening texture ./data/cube.png failed: " << stbi_failure_reason() << std::endl;
    return 1;
  }
  auto texture = sga::Image::create(w, h);
  texture->putDataRaw(data, w*h*4);
  
  glm::vec3 viewpos = {2,-2,-2};
  glm::mat4 camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
  glm::mat4 projection = glm::perspectiveFov(glm::radians(70.0), 800.0, 600.0, 0.1, 10.0);
  glm::mat4 MVP = projection * camera;
  
  auto program = sga::Program::create();
  program->setVertexShader(vertShader);
  program->setFragmentShader(fragShader);
  program->compile();

  auto pipeline = sga::Pipeline::create();
  pipeline->setProgram(program);
  pipeline->setTarget(window);
  pipeline->setFaceCull(sga::FaceCullMode::None);
  
  pipeline->setUniform("MVP", MVP);
  pipeline->setSampler("tex", texture, sga::SamplerInterpolation::Linear);
  
  window->setFPSLimit(60);

  window->setOnMouseMove([&](double x, double y){
      x = glm::min(x/window->getWidth(),  0.999);
      y = glm::min(y/window->getHeight(), 0.999);
      float phi = glm::radians(180*y - 90);
      float theta = -glm::radians(360*x);
      glm::vec3 p = {glm::cos(phi), glm::sin(phi), 0};
      viewpos = glm::rotateY(p * 4.0f, theta);
      camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
      MVP = projection * camera;
      pipeline->setUniform("MVP", MVP);
    });
  
  while(window->isOpen()){
    float angle = sga::getTime() * 0.4;
  
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  
  sga::terminate();
}
