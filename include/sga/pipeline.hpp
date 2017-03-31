#ifndef __SGA_PIPELINE_HPP__
#define __SGA_PIPELINE_HPP__

#include "config.hpp"
#include "window.hpp"
#include "layout.hpp"

namespace sga{

class VBO;
class VertexShader;
class FragmentShader;
class Program;
class Image;

enum class SamplerInterpolation{
  Nearest,
  Linear,
};

enum class SamplerWarpMode{
  Clamp,
  Repeat,
  Mirror
};

enum class FaceCullMode{
  None,
  Back,
  Front,
};

enum class FaceDirection{
  Clockwise,
  CounterClockwise
};

enum class PolygonMode{
  Points,
  Lines,
  LineStrip,
  Triangles,
  TriangleStrip,
  TriangleFan,
};

enum class RasterizerMode{
  Filled,
  Wireframe,
  Points,
};

/** This class represents the state and configuration of a rendering
    pipeline. Once it is configured, it may then be used for rendering onto a
    window or image surface.
*/
class Pipeline{
public:
  ~Pipeline();

  /** Creates a new unconfigured Pipeline. */
  static std::shared_ptr<Pipeline> create(){
    return std::shared_ptr<Pipeline>(new Pipeline());
  }
  
  /** Configures the pipeline to render onto the provided window. */
  void setTarget(std::shared_ptr<Window> window);
  
  void setTarget(std::shared_ptr<Image> image) {setTarget({image});}
  void setTarget(std::initializer_list<std::shared_ptr<Image>> images) {
    setTarget(std::vector<std::shared_ptr<Image>>(images));
  }
  void setTarget(std::vector<std::shared_ptr<Image>> images);
  
  /** Performs rendering using vertex data from the provided VBO. This method
      blocks until rendering completes. Before rendering pipeline configuration
      is valdiated, and you will be notified of any errors or
      inconsistencies. */
  void drawVBO(std::shared_ptr<VBO>);

  /** Configures the clear color of this pipeline, i.e. the color used for
      clearing the target surface before at the beginning of a render. */
  void setClearColor(float r, float g, float b);

  void setProgram(std::shared_ptr<Program>);

  void setSampler(std::string, std::shared_ptr<Image>,
                  SamplerInterpolation interpolation = SamplerInterpolation::Linear,
                  SamplerWarpMode warp_mode = SamplerWarpMode::Clamp);

  void setFaceCull(FaceCullMode fcm = FaceCullMode::None, FaceDirection fd = FaceDirection::Clockwise);
  
  void setPolygonMode(PolygonMode p);
  void setRasterizerMode(RasterizerMode r);
  
  //@{
  /** Sets the value of a named uniform within this pipeline to the provided
      value. The uniform name must correspond to a uniform previously declared
      during shader preparation. These methods will refuse to set a standard
      uniform (`u.sga*`). Using these methods is insanely fast. All changes are
      cached and writes are postponed until next render. */
  void setUniform(std::string name, float value){
    setUniform(DataType::Float, name, (char*)&value, sizeof(value));
  }
  void setUniform(std::string name, int value){
    setUniform(DataType::Int, name, (char*)&value, sizeof(value));
  }
  void setUniform(std::string name, unsigned int value){
    setUniform(DataType::UInt, name, (char*)&value, sizeof(value));
  }
  void setUniform(std::string name, std::array<float,2> value){
    setUniform(DataType::Float2, name, (char*)&value, sizeof(value));
  }
  void setUniform(std::string name, std::array<float,3> value){
    setUniform(DataType::Float3, name, (char*)&value, sizeof(value));
  }
  void setUniform(std::string name, std::array<float,4> value){
    setUniform(DataType::Float4, name, (char*)&value, sizeof(value));
  }
  void setUniform(std::string name, double value){
    setUniform(DataType::Double, name, (char*)&value, sizeof(value));
  }
#ifdef SGA_USE_GLM
  void setUniform(std::string name, glm::vec2 value){
    setUniform(DataType::Float2, name, (char*)&value, sizeof(value));
  }
  void setUniform(std::string name, glm::vec3 value){
    setUniform(DataType::Float3, name, (char*)&value, sizeof(value));
  }
  void setUniform(std::string name, glm::vec4 value){
    setUniform(DataType::Float4, name, (char*)&value, sizeof(value));
  }
  // TODO: Careful with mat3's! They use a non-packed layout:
  // (1  2  3)  4
  // (5  6  7)  8
  // (9 10 11) 12
  void setUniform(std::string name, glm::mat3 value){
    setUniform(DataType::Mat3, name, (char*)&value, sizeof(value));
  }
  void setUniform(std::string name, glm::mat4 value){
    setUniform(DataType::Mat4, name, (char*)&value, sizeof(value));
  }
#endif
  void setUniform(std::string name, std::initializer_list<float> floats);


  template <typename T>
  void setUniform(std::string name, T value, DataType dt){
    setUniform(dt, name, (char*)&value, sizeof(value));
  }
  //@}
private:
  Pipeline();
  void setUniform(DataType dt, std::string name, char* pData, size_t size);
  
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_PIPELINE_HPP__
