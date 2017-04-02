#include <cmath>

#include <sga.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

int main(){
  sga::init();
  auto window = sga::Window::create(800, 600, "Example window");
  
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      vec2 coords = gl_FragCoord.xy / u.sgaResolution.xy;
      coords.x += 0.2 * sin(u.sgaTime);
      coords.y += 0.2 * cos(u.sgaTime);
      outColor = texture(tex, coords);
    }
  )");

  // Read image
  int w,h,n;
  unsigned char* data = stbi_load("examples/data/test_image.png", &w, &h, &n, 4);
  if(!data){
    std::cout << "Opening texture failed: " << stbi_failure_reason() << std::endl;
    return 1;
  }
  auto texture = sga::Image::create(w, h);
  texture->putData(std::vector<uint8_t>(data, data + w*h*4));
  
  fragShader->addOutput(sga::DataType::Float4, "outColor");
  fragShader->addSampler("tex");

  auto program = sga::Program::createAndCompile(fragShader);

  auto pipeline = sga::FullQuadPipeline::create();
  pipeline->setProgram(program);
  pipeline->setTarget(window);
  pipeline->setSampler("tex", texture);
  
  window->setFPSLimit(60);

  while(window->isOpen()){
    pipeline->drawFullQuad();
    window->nextFrame();
  }
  sga::terminate();
}
