#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "../common/sceneloader.hpp"

int main(){
  sga::init();
  
  Scene scene;
  bool loaded = scene.load("castle-on-hills/castle-on-hills.fbx", true);
  //bool loaded = scene.load("kokura/kokura.obj", true, true);
  if(!loaded)
    return 1;

  
  auto vertShader = sga::VertexShader::createFromSource(R"(
    void main(){
      gl_Position = MVP * vec4(in_position,1);
      out_world_normal = in_normal;
      out_world_position = in_position;
    }
  )");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      vec3 N = normalize(in_world_normal);
      vec3 L = normalize(lightpos - in_world_position);
      vec3 R = normalize(-reflect(L, N));
      vec3 E = normalize(viewpos - in_world_position);

      vec3 Ks = vec3(1.0);
      vec3 a = Kd * 0.2;
      vec3 d = Kd * max(0, dot(L, N));
      vec3 s = Ks * pow(max(0, dot(R, E)), 50.0);
      s = clamp(s, 0.0, 1.0);

      out_color = vec4((a + d + s) * 0.86, 1.0);
    }
  )");
  
  // Configure shader input/output/uniforms
  vertShader.addInput({{sga::DataType::Float3, "in_position"},
                       {sga::DataType::Float3, "in_normal"},
                       {sga::DataType::Float2, "in_uv"}});
  vertShader.addOutput({{sga::DataType::Float3, "out_world_position"},
                         {sga::DataType::Float3, "out_world_normal"}});
  vertShader.addUniform(sga::DataType::Mat4, "MVP");

  fragShader.addInput(sga::DataType::Float3, "in_world_position");
  fragShader.addInput(sga::DataType::Float3, "in_world_normal");
  fragShader.addOutput(sga::DataType::Float4, "out_color");
  fragShader.addUniform(sga::DataType::Float3, "lightpos");
  fragShader.addUniform(sga::DataType::Float3, "viewpos");
  fragShader.addUniform(sga::DataType::Float3, "Kd");
  fragShader.addUniform(sga::DataType::Float3, "Ka");

  // Prepare window
  sga::Window window(800, 600, "Teapot");
  window.setFPSLimit(60);
  
  // Compute initial MVP
  float distance = 7;
  glm::vec3 viewpos = {0,0,-distance};
  glm::mat4 camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
  glm::mat4 projection = glm::perspectiveFov(glm::radians(70.0), 800.0, 600.0, 0.2, distance * 2.1);
  
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
  
  pipeline.setUniform("lightpos", {distance*1.3f, distance*0.6f, -distance});
  
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

  glm::tvec2<double> viewmouse = {0.0, 0.0};
  glm::tvec2<double> viewmouse_target = {0.0, 0.0};
  glm::tvec2<double> viewmouse_drag_start = {0.0, 0.0};
  
  auto recalculateMVP = [&](){
      // Calculate new viewpos and MVP
      double x = viewmouse.x;
      double y = viewmouse.y;
      float phi = -glm::radians(180*y);
      float theta = glm::radians(360*x);
      glm::vec3 p = {0, glm::sin(phi), glm::cos(phi)};
      viewpos = glm::rotateY(p * distance, theta);
      camera = glm::lookAt(viewpos, {0,0,0}, {0,-1,0});
      glm::mat4 MVP = projection * camera;
      pipeline.setUniform("MVP", MVP);
  };

  window.setOnResize([&](double w, double h){
      projection = glm::perspectiveFov(glm::radians(70.0), w, h, 0.2, distance * 2.1);
    });

  glm::tvec2<double> mousedown_pos;
  glm::tvec2<double> mouse_pos;
  bool mouse_down = false;
  window.setOnMouseButton([&](bool left, bool){
      if(left && ! mouse_down){
        mouse_down = true;
        // Left press
        mousedown_pos = mouse_pos;
        viewmouse_drag_start = viewmouse_target;
      }else if(!left && mouse_down){
        mouse_down = false;
        // Left release
      }
    });
  
  window.setOnMouseMove([&](double x, double y){
      mouse_pos = {x/window.getWidth(),y/window.getHeight()};
      if(mouse_down){
        auto mouse_delta = mouse_pos - mousedown_pos;
        viewmouse_target = viewmouse_drag_start + mouse_delta;
        viewmouse_target.y = glm::max(viewmouse_target.y, -0.5 + 0.001);
        viewmouse_target.y = glm::min(viewmouse_target.y,  0.5 - 0.001);
      }
    });
  
  // Main loop
  while(window.isOpen()){
    double theta = glm::pow(0.12, window.getLastFrameDelta());
    viewmouse = viewmouse * theta + viewmouse_target * (1.0 - theta);
    recalculateMVP();
  
    pipeline.clear();
    pipeline.setUniform("viewpos", viewpos);
    for(auto& mesh : scene.meshes){
      pipeline.setUniform("Kd", mesh.diffuse_color);
      pipeline.setUniform("Ka", mesh.ambient_color);
      pipeline.drawVBO(mesh.vbo);
    }
    window.nextFrame();
  }
  
  sga::terminate();
}
