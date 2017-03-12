#include <fstream>
#include <sstream>

#include <sga.hpp>

std::vector<std::array<float,2>> vertices = {
  { {-1, -1 } },
  { { 3, -1 } },
  { {-1,  3 } },
};

std::string vertSource = R"(
  void main() {
    gl_Position = vec4(inVertex, 0, 1);
  }
)";

std::string fragMain = R"(
  void main() {
    mainImage(outColor, vec2(gl_FragCoord.x, u.sgaResolution.y - gl_FragCoord.y));
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
  fragSource = ReplaceString(fragSource, "iGlobalTime", "u.sgaTime");
  fragSource = ReplaceString(fragSource, "iResolution", "u.sgaResolution");
  fragSource = ReplaceString(fragSource, "iMouse", "u.stoyMouse");
  fragSource += fragMain;

  // Prepare SGA
  sga::init();
  auto window = sga::Window::create(800, 600, "Shadertoy simulator");
  
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO::create({sga::DataType::Float2}, 3);
  vbo->write(vertices);
  
  auto vertShader = sga::VertexShader::createFromSource(vertSource);
  auto fragShader = sga::FragmentShader::createFromSource(fragSource);
  vertShader->addInput(sga::DataType::Float2, "inVertex");
  fragShader->addOutput(sga::DataType::Float4, "outColor");
  fragShader->addUniform(sga::DataType::Float4, "stoyMouse");

  vertShader->compile();
  fragShader->compile();

  pipeline->setVertexShader(vertShader);
  pipeline->setFragmentShader(fragShader);
  pipeline->setTarget(window);

  bool mouse_l_last = false;
  float click_x = 250.0, click_y = 250.0;
  window->setOnMouseAny([&](double x, double y, bool l, bool){
      if(!mouse_l_last && l){
        click_x = x;
        click_y = y;
      }
      if(!l) x = y = 0;
      std::array<float,4> val = {(float)x,
                                 (float)y,
                                 l ? click_x : -1.0f,
                                 l ? click_y : -1.0f};
      pipeline->setUniform("stoyMouse", val);
      mouse_l_last = l;
    });
  
  window->setFPSLimit(60);
  while(window->isOpen()){
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}
