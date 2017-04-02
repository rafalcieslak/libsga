#include <cmath>

#include <sga.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/stb_image_write.h"

int main(){
  sga::init();
  
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      outColor = vec4(gl_FragCoord.xy / u.sgaResolution, 0, 1);
      vec2 screenPos = gl_FragCoord.xy;
      if( int(screenPos.x) % 50 < 3 ){
        outColor.rgb = vec3(0);
      }
      if( int(screenPos.y) % 50 < 3 ){
        outColor.rgb = vec3(0);
      }
    }
  )");

  fragShader->addOutput(sga::DataType::Float4, "outColor");

  auto program = sga::Program::createAndCompile(fragShader);

  auto image = sga::Image::create(1440, 900);

  auto pipeline = sga::FullQuadPipeline::create();
  pipeline->setProgram(program);
  pipeline->setTarget({image});

  pipeline->drawFullQuad();

  // Save to file.
  std::vector<uint8_t> out(image->getElems());
  image->getData(out);
  stbi_write_png("output.png", image->getWidth(), image->getHeight(), 4, out.data(), 0);
  
  sga::terminate();
}
