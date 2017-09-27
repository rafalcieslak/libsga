#include <Eigen/Dense>
using Eigen::MatrixXf;

#define SGA_USE_EIGEN
#include <sga.hpp>
#include "../common/common.hpp"

#include "../common/json.hpp"
using json = nlohmann::json;

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

#include <iostream>
#include <fstream>
#include <cassert>

inline bool file_exists(std::string path) {
  return std::ifstream(path).good();
}

void usage(char** argv){
  std::cout << "USAGE: " << argv[0] << " CONFIG_FILE" << std::endl;
}

Eigen::Matrix3f find_homography(std::vector<Eigen::Vector2f> points1, std::vector<Eigen::Vector2f> points2){
  assert(points1.size() == points2.size());
  Eigen::MatrixXf A = Eigen::MatrixXf::Zero(points1.size()*2,9);
  for(unsigned int i = 0; i < points1.size(); i++){
    float x1 = points1[i](0), y1 = points1[i](1);
    float x2 = points2[i](0), y2 = points2[i](1);
    Eigen::Matrix<float, 2, 9> Q;
    Q <<
      x1, y1, 1,  0,  0, 0, -x1*x2, -y1*x2, -x2,
      0 , 0 , 0, x1, y1, 1, -x1*y2, -y1*y2, -y2;
    A.block(i*2,0,2,9) = Q;
  }
  
  Eigen::JacobiSVD<Eigen::MatrixXf> svd(A, Eigen::ComputeThinU | Eigen::ComputeFullV);
  unsigned int P = svd.matrixV().cols();
  Eigen::Map<const Eigen::Matrix3f> view(svd.matrixV().block(0,P-1,9,1).data(), 3, 3);
  Eigen::Matrix3f H = view.transpose() / view(2,2);
  
  return H;
}

int main(int argc, char** argv){
  if(argc < 2) {
    usage(argv);
    return 1;
  }
  std::string config_path = argv[1];
  size_t q = config_path.find_last_of("/\\");
  std::string config_dir = config_path.substr(0,q);
  std::string output_path = "output.png";
  
  // Enable sga
  sga::init();
  renderdoc_tryenable();
  renderdoc_capture_start();
  
  // Open and parse config file
  std::ifstream config_file(config_path);
  if(!config_file.good()){
    std::cout << "Failed to open file \"" << config_path << "\"" << std::endl;
    return 1;
  }
  std::vector<std::string> image_paths;
  std::vector<std::pair<std::vector<Eigen::Vector2f>, std::vector<Eigen::Vector2f>>> points;
  unsigned int center = 0;
  unsigned int N;
  bool render_lines = false;
  try{
    json config;
    config_file >> config;
    auto images = config["images"];
    if(!images.is_array()){
      std::cout << "Json config must contain an array of strings \"images\" representing a list of input images." << std::endl;
      return 1;
    }
    for(auto& q : images){
      image_paths.push_back(q);
    }
    N = image_paths.size();
    if(config.count("center") > 0){
      center = config["center"];
    }
    if(config.count("lines") > 0){
      render_lines = config["lines"] != 0;
    }
    auto pointlist = config["points"];
    if(!pointlist.is_array() || pointlist.size() != N-1){
      std::cout << "Json config must contain an array of point coordinates \"points\" of the lentgh equal to the number of images minus one (" << N-1 << ")." << std::endl;
      return 1;
    }
    for(auto& pointspair : pointlist){
      std::vector<Eigen::Vector2f> pl;
      std::vector<Eigen::Vector2f> pr;
      if(pointspair.size() != 2){
        std::cout << "Each entry in \"points\" array must have two elements (a list of points in image N and a list of corresponding points on image N+1)" << std::endl;
        return 1;
      }
      for(auto& point : pointspair[0]){
        Eigen::Vector2f p;
        if(point.size() != 2){
          std::cout << "Each point must be an array of 2 numbers." << std::endl;
          return 1;
        }
        p(0) = point[0]; p(1) = point[1];
        pl.push_back(p);
      }
      for(auto& point : pointspair[1]){
        Eigen::Vector2f p;
        if(point.size() != 2){
          std::cout << "Each point must be an array of 2 numbers." << std::endl;
          return 1;
        }
        p(0) = point[0]; p(1) = point[1];
        pr.push_back(p);
      }
      points.push_back(std::make_pair(pl,pr));
    }
  }catch (json::exception& e){
    std::cout << "Error reading json config: " << e.what() << std::endl;
  }
  
  // Print out configuration
  /*
  for(unsigned int i = 0; i < N; i++){
    std::cout << image_paths[i] << std::endl;
    if(i < N-1){
      for(auto p : points[i].first){
        std::cout << "[" << p(0) << ", " << p(1) << "] ";
      }
      std::cout << std::endl;
      for(auto p : points[i].second){
        std::cout << "[" << p(0) << ", " << p(1) << "] ";
      }
      std::cout << std::endl;
    }
  }
  */

  // Compute homographies
  std::cout << "Preparing Hs" << std::endl;
  std::vector<Eigen::Matrix3f> Hs;
  Eigen::Matrix3f H0 = Eigen::Matrix3f::Identity(3,3);
  Hs.push_back(H0);

  for(unsigned int i = center+1; i < N; i++){
    std::cout << "Computing H between " << i-1 << " and " << i << std::endl;
    auto Hdiff = find_homography(points[i-1].first, points[i-1].second);
    auto Habs = Hdiff * Hs.back();
    Hs.push_back(Habs);
  }
  for(int i = center-1; i > -1; i--){
    std::cout << "Computing H between " << i+1 << " and " << i << std::endl;
    auto Hdiff = find_homography(points[i].second, points[i].first);
    auto Habs = Hdiff * Hs.front();
    Hs.insert(Hs.begin(), Habs);
  }
  assert(Hs.size() == N);

  // Compute inverses
  std::vector<Eigen::Matrix3f> His;
  for(const auto& H : Hs)
    His.push_back(H.inverse());

  // Load images
  std::cout << "Loading images" << std::endl;
  std::vector<sga::Image> images;
  for(std::string image_path : image_paths){
    image_path = config_dir + "/" + image_path;

    int w,h,n;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(image_path.c_str(), &w, &h, &n, 4);
    if(!data){
      std::cout << "Opening image '" << image_path << "' failed: " << stbi_failure_reason() << std::endl;
      return 1;
    }
    sga::Image image(w, h, 4, sga::ImageFormat::NInt8, sga::ImageFilterMode::Anisotropic);
    image.putData(std::vector<uint8_t>(data, data + w*h*4));
    free(data);

    images.push_back(image);
  }

  std::cout << "Preparing render resources" << std::endl;
  
  // Prepare image bounding boxes
  Eigen::ArrayXXf imgBBs(4*N,3);
  for(unsigned int i = 0; i < N; i++){
    auto& p = images[i];
    auto& Hi = His[i];
    Eigen::Vector3f v0(0,0,1);
    Eigen::Vector3f v1(0,p.getHeight(),1);
    Eigen::Vector3f v2(p.getWidth(),0,1);
    Eigen::Vector3f v3(p.getWidth(),p.getHeight(),1);
    imgBBs.block(4*i + 0, 0, 1, 3) = (Hi*v0).transpose();
    imgBBs.block(4*i + 1, 0, 1, 3) = (Hi*v1).transpose();
    imgBBs.block(4*i + 2, 0, 1, 3) = (Hi*v2).transpose();
    imgBBs.block(4*i + 3, 0, 1, 3) = (Hi*v3).transpose();
  }
  imgBBs.colwise() /= imgBBs.col(2);
  Eigen::Vector3f BBmax, BBmin;
  BBmax = imgBBs.colwise().maxCoeff();
  BBmin = imgBBs.colwise().minCoeff();
  Eigen::Vector3f BBsize = BBmax - BBmin;
  Eigen::Vector3f offset = BBmin;
  std::cout << "offset " << offset << std::endl;

  // Create target image
  sga::Image result(BBsize(0), BBsize(1));

  // Prepare a VBO
  struct VertData{
    Eigen::Vector2f pos;
  };
  std::vector<VertData> vertices = {
    {{0,0}},{{0,1}},{{1,0}},
    {{1,1}},{{1,0}},{{0,1}}
  };
  sga::VBO vbo({sga::DataType::Float2}, vertices.size());
  vbo.write(vertices);

  // Prepare shaders
  auto vertShader = sga::VertexShader::createFromSource(R"(
    mat3 align = mat3(vec3(2, 0, 0),vec3(0, 2, 0),vec3(-1, -1, 1));
    void main(){
      vec2 pos_texturespace = in_position * textureSize(image,0);
      vec3 pos_transformed_homog = H * vec3(pos_texturespace,1);
      float perspective_factor = 1/pos_transformed_homog.z;
      vec2 pos_transformed = pos_transformed_homog.xy/pos_transformed_homog.z;
      vec2 pos3 = 2*(pos_transformed - offset)/sgaResolution.xy - 1;
      gl_Position = vec4(pos3,0,1);
      imageUVW = vec3(in_position.x, 1 - in_position.y, 1) * perspective_factor;
    }
  )");
  vertShader.addInput(sga::DataType::Float2, "in_position");
  vertShader.addOutput(sga::DataType::Float3, "imageUVW");
  vertShader.addSampler("image");
  vertShader.addUniform(sga::DataType::Mat3, "H");
  vertShader.addUniform(sga::DataType::Float2, "offset");
  auto fragShader = sga::FragmentShader::createFromSource(R"(
    void main(){
      out_color = textureProj(image, imageUVW);
    }
  )");
  fragShader.addOutput(sga::DataType::Float4, "out_color");
  fragShader.addInput(sga::DataType::Float3, "imageUVW");
  fragShader.addSampler("image");
  auto program = sga::Program::createAndCompile(vertShader,fragShader);

  // Prepare pipeline
  sga::Pipeline pipeline;
  pipeline.setProgram(program);
  pipeline.setTarget({result});
  pipeline.setFaceCull(sga::FaceCullMode::None);
  pipeline.setBlendModeColor(sga::BlendFactor::One, sga::BlendFactor::OneMinusSrcAlpha);
  pipeline.setBlendModeAlpha(sga::BlendFactor::One, sga::BlendFactor::OneMinusSrcAlpha);
  pipeline.clear();

  std::cout << "Rendering" << std::endl;
  
  // Draw!
  pipeline.setUniform("offset", {offset(0), offset(1)});
  for(unsigned int i = 0; i < N; i++){
    pipeline.setSampler("image", images[i]);
    pipeline.setUniform("H", His[i]);
    std::cout << His[i] << std::endl;
    pipeline.drawVBO(vbo);
  }

  // Prepare another pipeline, just for rendering image edges
  if(render_lines){
    auto linesFragShader = sga::FragmentShader::createFromSource(R"(
      void main(){out_color = vec4(1,0,0,0.2);}
    )");
    linesFragShader.addOutput(sga::DataType::Float4, "out_color");
    linesFragShader.addInput(sga::DataType::Float3, "imageUVW");
    auto lines_program = sga::Program::createAndCompile(vertShader,linesFragShader);
    sga::Pipeline lines_pipeline;
    lines_pipeline.setPolygonMode(sga::PolygonMode::LineStrip);
    lines_pipeline.setLineWidth(2.0);
    lines_pipeline.setTarget(result);
    lines_pipeline.setProgram(lines_program);
    lines_pipeline.setBlendModeColor(sga::BlendFactor::One, sga::BlendFactor::OneMinusSrcAlpha);
    lines_pipeline.setBlendModeAlpha(sga::BlendFactor::One, sga::BlendFactor::OneMinusSrcAlpha);
  
    // VBO for lines
    std::vector<VertData> lines_vertices = {
      {{0,0}},{{0,1}},{{1,1}},{{1,0}},{{0,0}}
    };
    sga::VBO lines_vbo({sga::DataType::Float2}, lines_vertices.size());
    lines_vbo.write(lines_vertices);
  
    lines_pipeline.setUniform("offset", {offset(0), offset(1)});
    for(unsigned int i = 0; i < N; i++){
      lines_pipeline.setSampler("image", images[i]);
      lines_pipeline.setUniform("H", His[i]);
      lines_pipeline.drawVBO(lines_vbo);
    }
  }
    
  // Save result
  std::cout << "Saving result to " << output_path << std::endl;
  result.savePNG(output_path);

  renderdoc_capture_end();

  sga::terminate();
}
