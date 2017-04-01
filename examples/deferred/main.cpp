#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <chrono>
#include <thread>

#define SGA_USE_GLM
#include <sga.hpp>

struct __attribute__((packed)) QuadVertData{
  float position[2];
};

std::vector<QuadVertData> quadVertices = {
  { {-1, -1 } },
  { { 3, -1 } },
  { {-1,  3 } },
};

struct ModelVertData{
  glm::vec3 pos;
  glm::vec3 normal;
};

int main(){
  sga::init();

  // Import model.
  Assimp::Importer aimporter;
  std::string filepath =  "./examples/data/teapot/teapot-smooth.obj";
  const aiScene* scene = aimporter.ReadFile(filepath, 
                                            aiProcess_Triangulate |
                                            aiProcess_GenNormals  |
                                            aiProcess_SortByPType);
  if( !scene){
    std::cout << "Failed to open model file " << filepath << ": "
              << aimporter.GetErrorString() << std::endl;
    return 1;
  }
  std::vector<ModelVertData> vertices;
  for(unsigned int m = 0; m < scene->mNumMeshes; m++){
    const aiMesh* mesh = scene->mMeshes[m];
    for(unsigned int f = 0; f < mesh->mNumFaces; f++){
      aiFace face = mesh->mFaces[f];
      for(unsigned int v = 0; v < 3; v++){
        aiVector3D vertex = mesh->mVertices[face.mIndices[v]];
        aiVector3D normal = mesh->mNormals[face.mIndices[v]];
        vertices.push_back({
            {vertex.x, vertex.y, vertex.z},
            {normal.x, normal.y, normal.z}
          });
      }
    }
  }
  std::cout << "Loaded " << vertices.size() << " vertices." << std::endl;
  
  // Prepare VBOs
  auto modelVbo = sga::VBO::create({
      sga::DataType::Float3,
      sga::DataType::Float3},
    vertices.size());
  modelVbo->write(vertices);
  
  auto quadVbo = sga::VBO::create({
      sga::DataType::Float2},
    quadVertices.size());
  quadVbo->write(quadVertices);

  // G-buffer program
  auto GvertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      vec3 pos = in_position + vec3(0, -40, 0);
      pos = pos / 10.0;
      gl_Position = u.MVP * vec4(pos,1);
      out_world_normal = in_normal;
      out_world_position = pos;
    }
  )");
  auto GfragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      out_position = vec4(in_world_position, 1.0);
      out_normal = vec4(in_world_normal, 1.0);
      // Typicaly: Read material texture.
      out_albedo = vec4(1.0, 145.0/255, 231.0/255, 1.0);
    }
  )");
  GvertShader->addInput(sga::DataType::Float3, "in_position");
  GvertShader->addInput(sga::DataType::Float3, "in_normal");
  GvertShader->addOutput(sga::DataType::Float3, "out_world_position");
  GvertShader->addOutput(sga::DataType::Float3, "out_world_normal");
  GvertShader->addUniform(sga::DataType::Mat4, "MVP");
  GfragShader->addInput(sga::DataType::Float3, "in_world_position");
  GfragShader->addInput(sga::DataType::Float3, "in_world_normal");
  GfragShader->addOutput(sga::DataType::Float4, "out_position");
  GfragShader->addOutput(sga::DataType::Float4, "out_normal");
  GfragShader->addOutput(sga::DataType::Float4, "out_albedo");
  auto program_gbuffer = sga::Program::createAndCompile(GvertShader, GfragShader);

  // Create buffers
  auto buffer_position = sga::Image::create(800,600, 3, sga::ImageFormat::Float);
  auto buffer_normal   = sga::Image::create(800,600, 3, sga::ImageFormat::Float);
  auto buffer_albedo   = sga::Image::create(800,600, 3, sga::ImageFormat::Float);
  
  // G-buffer pipeline
  auto pipeline_gbuffer = sga::Pipeline::create();
  pipeline_gbuffer->setProgram(program_gbuffer);
  pipeline_gbuffer->setFaceCull(sga::FaceCullMode::Back);
  pipeline_gbuffer->setTarget({buffer_position, buffer_normal, buffer_albedo});

  // Lighting program
  auto LvertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      gl_Position = vec4(inVertex, 0, 1);
    }
  )");
  auto LfragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      vec2 here = gl_FragCoord.xy / u.sgaResolution;
      vec3 position = texture(buffer_position, here).xyz;
      vec3 normal   = normalize(texture(buffer_normal,   here).xyz);
      vec3 albedo   = texture(buffer_albedo,   here).xyz;

      vec3 L = normalize(u.lightpos - position);
      vec3 R = -reflect(L, normal);
      vec3 E = normalize(u.viewpos - position);

      vec3 a = albedo * 0.08;
      vec3 d = albedo * max(0, dot(L, normal));
      vec3 s = vec3(0.8) * pow(max(0, dot(R, E)), 20.0);

      out_color = vec4((a + d + s) * 0.86, 1.0);

    }
  )");
  LvertShader->addInput(sga::DataType::Float2, "inVertex");
  LfragShader->addOutput(sga::DataType::Float4, "out_color");
  LfragShader->addSampler("buffer_position");
  LfragShader->addSampler("buffer_normal");
  LfragShader->addSampler("buffer_albedo");
  LfragShader->addUniform(sga::DataType::Float3, "viewpos");
  LfragShader->addUniform(sga::DataType::Float3, "lightpos");
  auto program_lighting = sga::Program::createAndCompile(LvertShader, LfragShader);

  // Result image
  auto result_image = sga::Image::create(800,600);

  // Lighting pipeline
  auto pipeline_lighting = sga::Pipeline::create();
  pipeline_lighting->setProgram(program_lighting);
  pipeline_lighting->setSampler("buffer_position", buffer_position);
  pipeline_lighting->setSampler("buffer_normal", buffer_normal);
  pipeline_lighting->setSampler("buffer_albedo", buffer_albedo);
  pipeline_lighting->setTarget(result_image);

  // Window program
  auto WvertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      gl_Position = vec4(inVertex, 0, 1);
    }
  )");
  auto WfragShader = sga::FragmentShader::createFromSource(R"(
    void main()
    {
      vec2 here = gl_FragCoord.xy / u.sgaResolution;

      vec2 texpos = mod(here, vec2(0.5)) * 2.0;
      // texpos = here;

      vec3 s = vec3(1.0, 0.0, 0.0);
      if(here.x < 0.5 && here.y < 0.5)
        s = texture(buffer_position, texpos).xyz;
      if(here.x >= 0.5 && here.y < 0.5)
        s = texture(buffer_normal, texpos).xyz;
      if(here.x < 0.5 && here.y >= 0.5)
        s = texture(buffer_albedo, texpos).xyz;
      if(here.x >= 0.5 && here.y >= 0.5)
        s = texture(result_image, texpos).xyz;

      out_color = vec4(s, 1.0);
    }
  )");
  WvertShader->addInput(sga::DataType::Float2, "inVertex");
  WfragShader->addOutput(sga::DataType::Float4, "out_color");
  WfragShader->addSampler("buffer_position");
  WfragShader->addSampler("buffer_normal");
  WfragShader->addSampler("buffer_albedo");
  WfragShader->addSampler("result_image");
  auto program_window = sga::Program::createAndCompile(WvertShader, WfragShader);

  // The window
  auto window = sga::Window::create(800,600,"Deferred shading");

  // Lighting pipeline
  auto pipeline_window = sga::Pipeline::create();
  pipeline_window->setProgram(program_window);
  pipeline_window->setSampler("buffer_position", buffer_position);
  pipeline_window->setSampler("buffer_normal", buffer_normal);
  pipeline_window->setSampler("buffer_albedo", buffer_albedo);
  pipeline_window->setSampler("result_image", result_image);
  pipeline_window->setTarget(window);


  window->setOnKeyDown(sga::Key::Escape, [&](){
      window->close();
    });

  window->setOnKeyDown(sga::Key::F11, [&](){
      window->toggleFullscreen();
    });
  
  float phi = 0.0;
  float theta = M_PI;
  float distance = 12;
  glm::vec3 lightpos = {15.0f, 7.0f, -12.0f};
  
  window->setOnMouseMove([&](double x, double y){
      // Calculate new viewpos and MVP
      x = glm::min(x/window->getWidth(),  0.999);
      y = glm::min(y/window->getHeight(), 0.999);
      phi = -glm::radians(180*y - 90);
      theta = glm::radians(360*x);
    });

  window->setFPSLimit(60);
  while(window->isOpen()){
    // Compute camera position
    glm::vec3 p = {0, glm::sin(phi), glm::cos(phi)};
    glm::vec3 viewpos = glm::rotateY(p * distance, theta);
    glm::mat4 camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
    glm::mat4 projection = glm::perspectiveFov(glm::radians(70.0), 800.0, 600.0, 0.2, distance * 2.1);
    glm::mat4 MVP = projection * camera;
    
    pipeline_gbuffer->setUniform("MVP", MVP);
    pipeline_lighting->setUniform("lightpos", lightpos*3.0f);
    pipeline_lighting->setUniform("viewpos", viewpos);

    pipeline_gbuffer->drawVBO(modelVbo);
    pipeline_lighting->drawVBO(quadVbo);
    pipeline_window->drawVBO(quadVbo);

    window->nextFrame();
  }
  
  sga::terminate();
}

