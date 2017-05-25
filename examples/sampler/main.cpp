#include <cmath>

#include <sga.hpp>
#include "../common/common.hpp"

int main(){
  sga::init();
  auto window = sga::Window::create(800, 600, "Example window");
  
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      vec2 coords = sgaWindowCoords;
      coords.xy -= 0.5;
      coords *= (-cos(sgaTime / 2.0) + 1.01) * 60;
      coords.x += 0.2 * sin(sgaTime);
      coords.y += 0.2 * cos(sgaTime);
      coords.xy += 0.5;
      outColor = texture(tex, coords);
    }
  )");

  // Read image
  auto texture = sga::Image::createFromPNG(EXAMPLE_DATA_DIR "test_image.png", sga::ImageFormat::NInt8, sga::ImageFilterMode::Anisotropic);
  
  fragShader->addOutput(sga::DataType::Float4, "outColor");
  fragShader->addSampler("tex");

  auto program = sga::Program::createAndCompile(fragShader);

  auto pipeline = sga::FullQuadPipeline::create();
  pipeline->setProgram(program);
  pipeline->setTarget(window);
  pipeline->setSampler("tex", texture, sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
  
  window->setFPSLimit(60);

  window->setOnKeyDown(sga::Key::Escape, [&](){ window->close(); });
  window->setOnKeyDown(sga::Key::F11, [&](){ window->toggleFullscreen(); });

  while(window->isOpen()){
    pipeline->clear();
    pipeline->drawFullQuad();
    window->nextFrame();
  }
  sga::terminate();
}
