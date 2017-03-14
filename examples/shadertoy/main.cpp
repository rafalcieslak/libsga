#include <fstream>
#include <sstream>

#include <sga.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

std::vector<std::array<float,2>> vertices = {
  { {-1, -1 } },
  { { 3, -1 } },
  { {-1,  3 } },
};

std::string vertSource = R"(
  void main() {
    gl_Position = vec4(inVertex, 0, 1);
  }
)";

std::string fragMain = R"(
  void main() {
    mainImage(outColor, vec2(gl_FragCoord.x, u.sgaResolution.y - gl_FragCoord.y));
  }
)";


static std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::string::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
  return subject;
}

int main(int argc, char** argv){
  // Parse command-line args
  if(argc < 2){
    std::cout << "USAGE: " << argv[0] << " shader_file" << std::endl;
    return 1;
  }
  // Read shader file
  std::ifstream file(argv[1]);
  if(!file){
    std::cout << "Failed to open file " << argv[1] << std::endl;
    return 1;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string fragSource = buffer.str();
  
  // Prepare shadertoy shader
  fragSource = ReplaceString(fragSource, "iGlobalTime", "u.sgaTime");
  fragSource = ReplaceString(fragSource, "iResolution", "u.sgaResolution");
  fragSource = ReplaceString(fragSource, "iMouse", "u.stoyMouse");
  fragSource += fragMain;

  // Prepare SGA
  sga::init();
  auto window = sga::Window::create(640*2, 360*2, "Shadertoy simulator");
  
  // Really wish we could use getopt, but it's posix-only!
  std::vector<std::string> textures;
  for(int i = 2; i < argc; i++)
    textures.push_back(argv[i]);

  // Load images
  stbi_set_flip_vertically_on_load(1);
  std::array<std::shared_ptr<sga::Image>,4> images;
  for(int i = 0; i < 4; i++){
    if(i < (int)textures.size()){
      int w,h,n;
      unsigned char* data = stbi_load(textures[i].c_str(), &w, &h, &n, 4);
      if(!data){
        std::cout << "Opening texture failed: " << stbi_failure_reason() << std::endl;
        return 1;
      }
      std::cout << "ASDASDSADAS" << std::endl;
      images[i] = sga::Image::create(w, h);
      images[i]->putDataRaw(data, w*h*4);
    }else{
      images[i] = sga::Image::create(16,16); // Some empty texture.
    }
  }
  
  auto vbo = sga::VBO::create({sga::DataType::Float2}, 3);
  vbo->write(vertices);
  
  auto vertShader = sga::VertexShader::createFromSource(vertSource);
  auto fragShader = sga::FragmentShader::createFromSource(fragSource);
  vertShader->addInput(sga::DataType::Float2, "inVertex");
  fragShader->addOutput(sga::DataType::Float4, "outColor");
  fragShader->addUniform(sga::DataType::Float4, "stoyMouse");
  fragShader->addSampler("iChannel0");
  fragShader->addSampler("iChannel1");
  fragShader->addSampler("iChannel2");
  fragShader->addSampler("iChannel3");

  auto program = sga::Program::create();
  program->setVertexShader(vertShader);
  program->setFragmentShader(fragShader);
  program->compile();

  auto pipeline = sga::Pipeline::create();
  pipeline->setProgram(program);
  pipeline->setTarget(window);
  pipeline->setUniform("stoyMouse", {0,0,-1,-1});
  pipeline->setSampler("iChannel0", images[0], sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
  pipeline->setSampler("iChannel1", images[1], sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
  pipeline->setSampler("iChannel2", images[2], sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
  pipeline->setSampler("iChannel3", images[3], sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);

  bool mouse_l_last = false;
  float click_x = 250.0, click_y = 250.0;
  window->setOnMouseAny([&](double x, double y, bool l, bool){
      if(!mouse_l_last && l){
        click_x = x;
        click_y = y;
      }
      if(!l) x = y = 0;
      pipeline->setUniform("stoyMouse",
                           {(float)x,
                            (float)y,
                            l ? click_x : -1.0f,
                            l ? click_y : -1.0f});
      mouse_l_last = l;
    });
  
  window->setFPSLimit(60);
  while(window->isOpen()){
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}
