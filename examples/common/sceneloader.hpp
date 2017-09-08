#define SGA_USE_GLM
#include <sga.hpp>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../common/stb_image.h"

#include "../common/common.hpp"

#include <string>
#include <iostream>
#include <unordered_map>

struct VertData{
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct MeshData{
  MeshData(sga::VBO v, std::string tn)
    : vbo(v), texture_name(tn) {}
  sga::VBO vbo;
  glm::vec3 diffuse_color;
  glm::vec3 ambient_color;
  std::string texture_name;
};

class Scene{
public:
  Scene(){
  }
  
  std::string scene_dir;
  // Meshes in scene
  std::vector<MeshData> meshes;
  // Texture bank
  std::unordered_map<std::string, sga::Image> textures;
  
  bool load(std::string scenename, bool generate_normals = false, bool flipxz = false, bool normalize = true){
    std::string filepath = getScenePath(scenename);
    scene_dir = filepath.substr(0, filepath.find_last_of("/\\"));
    
    Assimp::Importer aimporter;
    aimporter.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, normalize);
    const aiScene* scene = aimporter.ReadFile(filepath,
                                              aiProcess_Triangulate |
                                              aiProcess_PreTransformVertices |
                                              (generate_normals ? aiProcess_GenNormals : 0) |
                                              aiProcess_SortByPType);
    
    if(!scene){
      std::cout << "Failed to open model file " << filepath << ": "
                << aimporter.GetErrorString() << std::endl;
      return false;
    }

    
    for(unsigned int m = 0; m < scene->mNumMeshes; m++){
      const aiMesh* mesh = scene->mMeshes[m];
      std::vector<VertData> vertices;
      for(unsigned int f = 0; f < mesh->mNumFaces; f++){
        aiFace face = mesh->mFaces[f];
        for(unsigned int v = 0; v < 3; v++){
          aiVector3D vertex = mesh->mVertices[face.mIndices[v]];
          aiVector3D normal = mesh->mNormals[face.mIndices[v]];
          aiVector3D uv = mesh->mTextureCoords[0][face.mIndices[v]];
          if(!flipxz){
            vertices.push_back({
                {5*vertex.x, 5*vertex.y, 5*vertex.z},
                {normal.x, normal.y, normal.z},
                {uv.x, uv.y}
              });
          }else{
            vertices.push_back({
                {vertex.x, vertex.z, -vertex.y},
                {normal.x, normal.z, -normal.y},
                {uv.x, uv.y}
              });
          }
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
      aiColor3D cd;
      mat->Get(AI_MATKEY_COLOR_DIFFUSE, cd);
      aiColor3D ca;
      mat->Get(AI_MATKEY_COLOR_AMBIENT, ca);

      aiString diffuse_tex_pathAI;
      mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_tex_pathAI);
      std::string diffuse_tex_path = diffuse_tex_pathAI.C_Str();
      auto it = textures.find(diffuse_tex_path);
      std::cout << "texpath: " << diffuse_tex_path << std::endl;
      if(diffuse_tex_path!= "" && it == textures.end()){
        std::string full_path = scene_dir + "/" + diffuse_tex_path;
        std::cout << "Loading texture \"" << diffuse_tex_path << "\"" << std::endl;
        int w,h,n;
        stbi_set_flip_vertically_on_load(1);
        unsigned char* data = stbi_load(full_path.c_str(), &w, &h, &n, 4);
        if(!data){
          std::cout << "Opening texture '" << full_path << "' failed: " << stbi_failure_reason() << std::endl;
          return false;
        }
        sga::Image image(w, h, 4, sga::ImageFormat::NInt8, sga::ImageFilterMode::Anisotropic);
        image.putData(std::vector<uint8_t>(data, data + w*h*4));
        textures.insert({diffuse_tex_path, image});
      }

      meshes.emplace_back(vbo, diffuse_tex_path);
      meshes.back().diffuse_color = glm::vec3(cd.r, cd.g, cd.b);
      std::cout << "Loaded " << vertices.size() << " vertices." << std::endl;
    }
    return true;
  }

  static std::string getScenePath(std::string scenename){
    // TODO: Search for the model in several hard-coded locations.
    return EXAMPLE_DATA_DIR + scenename;
  }
  
};
