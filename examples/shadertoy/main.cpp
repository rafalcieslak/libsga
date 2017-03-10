#include <fstream>
#include <sstream>

#include <sga.hpp>

std::vector<std::array<float,2>> vertices = {
  { {-1, -1 } },
  { { 3, -1 } },
  { {-1,  3 } },
};

static std::string ReplaceString(std::string subject, const std::string& search,
                          const std::string& replace) {
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
  std::string source = buffer.str();

  // Prepare shadertoy shader
  source = ReplaceString(source, "iGlobalTime", "u.sgaTime");
  source = ReplaceString(source, "iResolution", "u.sgaResolution");
  source = ReplaceString(source, "iMouse", "vec4(0)");
  source += R"(
    void main() {
      mainImage(outColor, vec2(gl_FragCoord.x, u.sgaResolution.y - gl_FragCoord.y));
    }
  )";

  // Prepare SGA
  sga::init(sga::VerbosityLevel::Debug, sga::ErrorStrategy::MessageThrow);
  auto window = sga::Window::create(800, 600, "Shadertoy simulator");
  auto pipeline = sga::Pipeline::create();

  auto vbo = sga::VBO::create({sga::DataType::Float2}, 3);
  vbo->write(vertices);
  
  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main() {
      gl_Position = vec4(inVertex, 0, 1);
    }
  )");

  auto fragShader = sga::FragmentShader::createFromSource(source);

  vertShader->addInput(sga::DataType::Float2, "inVertex");
  fragShader->addOutput(sga::DataType::Float4, "outColor");

  vertShader->compile();
  fragShader->compile();

  pipeline->setVertexShader(vertShader);
  pipeline->setFragmentShader(fragShader);
  pipeline->setTarget(window);
  
  window->setFPSLimit(60);
  while(window->isOpen()){
    pipeline->drawVBO(vbo);
    window->nextFrame();
  }
  sga::terminate();
}

