#include <cmath>

#include <sga.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/stb_image_write.h"

struct __attribute__((packed)) CustomData{
  float position[2];
};

std::vector<CustomData> vertices = {
  { {-1, -1 } },
  { { 3, -1 } },
  { {-1,  3 } },
};

int main(){
  sga::init();

  auto vbo = sga::VBO::create(
    {sga::DataType::Float2}, 3);
  
  vbo->write(vertices);
  
  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main()
    {
      gl_Position = vec4(inVertex, 0, 1);
    }
  )");
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

  vertShader->addInput(sga::DataType::Float2, "inVertex");
  fragShader->addOutput(sga::DataType::Float4, "outColor");

  auto program = sga::Program::create();
  program->setVertexShader(vertShader);
  program->setFragmentShader(fragShader);
  program->compile();

  auto image = sga::Image::create(1440, 900);

  auto pipeline = sga::Pipeline::create();
  pipeline->setProgram(program);
  
  pipeline->setTarget({image});

  pipeline->drawVBO(vbo);

  // Save to file.
  auto out = image->getData();
  stbi_write_png("output.png", image->getWidth(), image->getHeight(), 4, out.data(), 0);
  
  sga::terminate();
}
