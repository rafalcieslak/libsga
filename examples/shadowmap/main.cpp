#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>


#define SGA_USE_GLM
#include <sga.hpp>

#include "../common/sceneloader.hpp"

int main(int argc, char** argv){

  std::string filepath;
  if(argc < 2 || std::string(argv[1]) == ""){
    std::cout << "Usage: " << argv[0] << " MODEL_NAME" << std::endl;
    return 1;
  }else if (std::string(argv[1]) == "sponza"){
    filepath = "sponza-shadowmap/sponza.obj";
  }else if (std::string(argv[1]) == "castle"){
    filepath = "castle-on-hills/castle-on-hills.fbx";
  }else{
    std::cout << "Unknown model " << argv[1] << ", choose between 'sponza' and 'castle'" << std::endl;
    return 1;
  }
  
  sga::init();
  
  Scene scene;
  bool loaded = scene.load(filepath, true);
  if(!loaded)
    return 1;
  
  // Done loading scene.
  std::cout << "Use WASD keys and mouselook to move. Hold SPACE to preview shadow map." << std::endl;

  // View and light source parameters.
  glm::vec3 viewpos = {1.5,0.5,0};
  glm::vec3 viewdir = {-1,0,0};
  float viewnear = 0.1f, viewfar = 10.0f;
  float viewfov = 70.0f;
  glm::vec3 lightposA = {0, 3.0, 0};
  glm::vec3 lightposB = {-1.0, 0, 0.25};
  glm::vec3 lightlookat = {0, 0, 0};
  float lightnear = -8.0f, lightfar = 8.0f;
  float shadowmap_size = 4096, shadowmap_range = 6.0f;
  sga::Image shadowmap(shadowmap_size, shadowmap_size, 1, sga::ImageFormat::Float, sga::ImageFilterMode::None);
  glm::mat4 shadowmapProj = glm::ortho(-shadowmap_range, shadowmap_range, -shadowmap_range, shadowmap_range, lightnear, lightfar);

  // Prepare window
  sga::Window window(1200, 900, "Shadowmap");
  //window.setFPSLimit(60);
  window.grabMouse();
  window.setClearColor(sga::ImageClearColor::NInt8(150,180,200));

  // This vertex shader is used both by main pipeline and shadow map.
  auto mainVertShader = sga::VertexShader  ::createFromFile(EXAMPLE_DATA_DIR "/shadowmap/model.vert");
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
  auto fragShader = sga::FragmentShader::createFromFile(EXAMPLE_DATA_DIR "/shadowmap/model.frag");
  fragShader.addInput(sga::DataType::Float3, "in_world_position");
  fragShader.addInput(sga::DataType::Float3, "in_world_normal");
  fragShader.addInput(sga::DataType::Float2, "in_texuv");
  fragShader.addInput(sga::DataType::Float3, "in_shadowmap");
  fragShader.addOutput(sga::DataType::Float4, "out_color");
  fragShader.addUniform(sga::DataType::Float3, "world_lightpos");
  fragShader.addUniform(sga::DataType::Float3, "world_viewpos");
  fragShader.addUniform(sga::DataType::Float3, "color_diffuse");
  fragShader.addUniform(sga::DataType::SInt, "use_texture");
  fragShader.addUniform(sga::DataType::SInt, "debug");
  fragShader.addSampler("diffuse");
  fragShader.addSampler("shadowmap");

  auto shadowmapFragShader = sga::FragmentShader::createFromFile(EXAMPLE_DATA_DIR "/shadowmap/shadow.frag");
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
  sga::Image no_image = sga::Image(16, 16);
  pipeline.setSampler("diffuse", no_image);
  pipeline.setUniform("color_diffuse", glm::vec3{0.0f, 0.0f, 0.0f});
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
    float playerspeed = 1.5;
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
    glm::vec3 lightpos = lightposA + glm::rotate(lightposB, (float)sga::getTime()/7.0f + 1.8f, glm::vec3(0,1,0));
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
    for(const MeshData& mesh : scene.meshes)
      shadowmapPipeline.draw(mesh.vbo);

    // Render scene
    pipeline.clear();
    #define REPEAT 1
    for(int i = 0; i < REPEAT; i++){
      for(const MeshData& mesh : scene.meshes){
        if(mesh.texture_name != ""){
          // Set texture sampler to use
          auto it = scene.textures.find(mesh.texture_name);
          pipeline.setUniform("use_texture", 1);
          pipeline.setSampler("diffuse", it->second, sga::SamplerInterpolation::Linear, sga::SamplerWarpMode::Repeat);
        }else{
          pipeline.setUniform("use_texture", 0);
          pipeline.setUniform("color_diffuse", mesh.diffuse_color);
        }
        pipeline.draw(mesh.vbo);
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
