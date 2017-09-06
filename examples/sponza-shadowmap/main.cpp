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

std::string model_dir = EXAMPLE_DATA_DIR "sponza-shadowmap";
std::string model_file = "sponza.obj";

int main(){
  sga::init();

  // Import model.
  Assimp::Importer aimporter;
  aimporter.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);
  std::string filepath =  model_dir + "/" + model_file;
  const aiScene* scene = aimporter.ReadFile(filepath,
                                            aiProcess_Triangulate |
                                            aiProcess_PreTransformVertices |
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
            {15*vertex.x, 15*vertex.y, 15*vertex.z},
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
    std::cout << "texpath: " << diffuse_tex_path << std::endl;
    if(diffuse_tex_path!= "" && it == textures.end()){
      std::string full_path = model_dir + "/" + diffuse_tex_path;
      std::cout << "Loading texture \"" << diffuse_tex_path << "\"" << std::endl;
      int w,h,n;
      stbi_set_flip_vertically_on_load(1);
      unsigned char* data = stbi_load(full_path.c_str(), &w, &h, &n, 4);
      if(!data){
        std::cout << "Opening texture '" << full_path << "' failed: " << stbi_failure_reason() << std::endl;
        return 1;
      }
      sga::Image image(w, h, 4, sga::ImageFormat::NInt8, sga::ImageFilterMode::Anisotropic);
      image.putData(std::vector<uint8_t>(data, data + w*h*4));
      textures.insert({diffuse_tex_path, image});
      // TODO: Bumpmaps?
    }

    meshes.emplace_back(vbo, (glm::vec3){c.r, c.g, c.b}, diffuse_tex_path);
    std::cout << "Loaded " << vertices.size() << " vertices." << std::endl;
  }

  // Done loading scene.
  std::cout << "Use WASD keys and mouselook to move. Hold SPACE to preview shadow map." << std::endl;

  // View and light source parameters.
  glm::vec3 viewpos = {5,2,0};
  glm::vec3 viewdir = {-1,0,0};
  float viewnear = 0.1f, viewfar = 30.0f;
  float viewfov = 70.0f;
  glm::vec3 lightposA = {0, 25, 0};
  glm::vec3 lightposB = {-20, 0, 4};
  glm::vec3 lightlookat = {0, 0, 0};
  float lightnear = -5.0f, lightfar = 60.0f;
  float shadowmap_size = 2048, shadowmap_range = 10.0f;
  sga::Image shadowmap(shadowmap_size, shadowmap_size, 1, sga::ImageFormat::Float, sga::ImageFilterMode::None);
  glm::mat4 shadowmapProj = glm::ortho(-shadowmap_range, shadowmap_range, -shadowmap_range, shadowmap_range, lightnear, lightfar);

  // Prepare window
  sga::Window window(1200, 900, "Sponza");
  //window.setFPSLimit(60);
  window.grabMouse();
  window.setClearColor(sga::ImageClearColor::NInt8(150,180,200));

  // This vertex shader is used both by main pipeline and shadow map.
  auto mainVertShader = sga::VertexShader  ::createFromFile(EXAMPLE_DATA_DIR "/sponza-shadowmap/model.vert");
  mainVertShader.addInput(sga::DataType::Float3, "in_position");
  mainVertShader.addInput(sga::DataType::Float3, "in_normal");
  mainVertShader.addInput(sga::DataType::Float2, "in_texuv");
  mainVertShader.addOutput(sga::DataType::Float3, "out_world_position");
  mainVertShader.addOutput(sga::DataType::Float3, "out_world_normal");
  mainVertShader.addOutput(sga::DataType::Float2, "out_texuv");
  mainVertShader.addOutput(sga::DataType::Float3, "out_shadowmap");
  mainVertShader.addUniform(sga::DataType::Mat4, "MVP");
  mainVertShader.addUniform(sga::DataType::Mat4, "shadowmapMVP");

  // Main fragment shader for the scene.
  auto fragShader = sga::FragmentShader::createFromFile(EXAMPLE_DATA_DIR "/sponza-shadowmap/model.frag");
  fragShader.addInput(sga::DataType::Float3, "in_world_position");
  fragShader.addInput(sga::DataType::Float3, "in_world_normal");
  fragShader.addInput(sga::DataType::Float2, "in_texuv");
  fragShader.addInput(sga::DataType::Float3, "in_shadowmap");
  fragShader.addOutput(sga::DataType::Float4, "out_color");
  fragShader.addUniform(sga::DataType::Float3, "world_lightpos");
  fragShader.addUniform(sga::DataType::Float3, "world_viewpos");
  fragShader.addUniform(sga::DataType::SInt, "debug");
  fragShader.addSampler("diffuse");
  fragShader.addSampler("shadowmap");

  auto shadowmapFragShader = sga::FragmentShader::createFromFile(EXAMPLE_DATA_DIR "/sponza-shadowmap/shadow.frag");
  shadowmapFragShader.addInput(sga::DataType::Float3, "in_world_position");
  shadowmapFragShader.addInput(sga::DataType::Float3, "in_world_normal");
  shadowmapFragShader.addInput(sga::DataType::Float2, "in_texuv");
  shadowmapFragShader.addInput(sga::DataType::Float3, "in_shadowmap");
  shadowmapFragShader.addOutput(sga::DataType::Float, "out_depth");

  // Shadowmap render pass
  sga::Program shadowmapProgram = sga::Program::createAndCompile(mainVertShader, shadowmapFragShader);
  sga::Pipeline shadowmapPipeline;
  shadowmapPipeline.setProgram(shadowmapProgram);
  shadowmapPipeline.setTarget(shadowmap);
  shadowmapPipeline.setFaceCull(sga::FaceCullMode::Back);

  // Main render pass
  glm::mat4 projection = glm::perspectiveFov(glm::radians(viewfov), (float)window.getWidth(),
                                             (float)window.getHeight(), viewnear, viewfar);
  sga::Program program = sga::Program::createAndCompile(mainVertShader, fragShader);
  sga::Pipeline pipeline;
  pipeline.setProgram(program);
  pipeline.setTarget(window);
  pipeline.setFaceCull(sga::FaceCullMode::Back);
  pipeline.setSampler("shadowmap", shadowmap);
  pipeline.setUniform("debug", 0);

  // Shadowmap preview render pass
  sga::FullQuadPipeline previewPipeline;
  auto previewShader = sga::FragmentShader::createFromFile(EXAMPLE_DATA_DIR "/sponza-shadowmap/preview.frag");
  previewShader.addSampler("shadowmap");
  previewShader.addOutput(sga::DataType::Float4,"out_color");
  previewPipeline.setProgram(sga::Program::createAndCompile(previewShader));
  previewPipeline.setTarget(window);
  previewPipeline.setSampler("shadowmap", shadowmap);
  previewPipeline.setViewport(0, 0, 512, 512);

  window.setOnKeyDown(sga::Key::Escape, [&](){
      window.close();
    });

  window.setOnKeyDown(sga::Key::F11, [&](){
      window.toggleFullscreen();
    });

  window.setOnResize([&](double w, double h){
      projection = glm::perspectiveFov(glm::radians(viewfov), (float)w, (float)h, viewnear, viewfar);
    });

  window.setOnMouseMove([&](double x, double y){
      // Calculate new viewpos and MVP
      float mouse_speed = 0.002;
      float phi = glm::clamp(-90.0*y*mouse_speed, -89.99, 89.99);
      float theta = 90.0f*x*mouse_speed - 90;
      glm::vec3 p = {0, glm::sin(glm::radians(phi)), glm::cos(glm::radians(phi))};
      viewdir = glm::rotateY(p, glm::radians(theta));
    });

  // Main loop
  while(window.isOpen()){

    // Process user movement
    float playerspeed = 4.5;
    glm::vec3 playermove(0.0f, 0.0f, 0.0f);
    if(window.isKeyPressed(sga::Key::Shift))
      playerspeed *= 2.5;
    glm::vec3 tangent = glm::normalize(glm::cross({0,-1,0}, viewdir));
    if(window.isKeyPressed(sga::Key::W)) playermove += viewdir;
    if(window.isKeyPressed(sga::Key::S)) playermove -= viewdir;
    if(window.isKeyPressed(sga::Key::A)) playermove += tangent;
    if(window.isKeyPressed(sga::Key::D)) playermove -= tangent;
    if(glm::length(playermove) > 0.0f)
      viewpos += glm::normalize(playermove) * playerspeed * window.getLastFrameDelta();

    // Update shadowmap MVPs
    glm::mat4 camera = glm::lookAt(viewpos, viewpos + viewdir, {0,-1,0});
    glm::mat4 MVP = projection * camera;
    glm::vec3 lightpos = lightposA + glm::rotate(lightposB, (float)sga::getTime()/11.0f + 1.8f, glm::vec3(0,1,0));
    glm::vec3 lightdir = glm::normalize(lightlookat - lightpos);
    glm::mat4 shadowmapCamera = glm::lookAt(lightpos, lightpos + lightdir, {0,-1,0});
    glm::mat4 shadowmapMVP = shadowmapProj * shadowmapCamera;

    // Apply uniforms
    shadowmapPipeline.setUniform("MVP", shadowmapMVP);
    shadowmapPipeline.setUniform("shadowmapMVP", shadowmapMVP);
    pipeline.setUniform("MVP", MVP);
    pipeline.setUniform("shadowmapMVP", shadowmapMVP);
    pipeline.setUniform("world_viewpos", viewpos);
    pipeline.setUniform("world_lightpos", lightpos);

    // Render shadowmap
    shadowmapPipeline.clear();
    for(const MeshData& mesh : meshes)
      shadowmapPipeline.drawVBO(mesh.vbo);

    // Render scene
    pipeline.clear();
    #define REPEAT 1
    for(int i = 0; i < REPEAT; i++){
      for(const MeshData& mesh : meshes){
        if(mesh.texture != ""){
          // Set texture sampler to use
          auto it = textures.find(mesh.texture);
          pipeline.setSampler("diffuse", it->second, sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
        }
        pipeline.drawVBO(mesh.vbo);
      }
    }
    if(window.isKeyPressed(sga::Key::Space))
      previewPipeline.drawFullQuad();
    window.nextFrame();


    if(window.getFrameNo() % 60 == 0)
      std::cout << "FPS: " << window.getAverageFPS() << ", frame time (ms): " << window.getAverageFrameTime()*1000 << std::endl;
  }

  sga::terminate();
}
