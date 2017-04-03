#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#define SGA_USE_GLM
#include <sga.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

struct ModelVertData{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texUV;
};

int main(){
  sga::init();

  // Load model data.
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
        aiVector3D texUV = mesh->mTextureCoords[0][face.mIndices[v]];
        vertices.push_back({
            {vertex.x, vertex.y, vertex.z},
            {normal.x, normal.y, normal.z},
            {texUV.x, texUV.y}    
          });
      }
    }
  }
  std::cout << "Loaded " << vertices.size() << " vertices." << std::endl;
  
  // Read texture image
  int w,h,n;
  unsigned char* data = stbi_load("examples/data/teapot/texture.png", &w, &h, &n, 4);
  if(!data){
    std::cout << "Opening texture failed: " << stbi_failure_reason() << std::endl;
    return 1;
  }
  auto texture = sga::Image::create(w, h);
  texture->putData(std::vector<uint8_t>(data, data + w*h*4));
  
  // Prepare VBO
  auto modelVbo = sga::VBO::create({
      sga::DataType::Float3,
        sga::DataType::Float3,
        sga::DataType::Float2},
    vertices.size());
  modelVbo->write(vertices);

  // Create the window
  auto window = sga::Window::create(800,600,"Deferred shading");
  
  // G-buffer program
  auto GvertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      vec3 pos = in_position + vec3(0, -4, 0);
      gl_Position = u.MVP * vec4(pos,1);
      out_world_normal = in_normal;
      out_world_position = pos;
      out_texUV = in_texUV;
    }
  )");
  auto GfragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      out_position = in_world_position;
      out_normal = in_world_normal;
      // Typicaly: Read material texture.
      // out_albedo = vec3(1.0, 145.0/255, 231.0/255);
      out_albedo = texture(tex, in_texUV).xyz;
    }
  )");
  GvertShader->addInput(sga::DataType::Float3, "in_position");
  GvertShader->addInput(sga::DataType::Float3, "in_normal");
  GvertShader->addInput(sga::DataType::Float2, "in_texUV");
  GvertShader->addOutput(sga::DataType::Float3, "out_world_position");
  GvertShader->addOutput(sga::DataType::Float3, "out_world_normal");
  GvertShader->addOutput(sga::DataType::Float2, "out_texUV");
  GvertShader->addUniform(sga::DataType::Mat4, "MVP");
  GfragShader->addInput(sga::DataType::Float3, "in_world_position");
  GfragShader->addInput(sga::DataType::Float3, "in_world_normal");
  GfragShader->addInput(sga::DataType::Float2, "in_texUV");
  GfragShader->addSampler("tex");
  GfragShader->addOutput(sga::DataType::Float3, "out_position");
  GfragShader->addOutput(sga::DataType::Float3, "out_normal");
  GfragShader->addOutput(sga::DataType::Float3, "out_albedo");
  auto program_gbuffer = sga::Program::createAndCompile(GvertShader, GfragShader);

  // Create buffers
  auto buffer_position = sga::Image::create(window->getWidth(),window->getHeight(), 3, sga::ImageFormat::Float);
  auto buffer_normal   = sga::Image::create(window->getWidth(),window->getHeight(), 3, sga::ImageFormat::Float);
  auto buffer_albedo   = sga::Image::create(window->getWidth(),window->getHeight(), 3, sga::ImageFormat::Float);
  
  // G-buffer pipeline
  auto pipeline_gbuffer = sga::Pipeline::create();
  pipeline_gbuffer->setProgram(program_gbuffer);
  pipeline_gbuffer->setSampler("tex", texture, sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
  pipeline_gbuffer->setFaceCull(sga::FaceCullMode::Back);
  pipeline_gbuffer->setTarget({buffer_position, buffer_normal, buffer_albedo});

  // Lighting program
  auto LfragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
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
  LfragShader->addOutput(sga::DataType::Float4, "out_color");
  LfragShader->addSampler("buffer_position");
  LfragShader->addSampler("buffer_normal");
  LfragShader->addSampler("buffer_albedo");
  LfragShader->addUniform(sga::DataType::Float3, "viewpos");
  LfragShader->addUniform(sga::DataType::Float3, "lightpos");
  auto program_lighting = sga::Program::createAndCompile(LfragShader);

  // Result image
  auto result_image = sga::Image::create(800,600);

  // Lighting pipeline
  auto pipeline_lighting = sga::FullQuadPipeline::create();
  pipeline_lighting->setProgram(program_lighting);
  pipeline_lighting->setSampler("buffer_position", buffer_position);
  pipeline_lighting->setSampler("buffer_normal", buffer_normal);
  pipeline_lighting->setSampler("buffer_albedo", buffer_albedo);
  pipeline_lighting->setTarget(result_image);

  // Window program
  auto WfragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      vec2 here = gl_FragCoord.xy / u.sgaResolution;
      vec2 texpos = mod(here, vec2(0.5)) * 2.0;
      if(u.demo == 0){
         out_color = texture(result_image, here); 
      }else{
        if(here.x < 0.5 && here.y < 0.5)
          out_color = texture(buffer_position, texpos);
        if(here.x >= 0.5 && here.y < 0.5)
          out_color = texture(buffer_normal, texpos);
        if(here.x < 0.5 && here.y >= 0.5)
          out_color = texture(buffer_albedo, texpos);
        if(here.x >= 0.5 && here.y >= 0.5)
          out_color = texture(result_image, texpos);
      }
    }
  )");
  WfragShader->addOutput(sga::DataType::Float4, "out_color");
  WfragShader->addSampler("buffer_position");
  WfragShader->addSampler("buffer_normal");
  WfragShader->addSampler("buffer_albedo");
  WfragShader->addSampler("result_image");
  WfragShader->addUniform(sga::DataType::SInt, "demo");
  auto program_window = sga::Program::createAndCompile(WfragShader);

  // Lighting pipeline
  auto pipeline_window = sga::FullQuadPipeline::create();
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

  std::cout << "Hold SHIFT to switch between display modes." << std::endl;
  pipeline_window->setUniform("demo", 0);
  window->setOnKeyDown(sga::Key::Shift, [&](){
      pipeline_window->setUniform("demo", 1);
    });
  window->setOnKeyUp(sga::Key::Shift, [&](){
      pipeline_window->setUniform("demo", 0);
    });

  window->setOnResize([&](unsigned int w, unsigned int h){
      // Recreate internediate and target images.
      buffer_position = sga::Image::create(w,h, 3, sga::ImageFormat::Float);
      buffer_normal   = sga::Image::create(w,h, 3, sga::ImageFormat::Float);
      buffer_albedo   = sga::Image::create(w,h, 3, sga::ImageFormat::Float);
      result_image = sga::Image::create(w,h);
      // Reset samplers and render targets
      pipeline_gbuffer->setTarget({buffer_position, buffer_normal, buffer_albedo});
      pipeline_lighting->setSampler("buffer_position", buffer_position);
      pipeline_lighting->setSampler("buffer_normal", buffer_normal);
      pipeline_lighting->setSampler("buffer_albedo", buffer_albedo);
      pipeline_lighting->setTarget(result_image);
      pipeline_window->setSampler("buffer_position", buffer_position);
      pipeline_window->setSampler("buffer_normal", buffer_normal);
      pipeline_window->setSampler("buffer_albedo", buffer_albedo);
      pipeline_window->setSampler("result_image", result_image);
      
    });
  
  float view_phi = 0.0;
  float view_theta = M_PI;
  float distance = 12;
  glm::vec3 lightpos = {15.0f, 17.0f, -12.0f};
  
  window->setOnMouseMove([&](double x, double y){
      // Calculate new viewpos and MVP
      x = glm::min(x/window->getWidth(),  0.999);
      y = glm::min(y/window->getHeight(), 0.999);
      view_phi = -glm::radians(180*y - 90);
      view_theta = glm::radians(360*x);
    });

  window->setFPSLimit(60);
  while(window->isOpen()){
    // Compute camera position
    glm::vec3 p = {0, glm::sin(view_phi), glm::cos(view_phi)};
    glm::vec3 viewpos = glm::rotateY(p * distance, view_theta);
    glm::mat4 camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
    glm::mat4 projection = glm::perspectiveFov(
      glm::radians(70.0), (double)window->getWidth(), (double)window->getHeight(), 0.2, distance * 2.1);
    glm::mat4 MVP = projection * camera;

    // Update uniforms
    pipeline_gbuffer->setUniform("MVP", MVP);
    pipeline_lighting->setUniform("lightpos", lightpos);
    pipeline_lighting->setUniform("viewpos", viewpos);

    pipeline_gbuffer->clear();
    pipeline_gbuffer->drawVBO(modelVbo);
    pipeline_lighting->clear();
    pipeline_lighting->drawFullQuad();
    pipeline_window->clear();
    pipeline_window->drawFullQuad();

    window->nextFrame();
  }
  
  sga::terminate();
}

