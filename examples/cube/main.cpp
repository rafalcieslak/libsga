#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

#define SGA_USE_GLM
#include <sga.hpp>
#include "../common/common.hpp"

struct VertData{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texuv;
};

std::vector<VertData> vertices = {
  // Green
  {{-1,-1,-1}, { 0, 0,-1}, {0       , 0  }},
  {{ 1, 1,-1}, { 0, 0,-1}, {0.333333, 0.5}},
  {{-1, 1,-1}, { 0, 0,-1}, {0       , 0.5}},
  {{-1,-1,-1}, { 0, 0,-1}, {0       , 0  }},
  {{ 1,-1,-1}, { 0, 0,-1}, {0.333333, 0  }},
  {{ 1, 1,-1}, { 0, 0,-1}, {0.333333, 0.5}},
                                    
  // Yellow                         
  {{-1,-1, 1}, { 0,-1, 0}, {0.333333, 0.0}},
  {{ 1,-1,-1}, { 0,-1, 0}, {0.666666, 0.5}},
  {{-1,-1,-1}, { 0,-1, 0}, {0.333333, 0.5}},
  {{-1,-1, 1}, { 0,-1, 0}, {0.333333, 0.0}},
  {{ 1,-1, 1}, { 0,-1, 0}, {0.666666, 0.0}},
  {{ 1,-1,-1}, { 0,-1, 0}, {0.666666, 0.5}},
                                    
  // Orange                         
  {{ 1,-1,-1}, { 1, 0, 0}, {0.666666, 0.0}},
  {{ 1, 1, 1}, { 1, 0, 0}, {0.999999, 0.5}},
  {{ 1, 1,-1}, { 1, 0, 0}, {0.666666, 0.5}},
  {{ 1,-1,-1}, { 1, 0, 0}, {0.666666, 0.0}},
  {{ 1,-1, 1}, { 1, 0, 0}, {0.999999, 0.0}},
  {{ 1, 1, 1}, { 1, 0, 0}, {0.999999, 0.5}},
                                    
  // Blue                           
  {{ 1,-1, 1}, { 0, 0, 1}, {0       , 0.5}},
  {{-1, 1, 1}, { 0, 0, 1}, {0.333333, 1.0}},
  {{ 1, 1, 1}, { 0, 0, 1}, {0       , 1.0}},
  {{ 1,-1, 1}, { 0, 0, 1}, {0       , 0.5}},
  {{-1,-1, 1}, { 0, 0, 1}, {0.333333, 0.5}},
  {{-1, 1, 1}, { 0, 0, 1}, {0.333333, 1.0}},
                                    
  // White                          
  {{ 1, 1, 1}, { 0, 1, 0}, {0.333333, 0.5}},
  {{-1, 1,-1}, { 0, 1, 0}, {0.666666, 1.0}},
  {{ 1, 1,-1}, { 0, 1, 0}, {0.333333, 1.0}},
  {{ 1, 1, 1}, { 0, 1, 0}, {0.333333, 0.5}},
  {{-1, 1, 1}, { 0, 1, 0}, {0.666666, 0.5}},
  {{-1, 1,-1}, { 0, 1, 0}, {0.666666, 1.0}},
                                    
  // Red                            
  {{-1, 1,-1}, {-1, 0, 0}, {0.666666, 0.5}},
  {{-1,-1, 1}, {-1, 0, 0}, {0.999999, 1.0}},
  {{-1,-1,-1}, {-1, 0, 0}, {0.666666, 1.0}},
  {{-1, 1,-1}, {-1, 0, 0}, {0.666666, 0.5}},
  {{-1, 1, 1}, {-1, 0, 0}, {0.999999, 0.5}},
  {{-1,-1, 1}, {-1, 0, 0}, {0.999999, 1.0}},
};

int main(){
  sga::init();

  auto window = sga::Window::create(800, 600, "Cube");

  // Prepare VBO
  auto vbo = sga::VBO::create({
      sga::DataType::Float3,
      sga::DataType::Float3,
      sga::DataType::Float2},
    vertices.size());
  
  vbo->write(vertices);

  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      gl_Position = u.MVP * vec4(in_position,1);
      out_normal = in_normal; //(u.MVP * vec4(in_position,0) ).xyz;
      out_texuv = in_texuv;
      out_position = in_position;

      gl_Position.y = -gl_Position.y;
    }
  )");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      vec3 dir_to_light = normalize(u.lightpos - in_position);
      float q = max(0.2, dot(dir_to_light, in_normal));
      out_color = texture(tex, in_texuv) * q;
    }
  )");

  // Configure shader input/output/uniforms
  vertShader->addInput({{sga::DataType::Float3, "in_position"},
                        {sga::DataType::Float3, "in_normal"},
                        {sga::DataType::Float2, "in_texuv"}});
  vertShader->addOutput({{sga::DataType::Float3, "out_position"},
                         {sga::DataType::Float3, "out_normal"},
                         {sga::DataType::Float2, "out_texuv"}});
  vertShader->addUniform(sga::DataType::Mat4, "MVP");

  fragShader->addInput(sga::DataType::Float3, "in_position");
  fragShader->addInput(sga::DataType::Float3, "in_normal");
  fragShader->addInput(sga::DataType::Float2, "in_texuv");
  fragShader->addOutput(sga::DataType::Float4, "out_color");
  fragShader->addUniform(sga::DataType::Float3, "lightpos");
  fragShader->addSampler("tex");

  // Read images
  auto textureImage  = sga::Image::createFromPNG(EXAMPLE_DATA_DIR "cube.png", sga::ImageFormat::NInt8, sga::ImageFilterMode::Anisotropic);
  auto textureImage2 = sga::Image::createFromPNG(EXAMPLE_DATA_DIR "cube2.png", sga::ImageFormat::NInt8, sga::ImageFilterMode::Anisotropic);

  // Compute initial MVP
  glm::vec3 viewpos = {0,0,-4};
  glm::mat4 camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
  glm::mat4 projection = glm::perspectiveFov(glm::radians(70.0), 800.0, 600.0, 0.1, 10.0);
  glm::mat4 MVP = projection * camera;

  // Compile shaders
  auto program = sga::Program::create();
  program->setVertexShader(vertShader);
  program->setFragmentShader(fragShader);
  program->compile();

  // Configure pipeline
  auto pipeline = sga::Pipeline::create();
  pipeline->setProgram(program);
  pipeline->setTarget(window);
  pipeline->setFaceCull(sga::FaceCullMode::None);
  
  pipeline->setUniform("MVP", MVP);
  pipeline->setUniform("lightpos", {3.0, -4.0, -5.0});
  pipeline->setSampler("tex", textureImage);
  
  window->setFPSLimit(60);

  window->setOnKeyDown(sga::Key::Escape, [&](){
      window->close();
    });

  window->setOnKeyDown(sga::Key::F11, [&](){
      window->toggleFullscreen();
    });

  window->setOnKeyDown(sga::Key::d1, [&](){
      pipeline->setRasterizerMode(sga::RasterizerMode::Filled);
    });
  window->setOnKeyDown(sga::Key::d2, [&](){
      pipeline->setRasterizerMode(sga::RasterizerMode::Wireframe);
    });
  window->setOnKeyDown(sga::Key::d3, [&](){
      pipeline->setRasterizerMode(sga::RasterizerMode::Points);
    });
  
  window->setOnMouseMove([&](double x, double y){
      // Calculate new viewpos and MVP
      x = glm::min(x/window->getWidth(),  0.999);
      y = glm::min(y/window->getHeight(), 0.999);
      float phi = glm::radians(180*y - 90);
      float theta = glm::radians(360*x);
      glm::vec3 p = {0, glm::sin(phi), glm::cos(phi)};
      viewpos = glm::rotateY(p * 4.0f, theta);
      camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
      MVP = projection * camera;
      pipeline->setUniform("MVP", MVP);
    });

  // Main loop
  while(window->isOpen()){
    // Periodically switch texture
    float q = sga::getTime();
    int w = q/0.75;
    if(w % 2) pipeline->setSampler("tex", textureImage);
    else      pipeline->setSampler("tex", textureImage2);
    
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  
  sga::terminate();
}
