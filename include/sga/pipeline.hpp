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

  void clear();
  
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
  #include "pipeline.uniforms.inc"
  void setUniform(std::string name, std::initializer_list<float> floats);
  //@}
protected:
  Pipeline();
  void setUniform(DataType dt, std::string name, char* pData, size_t size);
  
  class Impl;
  pimpl_unique_ptr<Impl> impl_;
  Impl* impl() {return impl_.get();}
  const Impl* impl() const {return impl_.get();}
  Pipeline(pimpl_unique_ptr<Impl> &&impl);
};

/* A simplified pipeline, which runs the pixel shader over the entire target area. */
class FullQuadPipeline : public Pipeline{
public:
  ~FullQuadPipeline();
  static std::shared_ptr<FullQuadPipeline> create(){
    return std::shared_ptr<FullQuadPipeline>(new FullQuadPipeline());
  }
  
  void drawFullQuad();
  
  void setProgram(std::shared_ptr<Program>);

  // Forbid some functions from Pipeline which make no sense for FullQuadPipeline
  void drawVBO(std::shared_ptr<VBO>) = delete;
  void setFaceCull(FaceCullMode fcm = FaceCullMode::None, FaceDirection fd = FaceDirection::Clockwise) = delete;
  void setPolygonMode(PolygonMode p) = delete;
  void setRasterizerMode(RasterizerMode r) = delete;
  
protected:
  FullQuadPipeline();
  class Impl;
  Impl* impl();
  const Impl* impl() const;
};

} // namespace sga

#endif // __SGA_PIPELINE_HPP__
