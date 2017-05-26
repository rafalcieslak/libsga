#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define SGA_USE_GLM
#include <sga.hpp>
#include "../common/common.hpp"

struct VertData{
  glm::vec3 pos;
  glm::vec3 normal;
};

/* This is a variant of the "teapot" example, but it renders to a
 * high-resolution target, and then downsamples it to produce an anti-aliased
 * output (SSAA) */

#define SSAA_RATIO 4

int main(){
  sga::init();

  // Import model.
  Assimp::Importer aimporter;
  std::string filepath =  EXAMPLE_DATA_DIR "teapot/teapot.obj";
  const aiScene* scene = aimporter.ReadFile(filepath, 
                                            aiProcess_Triangulate |
                                            aiProcess_GenNormals  |
                                            aiProcess_SortByPType);
  if( !scene){
    std::cout << "Failed to open model file " << filepath << ": "
              << aimporter.GetErrorString() << std::endl;
    return 1;
  }
  std::vector<std::shared_ptr<sga::VBO>> vbos;
  for(unsigned int m = 0; m < scene->mNumMeshes; m++){
    const aiMesh* mesh = scene->mMeshes[m];
    std::vector<VertData> vertices;
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
    // Prepare VBO
    auto vbo = sga::VBO::create({
        sga::DataType::Float3,
          sga::DataType::Float3},
      vertices.size());
    vbo->write(vertices);
    vbos.push_back(vbo);
    std::cout << "Loaded " << vertices.size() << " vertices." << std::endl;
  }

  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      vec3 pos = in_position + vec3(0, -4, 0);
      gl_Position = MVP * vec4(pos,1);
      out_world_normal = in_normal;
      out_world_position = pos;
    }
  )");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      vec3 N = normalize(in_world_normal);
      vec3 L = normalize(lightpos - in_world_position);
      vec3 R = normalize(-reflect(L, N));
      vec3 E = normalize(viewpos - in_world_position);

      vec3 Kd = vec3(1.0, 145.0/255, 231.0/255);
      vec3 Ks = vec3(1.0);
      vec3 a = Kd * 0.08;
      vec3 d = Kd * max(0, dot(L, N));
      vec3 s = Ks * pow(max(0, dot(R, E)), 20.0);
      s = clamp(s, 0.0, 1.0);

      out_color = vec4((a + d + s) * 0.86, 1.0);
    }
  )");
  
  // Configure shader input/output/uniforms
  vertShader->addInput({{sga::DataType::Float3, "in_position"},
                        {sga::DataType::Float3, "in_normal"}});
  vertShader->addOutput({{sga::DataType::Float3, "out_world_position"},
                         {sga::DataType::Float3, "out_world_normal"}});
  vertShader->addUniform(sga::DataType::Mat4, "MVP");

  fragShader->addInput(sga::DataType::Float3, "in_world_position");
  fragShader->addInput(sga::DataType::Float3, "in_world_normal");
  fragShader->addOutput(sga::DataType::Float4, "out_color");
  fragShader->addUniform(sga::DataType::Float3, "lightpos");
  fragShader->addUniform(sga::DataType::Float3, "viewpos");

  // Prepare window
  auto window = sga::Window::create(800, 600, "Teapot");
  window->setFPSLimit(60);
  
  // Compute initial MVP
  float distance = 12;
  glm::vec3 viewpos = {0,0,-distance};
  glm::mat4 camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
  glm::mat4 projection = glm::perspectiveFov(glm::radians(70.0), 800.0, 600.0, 0.2, distance * 2.1);
  glm::mat4 MVP = projection * camera;
  
  // Compile shaders
  auto program = sga::Program::create();
  program->setVertexShader(vertShader);
  program->setFragmentShader(fragShader);
  program->compile();

  sga::Image image(window->getWidth()*SSAA_RATIO,
                   window->getHeight()*SSAA_RATIO,
                   4, sga::ImageFormat::NInt8,
                   sga::ImageFilterMode::MipMapped);
  
  // Configure main pipeline
  auto pipeline = sga::Pipeline::create();
  pipeline->setProgram(program);
  pipeline->setTarget(image);
  pipeline->setFaceCull(sga::FaceCullMode::None);
  
  pipeline->setUniform("MVP", MVP);
  pipeline->setUniform("lightpos", {distance*1.3f, distance*0.6f, -distance});
  
  window->setOnKeyDown(sga::Key::Escape, [&](){
      window->close();
    });

  window->setOnKeyDown(sga::Key::F11, [&](){
      window->toggleFullscreen();
    });

  window->setOnKeyDown(sga::Key::d1, [&](){
      pipeline->setRasterizerMode(sga::RasterizerMode::Filled);
    });
  window->setOnKeyDown(sga::Key::d2, [&](){
      pipeline->setRasterizerMode(sga::RasterizerMode::Wireframe);
    });
  window->setOnKeyDown(sga::Key::d3, [&](){
      pipeline->setRasterizerMode(sga::RasterizerMode::Points);
    });

  window->setOnResize([&](double w, double h){
      projection = glm::perspectiveFov(glm::radians(70.0), w, h, 0.2, distance * 2.1);
      MVP = projection * camera;
      pipeline->setUniform("MVP", MVP);
    });
  
  window->setOnMouseMove([&](double x, double y){
      // Calculate new viewpos and MVP
      x = glm::min(x/window->getWidth(),  0.999);
      y = glm::min(y/window->getHeight(), 0.999);
      float phi = -glm::radians(180*y - 90);
      float theta = glm::radians(360*x);
      glm::vec3 p = {0, glm::sin(phi), glm::cos(phi)};
      viewpos = glm::rotateY(p * distance, theta);
      camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
      MVP = projection * camera;
      pipeline->setUniform("MVP", MVP);
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
  
  // Main loop
  while(window->isOpen()){
    pipeline->clear();
    pipeline->setUniform("viewpos", viewpos);
    pipeline->drawVBO(vbos[0]);
    pipeline->drawVBO(vbos[1]);
    downsample_pipeline->drawFullQuad();
    window->nextFrame();
  }
  
  sga::terminate();
}
