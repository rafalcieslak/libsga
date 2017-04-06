#include <cmath>

#include <sga.hpp>
#include "../common/common.hpp"

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
  auto texture = sga::Image::createFromPNG(EXAMPLE_DATA_DIR "test_image.png");
  
  fragShader->addOutput(sga::DataType::Float4, "outColor");
  fragShader->addSampler("tex");

  auto program = sga::Program::createAndCompile(fragShader);

  auto pipeline = sga::FullQuadPipeline::create();
  pipeline->setProgram(program);
  pipeline->setTarget(window);
  pipeline->setSampler("tex", texture);
  
  window->setFPSLimit(60);

  while(window->isOpen()){
    pipeline->clear();
    pipeline->drawFullQuad();
    window->nextFrame();
  }
  sga::terminate();
}
