SGA_API void setUniform(std::string name, float value){
  setUniform(DataType::Float, name, (char*)&value, sizeof(value));
}
SGA_API void setUniform(std::string name, int value){
  setUniform(DataType::SInt, name, (char*)&value, sizeof(value));
}
SGA_API void setUniform(std::string name, unsigned int value){
  setUniform(DataType::UInt, name, (char*)&value, sizeof(value));
}
SGA_API void setUniform(std::string name, std::array<float,2> value){
  setUniform(DataType::Float2, name, (char*)&value, sizeof(value));
}
SGA_API void setUniform(std::string name, std::array<float,3> value){
  setUniform(DataType::Float3, name, (char*)&value, sizeof(value));
}
SGA_API void setUniform(std::string name, std::array<float,4> value){
  setUniform(DataType::Float4, name, (char*)&value, sizeof(value));
}
SGA_API void setUniform(std::string name, double value){
  setUniform(DataType::Double, name, (char*)&value, sizeof(value));
}
#ifdef SGA_USE_GLM
  SGA_API void setUniform(std::string name, glm::vec2 value){
    setUniform(DataType::Float2, name, (char*)&value, sizeof(value));
  }
  SGA_API void setUniform(std::string name, glm::vec3 value){
    setUniform(DataType::Float3, name, (char*)&value, sizeof(value));
  }
  SGA_API void setUniform(std::string name, glm::vec4 value){
    setUniform(DataType::Float4, name, (char*)&value, sizeof(value));
  }
  SGA_API void setUniform(std::string name, glm::mat3 value){
    // Careful with mat3's! They use a non-packed layout:
    std::vector<float> tmp = {
      value[0][0], value[1][0], value[2][0], 0,
      value[0][1], value[1][1], value[2][1], 0,
      value[0][2], value[1][2], value[2][2], 0,};
    setUniform(DataType::Mat3, name, (char*)tmp.data(), sizeof(value));
  }
  SGA_API void setUniform(std::string name, glm::mat4 value){
      setUniform(DataType::Mat4, name, (char*)&value, sizeof(value));
  }
#endif

#ifdef SGA_USE_EIGEN
  SGA_API void setUniform(std::string name, Eigen::Vector2f value){
    setUniform(DataType::Float2, name, (char*)value.data(), sizeof(float)*2);
  }
  SGA_API void setUniform(std::string name, Eigen::Vector3f value){
    setUniform(DataType::Float3, name, (char*)value.data(), sizeof(float)*3);
  }
  SGA_API void setUniform(std::string name, Eigen::Vector4f value){
    setUniform(DataType::Float4, name, (char*)value.data(), sizeof(float)*4);
  }
  SGA_API void setUniform(std::string name, Eigen::Matrix<float,3,3> value){
    // Careful with mat3's! They use a non-packed layout:
    std::vector<float> tmp = {
      value(0,0), value(1,0), value(2,0), 0,
      value(0,1), value(1,1), value(2,1), 0,
      value(0,2), value(1,2), value(2,2), 0,};
    setUniform(DataType::Mat3, name, (char*)tmp.data(), sizeof(float)*12);
  }
  SGA_API void setUniform(std::string name, Eigen::Matrix<float,4,4> value){
    setUniform(DataType::Mat4, name, (char*)value.data(), sizeof(float)*16);
  }
#endif

template <typename T>
SGA_API void setUniform(std::string name, T value, DataType dt){
setUniform(dt, name, (char*)&value, sizeof(value));
}
