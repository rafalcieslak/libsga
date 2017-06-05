#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

#define SGA_USE_GLM
#include <sga.hpp>
#include "../common/common.hpp"

#include <unordered_map>

struct VertData{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct MeshData{
  MeshData(sga::VBO v, glm::vec3 c, std::string t)
    : vbo(v), color(c), texture(t) {}
  sga::VBO vbo;
  glm::vec3 color;
  std::string texture;
};

// Texture bank
std::unordered_map<std::string, sga::Image> textures;

int main(){
  sga::init();

  // Import model.
  Assimp::Importer aimporter;
  std::string filepath =  EXAMPLE_DATA_DIR "sponza/sponza.obj";
  const aiScene* scene = aimporter.ReadFile(filepath, 
                                            aiProcess_Triangulate |
                                            // aiProcess_GenNormals  |
                                            aiProcess_SortByPType);
  if( !scene){
    std::cout << "Failed to open model file " << filepath << ": "
              << aimporter.GetErrorString() << std::endl;
    return 1;
  }
  std::vector<MeshData> meshes;
  for(unsigned int m = 0; m < scene->mNumMeshes; m++){
    const aiMesh* mesh = scene->mMeshes[m];
    std::vector<VertData> vertices;
    for(unsigned int f = 0; f < mesh->mNumFaces; f++){
      aiFace face = mesh->mFaces[f];
      for(unsigned int v = 0; v < 3; v++){
        aiVector3D vertex = mesh->mVertices[face.mIndices[v]];
        aiVector3D normal = mesh->mNormals[face.mIndices[v]];
        aiVector3D uv = mesh->mTextureCoords[0][face.mIndices[v]];
        vertices.push_back({
            {vertex.x, vertex.y, vertex.z},
            {normal.x, normal.y, normal.z},
            {uv.x, uv.y}
          });
      }
    }
    
    // Prepare VBO
    sga::VBO vbo({
        sga::DataType::Float3,
        sga::DataType::Float3,
        sga::DataType::Float2},
      vertices.size());
    vbo.write(vertices);
    
    // Prepare material info
    const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
    aiColor3D c;
    mat->Get(AI_MATKEY_COLOR_DIFFUSE, c);

    aiString diffuse_tex_pathAI;
    mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_tex_pathAI);
    std::string diffuse_tex_path = diffuse_tex_pathAI.C_Str();
    auto it = textures.find(diffuse_tex_path);
    if(diffuse_tex_path!= "" && it == textures.end()){
      std::string full_path = EXAMPLE_DATA_DIR "sponza/" + diffuse_tex_path;
      std::cout << "Loading texture \"" << diffuse_tex_path << "\"" << std::endl;
      int w,h,n;
      stbi_set_flip_vertically_on_load(1);
      unsigned char* data = stbi_load(full_path.c_str(), &w, &h, &n, 4);
      if(!data){
        std::cout << "Opening texture failed: " << stbi_failure_reason() << std::endl;
        return 1;
      }
      sga::Image image(w, h);
      image.putData(std::vector<uint8_t>(data, data + w*h*4));
      textures.insert({diffuse_tex_path, image});
    }
    
    meshes.emplace_back(vbo, (glm::vec3){c.r, c.g, c.b}, diffuse_tex_path);
    std::cout << "Loaded " << vertices.size() << " vertices." << std::endl;
  }

  std::cout << "Use WASD keys and mouselook to move" << std::endl;

  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      vec3 pos = in_position + vec3(0, -4, 0);
      gl_Position = MVP * vec4(pos,1);
      out_world_normal = in_normal;
      out_world_position = pos;
      out_texuv = in_texuv;
    }
  )");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      vec3 Kd = texture(diffuse, in_texuv).xyz;
      out_color = vec4(Kd*0.8, 1.0);
    }
  )");
  
  // Configure shader input/output/uniforms
  vertShader.addInput({{sga::DataType::Float3, "in_position"},
                       {sga::DataType::Float3, "in_normal"},
                       {sga::DataType::Float2, "in_texuv"}});
  vertShader.addOutput({{sga::DataType::Float3, "out_world_position"},
                        {sga::DataType::Float3, "out_world_normal"},
                        {sga::DataType::Float2, "out_texuv"}});
  vertShader.addUniform(sga::DataType::Mat4, "MVP");

  fragShader.addInput(sga::DataType::Float3, "in_world_position");
  fragShader.addInput(sga::DataType::Float3, "in_world_normal");
  fragShader.addInput(sga::DataType::Float2, "in_texuv");
  fragShader.addOutput(sga::DataType::Float4, "out_color");
  fragShader.addSampler("diffuse");
  
  // Prepare window
  sga::Window window(1200, 675, "Teapot");
  window.setFPSLimit(60);
  window.grabMouse();

  glm::vec3 viewpos = {0,0,0};
  glm::vec3 viewdir = {-1,1,0};
  
  // Compute projection matrix
  glm::mat4 projection =
    glm::perspectiveFov( glm::radians(70.0f), (float)window.getWidth(),
                         (float)window.getHeight(), 0.2f, 50.0f);
  
  // Compile shaders
  sga::Program program;
  program.setVertexShader(vertShader);
  program.setFragmentShader(fragShader);
  program.compile();

  // Configure pipeline
  sga::Pipeline pipeline;
  pipeline.setProgram(program);
  pipeline.setTarget(window);
  pipeline.setFaceCull(sga::FaceCullMode::None);

  window.setOnKeyDown(sga::Key::Escape, [&](){
      window.close();
    });

  window.setOnKeyDown(sga::Key::F11, [&](){
      window.toggleFullscreen();
    });

  window.setOnKeyDown(sga::Key::d1, [&](){
      pipeline.setRasterizerMode(sga::RasterizerMode::Filled);
    });
  window.setOnKeyDown(sga::Key::d2, [&](){
      pipeline.setRasterizerMode(sga::RasterizerMode::Wireframe);
    });
  window.setOnKeyDown(sga::Key::d3, [&](){
      pipeline.setRasterizerMode(sga::RasterizerMode::Points);
    });

  window.setOnResize([&](double w, double h){
      projection = glm::perspectiveFov(glm::radians(70.0), w, h, 0.2, 50.0);
    });
  
  window.setOnMouseMove([&](double x, double y){
      // Calculate new viewpos and MVP
      float mouse_speed = 0.002;
      float phi = glm::clamp(-90.0*y*mouse_speed, -89.99, 89.99);
      float theta = 90.0f*x*mouse_speed;
      phi = glm::radians(phi);
      theta = glm::radians(theta);
      glm::vec3 p = {0, glm::sin(phi), glm::cos(phi)};
      viewdir = glm::rotateY(p, theta);
    });
  
  // Main loop
  while(window.isOpen()){
    // Process user movement
    float playerspeed = 2.5;
    if(window.isKeyPressed(sga::Key::Shift))
      playerspeed *= 2.5;
    glm::vec3 tangent = glm::normalize(glm::cross({0,-1,0}, viewdir));
    if(window.isKeyPressed(sga::Key::W))
      viewpos += viewdir * playerspeed * window.getLastFrameDelta();
    if(window.isKeyPressed(sga::Key::S))
      viewpos -= viewdir * playerspeed * window.getLastFrameDelta();
    if(window.isKeyPressed(sga::Key::A))
      viewpos += tangent * playerspeed * window.getLastFrameDelta();
    if(window.isKeyPressed(sga::Key::D))
      viewpos -= tangent * playerspeed * window.getLastFrameDelta();
    
    glm::mat4 camera = glm::lookAt(viewpos, viewpos + viewdir, {0,-1,0});
    glm::mat4 MVP = projection * camera;
    pipeline.setUniform("MVP", MVP);
      
    // Render scene
    pipeline.clear();
    for(const MeshData& mesh : meshes){
      if(mesh.texture != ""){
        auto it = textures.find(mesh.texture);
        pipeline.setSampler("diffuse", it->second, sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
      } else {
        // ???
      }
      pipeline.drawVBO(mesh.vbo);
    }
    window.nextFrame();
  }
  
  sga::terminate();
}
