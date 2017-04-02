#include <sga.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../common/stb_image_write.h"

int main(){
  sga::init();

  // ==== Images ====

  // Source image
  int w,h,n;
  unsigned char* data = stbi_load("examples/data/test_image.png", &w, &h, &n, 4);
  if(!data){
    std::cout << "Opening texture failed: " << stbi_failure_reason() << std::endl;
    return 1;
  }
  auto image0 = sga::Image::create(w, h);
  image0->putData(std::vector<uint8_t>(data, data + w*h*4));
  // Intermediate buffer
  auto image1 = sga::Image::create(1440, 900);
  // Final result
  auto image2 = sga::Image::create(1440, 900);


  //  ==== Pipeline 1 ==== (Ripple effect)

  auto fragShader1 = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      vec2 dx = vec2(1.0 / u.sgaResolution.x, 0);
      vec2 dy = vec2(0, 1.0 / u.sgaResolution.y);
      vec2 here = gl_FragCoord.xy / u.sgaResolution;
      float disthere = length(gl_FragCoord.xy - u.sgaResolution/2) / 30.0;
      float offset = pow(sin(disthere), 3);
      vec2 center = vec2(0.5, 0.5);
      vec2 dir = here - center;
      vec2 point = center + dir * (1.0 + 0.1*offset);
      vec3 q0 = texture(image0, point).xyz;
      outColor = vec4(q0, 1);
    }
  )");

  fragShader1->addOutput(sga::DataType::Float4, "outColor");
  fragShader1->addSampler("image0");

  auto program1 = sga::Program::createAndCompile(fragShader1);
  auto pipeline1 = sga::FullQuadPipeline::create();
  pipeline1->setProgram(program1);
  pipeline1->setSampler("image0", image0);
  pipeline1->setTarget({image1});


  //  ==== Pipeline 2 ==== (Border highlight)

  auto fragShader2 = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      vec2 dx = vec2(1.0 / u.sgaResolution.x, 0);
      vec2 dy = vec2(0, 1.0 / u.sgaResolution.y);
      vec2 here = gl_FragCoord.xy / u.sgaResolution;
      vec3 q0 = texture(image1, here - dx - dy).xyz;
      vec3 q1 = texture(image1, here      - dy).xyz;
      vec3 q2 = texture(image1, here + dx - dy).xyz;
      vec3 q3 = texture(image1, here - dx     ).xyz;
      vec3 q4 = texture(image1, here          ).xyz;
      vec3 q5 = texture(image1, here + dx     ).xyz;
      vec3 q6 = texture(image1, here - dx + dy).xyz;
      vec3 q7 = texture(image1, here      + dy).xyz;
      vec3 q8 = texture(image1, here + dx + dy).xyz;
      vec3 i0 = (q0 + 2*q1 + q2 + 2*q3 + 4*q4 + 2*q5 + q6 + 2*q7 + q8) / 16.0;
      vec3 i1 = texture(image1, here          ).xyz;
      vec3 qo = i1 - i0;
      outColor = vec4(qo * 15 + i1*0.1, 1);
    }
  )");

  fragShader2->addOutput(sga::DataType::Float4, "outColor");
  fragShader2->addSampler("image1");

  auto program2 = sga::Program::createAndCompile(fragShader2);
  auto pipeline2 = sga::FullQuadPipeline::create();
  pipeline2->setProgram(program2);
  pipeline2->setSampler("image1", image1);
  pipeline2->setTarget({image2});


  // Perform render, using both pipelines in sequence
  pipeline1->drawFullQuad();
  pipeline2->drawFullQuad();


  // Save image2 to file.
  std::vector<uint8_t> out(image2->getWidth() * image2->getHeight() * image2->getChannels());
  image2->getData(out);
  stbi_write_png("output.png", image2->getWidth(), image2->getHeight(), 4, out.data(), 0);

  sga::terminate();
}
