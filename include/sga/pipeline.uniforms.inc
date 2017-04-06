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
  // TODO: Careful with mat3's! They use a non-packed layout:
  // (1  2  3)  4
  // (5  6  7)  8
  // (9 10 11) 12
  SGA_API void setUniform(std::string name, glm::mat3 value){
    setUniform(DataType::Mat3, name, (char*)&value, sizeof(value));
  }
  SGA_API void setUniform(std::string name, glm::mat4 value){
      setUniform(DataType::Mat4, name, (char*)&value, sizeof(value));
  }
#endif

template <typename T>
SGA_API void setUniform(std::string name, T value, DataType dt){
setUniform(dt, name, (char*)&value, sizeof(value));
}
