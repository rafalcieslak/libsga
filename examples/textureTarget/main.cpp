#include <cmath>

#include <sga.hpp>

int main(){
  sga::init();
  
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      outColor = vec4(sgaWindowCoords, 0, 1);
      vec2 screenPos = gl_FragCoord.xy;
      if( int(screenPos.x) % 50 < 3 ){
        outColor.rgb = vec3(0);
      }
      if( int(screenPos.y) % 50 < 3 ){
        outColor.rgb = vec3(0);
      }
    }
  )");

  fragShader.addOutput(sga::DataType::Float4, "outColor");

  auto program = sga::Program::createAndCompile(fragShader);

  sga::Image image(1440, 900);

  sga::FullQuadPipeline pipeline;
  pipeline.setProgram(program);
  pipeline.setTarget(image);

  pipeline.drawFullQuad();

  // Save to file.
  image.savePNG("output.png");
  
  sga::terminate();
}
