#include <sga.hpp>
#include "../common/common.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"
#include <cassert>

#define PI 3.14159265359

/* This example loads a lightprobe circular HDR enviroment map image, and
 * displays it in equirectangular format. For each pixel of the output, it
 * samples 64 points on the input image. This is beacause of a non-affine
 * wrapping transform from polar to cartesian coordinates. Multisampling ensures
 * proper surface area integration, and is still very fast on a GP */

#define SS_SCALE 8

int main(){
  sga::init();
  auto window = sga::Window::create(500, 250, "Unwrapped envmap");
  
  std::string filepath =  EXAMPLE_DATA_DIR "thepit.hdr";

  // Load image from .hdr file
  int w,h,n;
  float *file_data = stbi_loadf(filepath.c_str(), &w, &h, &n, 4);
  std::vector<float> data(file_data, file_data + w*h*4);
  sga::Image lightprobe_image(w,h,4,sga::ImageFormat::Float,sga::ImageFilterMode::None);
  lightprobe_image.putData(data);
  free(file_data);

  // TODO: Compute scaling factor as partition of data floats, and pass it as uniform.
  auto fragShader = sga::FragmentShader::createFromSource(R"(
  #define PI 3.14159265359
  vec3 sample_lightprobe(vec3 dir){
    dir = normalize(dir);
    float r = (1/PI) * acos(dir.z) / sqrt(dir.x * dir.x + dir.y * dir.y);
    vec2 p = vec2(dir.x*r, dir.y*r);
    return texture(lightprobe, p/2 + vec2(0.5)).xyz;
  }
  void main(){
    vec2 coords = sgaWindowCoords;
    float phi = coords.x * 2 * PI, theta = (1.0 - coords.y) * PI;
    phi += phi_offset;
    if(abs(phi_offset) < 0.0000001)
      phi += 0.2 * sgaTime;
    float scale = 0.12;
    vec3 dir = vec3(sin(theta)*cos(phi), cos(theta), sin(theta)*sin(phi));
    dir = normalize(dir);
    vec4 outColorA = vec4(sample_lightprobe(dir) / scale, 1.0);
    vec4 outColorB = vec4(dir, 1.0);
    outColor = mix(outColorA, outColorB, 0.0);
  }
  )");
  fragShader->addOutput(sga::DataType::Float4, "outColor");
  fragShader->addUniform(sga::DataType::Float, "phi_offset");
  fragShader->addSampler("lightprobe");

  sga::Image image(window->getWidth()*SS_SCALE,
                   window->getHeight()*SS_SCALE,
                   4, sga::ImageFormat::NInt8,
                   sga::ImageFilterMode::MipMapped);
  
  auto program = sga::Program::createAndCompile(fragShader);
  auto pipeline = sga::FullQuadPipeline::create();
  pipeline->setTarget(image);
  pipeline->setProgram(program);
  pipeline->setUniform("phi_offset",0.0f);
  pipeline->setSampler("lightprobe", lightprobe_image);
  
  window->setOnKeyDown(sga::Key::Escape, [&](){ window->close(); });
  window->setOnKeyDown(sga::Key::F11, [&](){ window->toggleFullscreen(); });

  float phi_offset = 0.0;
  bool mouse_down = false;
  double mouse_drag_start = 0.0, phi_offset_drag_start = 0.0;
  double mouse_pos = 0.0;
  window->setOnMouseButton([&](bool left, bool){
      if(!mouse_down && left){
        mouse_drag_start = mouse_pos;
        phi_offset_drag_start = phi_offset;
      }
      mouse_down = left;
    });
  window->setOnMouseMove([&](double x, double){
      mouse_pos = x;
      if(mouse_down){
        phi_offset = phi_offset_drag_start + (mouse_drag_start - mouse_pos) * 2*PI / window->getWidth();
      }
      pipeline->setUniform("phi_offset", phi_offset);
    });
  
  // Downsample program
  auto downsample_shader = sga::FragmentShader::createFromSource(R"(
    void main(){
      outColor = texture(image, sgaWindowCoords);
    }
  )");
  downsample_shader->addOutput(sga::DataType::Float4, "outColor");
  downsample_shader->addSampler("image");
  auto downsample_program = sga::Program::createAndCompile(downsample_shader);
  
  // Downsample pipeline
  auto downsample_pipeline = sga::FullQuadPipeline::create();
  downsample_pipeline->setProgram(downsample_program);
  downsample_pipeline->setTarget(window);
  downsample_pipeline->setSampler("image", image);
  
  std::cout << "HINT: Drag the image left-right with mouse." << std::endl;
  
  while(window->isOpen()){
    pipeline->clear();
    pipeline->drawFullQuad();
    downsample_pipeline->drawFullQuad();
    window->nextFrame();
  }
  sga::terminate();

}
