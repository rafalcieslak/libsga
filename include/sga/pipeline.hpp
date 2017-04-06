#ifndef __SGA_PIPELINE_HPP__
#define __SGA_PIPELINE_HPP__

#include <array>
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
  SGA_API ~Pipeline();

  /** Creates a new unconfigured Pipeline. */
  SGA_API static std::shared_ptr<Pipeline> create(){
    return std::shared_ptr<Pipeline>(new Pipeline());
  }
  
  /** Configures the pipeline to render onto the provided window. */
  SGA_API void setTarget(std::shared_ptr<Window> window);
  
  SGA_API void setTarget(std::shared_ptr<Image> image) {setTarget({image});}
  SGA_API void setTarget(std::initializer_list<std::shared_ptr<Image>> images) {
    setTarget(std::vector<std::shared_ptr<Image>>(images));
  }
  SGA_API void setTarget(std::vector<std::shared_ptr<Image>> images);
  
  /** Performs rendering using vertex data from the provided VBO. This method
      blocks until rendering completes. Before rendering pipeline configuration
      is valdiated, and you will be notified of any errors or
      inconsistencies. */
  SGA_API void drawVBO(std::shared_ptr<VBO>);

  /** Configures the clear color of this pipeline, i.e. the color used for
      clearing the target surface before at the beginning of a render. */
  SGA_API void setClearColor(float r, float g, float b);

  SGA_API void clear();
  
  SGA_API void setProgram(std::shared_ptr<Program>);

  SGA_API void setSampler(std::string, std::shared_ptr<Image>,
                  SamplerInterpolation interpolation = SamplerInterpolation::Linear,
                  SamplerWarpMode warp_mode = SamplerWarpMode::Clamp);

  SGA_API void setFaceCull(FaceCullMode fcm = FaceCullMode::None, FaceDirection fd = FaceDirection::Clockwise);
  
  SGA_API void setPolygonMode(PolygonMode p);
  SGA_API void setRasterizerMode(RasterizerMode r);
  
  //@{
  /** Sets the value of a named uniform within this pipeline to the provided
      value. The uniform name must correspond to a uniform previously declared
      during shader preparation. These methods will refuse to set a standard
      uniform (`u.sga*`). Using these methods is insanely fast. All changes are
      cached and writes are postponed until next render. */
  #include "pipeline.uniforms.inc"
  SGA_API void setUniform(std::string name, std::initializer_list<float> floats);
  //@}
protected:
  SGA_API Pipeline();
  SGA_API void setUniform(DataType dt, std::string name, char* pData, size_t size);
  
  class Impl;
  pimpl_unique_ptr<Impl> impl_;
  SGA_API Impl* impl() {return impl_.get();}
  SGA_API const Impl* impl() const {return impl_.get();}
  SGA_API Pipeline(pimpl_unique_ptr<Impl> &&impl);
};

/* A simplified pipeline, which runs the pixel shader over the entire target area. */
class FullQuadPipeline : public Pipeline{
public:
  SGA_API ~FullQuadPipeline();
  SGA_API static std::shared_ptr<FullQuadPipeline> create(){
    return std::shared_ptr<FullQuadPipeline>(new FullQuadPipeline());
  }
  
  SGA_API void drawFullQuad();
  
  SGA_API void setProgram(std::shared_ptr<Program>);

  // Forbid some functions from Pipeline which make no sense for FullQuadPipeline
  SGA_API void drawVBO(std::shared_ptr<VBO>) = delete;
  SGA_API void setFaceCull(FaceCullMode fcm = FaceCullMode::None, FaceDirection fd = FaceDirection::Clockwise) = delete;
  SGA_API void setPolygonMode(PolygonMode p) = delete;
  SGA_API void setRasterizerMode(RasterizerMode r) = delete;
  
protected:
  SGA_API FullQuadPipeline();
  class Impl;
  SGA_API Impl* impl();
  const Impl* impl() const;
};

} // namespace sga

#endif // __SGA_PIPELINE_HPP__
