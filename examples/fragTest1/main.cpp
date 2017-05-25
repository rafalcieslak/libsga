#include <cmath>

#include <sga.hpp>

int main(){
  sga::init();
  auto window = sga::Window::create(800, 600, "Example window");
  
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      outColor = vec4(sgaWindowCoords, 0, 1);
      vec2 screenPos = gl_FragCoord.xy;
      if( int(screenPos.x) % 50 < 3 ){
        outColor.rg = vec2(0);
        outColor.b += 0.5;
      }
      if( int(screenPos.y) % 50 < 3 ){
        outColor.rg = vec2(0);
        outColor.b += 0.5;
      }
    }
  )");

  fragShader->addOutput(sga::DataType::Float4, "outColor");

  auto program = sga::Program::createAndCompile(fragShader);

  auto pipeline = sga::FullQuadPipeline::create();
  pipeline->setProgram(program);
  pipeline->setTarget(window);
  
  window->setFPSLimit(60);

  window->setOnKeyDown(sga::Key::F11, [&](){
      window->toggleFullscreen();
    });
  
  while(window->isOpen()){
    pipeline->drawFullQuad();
    window->nextFrame();
  }
  sga::terminate();
}
