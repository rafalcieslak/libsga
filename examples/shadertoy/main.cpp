#include <fstream>
#include <sstream>

#include <sga.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

std::string fragMain = R"(
  void main() {
    mainImage(outColor, vec2(gl_FragCoord.x, sgaResolution.y - gl_FragCoord.y));
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
  fragSource = ReplaceString(fragSource, "iGlobalTime", "sgaTime");
  fragSource = ReplaceString(fragSource, "iResolution", "sgaResolution");
  fragSource = ReplaceString(fragSource, "iMouse", "stoyMouse");
  fragSource += fragMain;

  // Prepare SGA
  sga::init();
  sga::Window window(640*2, 360*2, "Shadertoy simulator");
  
  // Really wish we could use getopt, but it's posix-only!
  std::vector<std::string> textures;
  for(int i = 2; i < argc; i++)
    textures.push_back(argv[i]);

  // Load images
  stbi_set_flip_vertically_on_load(1);
  std::vector<sga::Image> images;
  for(int i = 0; i < 4; i++){
    if(i < (int)textures.size()){
      int w,h,n;
      unsigned char* data = stbi_load(textures[i].c_str(), &w, &h, &n, 4);
      if(!data){
        std::cout << "Opening texture failed: " << stbi_failure_reason() << std::endl;
        return 1;
      }
      images.push_back(sga::Image(w, h));
      images.back().putData(std::vector<uint8_t>(data, data + w*h*4));
    }else{
      images.push_back(sga::Image(16,16)); // Some empty texture.
    }
  }
  
  auto shader = sga::FragmentShader::createFromSource(fragSource);
  shader.addOutput(sga::DataType::Float4, "outColor");
  shader.addUniform(sga::DataType::Float4, "stoyMouse");
  shader.addSampler("iChannel0");
  shader.addSampler("iChannel1");
  shader.addSampler("iChannel2");
  shader.addSampler("iChannel3");

  auto program = sga::Program::createAndCompile(shader);

  sga::FullQuadPipeline pipeline;
  pipeline.setProgram(program);
  pipeline.setTarget(window);
  pipeline.setUniform("stoyMouse", {0,0,-1,-1});
  pipeline.setSampler("iChannel0", images[0], sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
  pipeline.setSampler("iChannel1", images[1], sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
  pipeline.setSampler("iChannel2", images[2], sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
  pipeline.setSampler("iChannel3", images[3], sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);

  bool mouse_l_last = false;
  float click_x = 250.0, click_y = 250.0;
  window.setOnMouseAny([&](double x, double y, bool l, bool){
      if(!mouse_l_last && l){
        click_x = x;
        click_y = y;
      }
      if(!l) x = y = 0;
      pipeline.setUniform("stoyMouse",
                          {(float)x,
                           (float)y,
                           l ? click_x : -1.0f,
                           l ? click_y : -1.0f});
      mouse_l_last = l;
    });

  window.setOnKeyDown(sga::Key::Escape, [&](){
      window.close();
    });
  window.setOnKeyDown(sga::Key::F11, [&](){
      window.toggleFullscreen();
    });
  
  window.setFPSLimit(60);
  while(window.isOpen()){
    pipeline.drawFullQuad();
    window.nextFrame();
    
    if(window.getFrameNo() % 20 == 0)
      std::cout << window.getAverageFPS() << std::endl;
  }
  sga::terminate();
}
