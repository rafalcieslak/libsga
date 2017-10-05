#ifndef __SGA_PIPELINE_HPP__
#define __SGA_PIPELINE_HPP__

#include <array>
#include "config.hpp"
#include "window.hpp"
#include "layout.hpp"
#include "image.hpp"

namespace sga{

class VBO;
class IBO;
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

enum class BlendFactor{
  Zero,
  One,
  SrcAlpha,
  DstAlpha,
  OneMinusSrcAlpha,
  OneMinusDstAlpha,
  // TODO: Are any other factors generally useful?
};

enum class BlendOperation{
  Add,
  Subtract,
  ReverseSubtract,
  Min,
  Max
};

/** This class represents the state and configuration of a rendering
    pipeline. Once it is configured, it may then be used for rendering onto a
    window or image surface.
*/
class Pipeline{
public:
  SGA_API Pipeline();
  SGA_API ~Pipeline();

  /** Configures the pipeline to render onto the provided window. */
  SGA_API void setTarget(const Window& window);

  SGA_API void setTarget(const Image& image) {setTarget({image});}
  SGA_API void setTarget(std::initializer_list<Image> images) {
    setTarget(std::vector<Image>(images));
  }
  SGA_API void setTarget(std::vector<Image> images);

  /** Performs rendering using vertex data from the provided VBO. Before
      rendering pipeline configuration is valdiated, and you will be notified of
      any errors or inconsistencies. Rendering is deferred and this function may
      return before GPU is done. */
  SGA_API void draw(const VBO&);
  SGA_API void drawIndexed(const VBO&, const IBO&);

  SGA_API void clear();

  SGA_API void setProgram(const Program&);

  SGA_API void setSampler(std::string, const Image&,
                  SamplerInterpolation interpolation = SamplerInterpolation::Linear,
                  SamplerWarpMode warp_mode = SamplerWarpMode::Clamp);

  SGA_API void setFaceCull(FaceCullMode fcm = FaceCullMode::None, FaceDirection fd = FaceDirection::Clockwise);

  SGA_API void setPolygonMode(PolygonMode p);
  SGA_API void setRasterizerMode(RasterizerMode r);

  SGA_API void setLineWidth(float w);

  SGA_API void setBlendModeColor(BlendFactor src, BlendFactor dst, BlendOperation op = BlendOperation::Add);
  SGA_API void setBlendModeAlpha(BlendFactor src, BlendFactor dst, BlendOperation op = BlendOperation::Add);

  SGA_API void resetViewport();
  SGA_API void setViewport(float left, float top, float right, float bottom);

  //@{
  /** Sets the value of a named uniform within this pipeline to the provided
      value. The uniform name must correspond to a uniform previously declared
      during shader preparation. These methods will refuse to set a standard
      uniform (`u.sga*`). Using these methods is insanely fast. All changes are
      cached and writes are postponed until next render. */
  #include "pipeline.uniforms.inc"
  SGA_API void setUniform(std::string name, std::initializer_list<float> floats);
  //@}

private:
  enum class UniformProxyMode{
    Uniform,
    Sampler
  };

  template <UniformProxyMode>
  class UniformBank;

  template <UniformProxyMode mode>
  class UniformProxy{
    UniformProxy& operator=(const UniformProxy& other) = delete;
    template <UniformProxyMode m>
    friend class Pipeline::UniformBank;
    UniformProxy(Pipeline& p, std::string n)
      : parent(p), name(n) {};
    Pipeline& parent;
    std::string name;
  public:
    template <typename T, typename std::enable_if<(sizeof(T),mode == UniformProxyMode::Uniform), int>::type = 0>
    void operator=(T value){
      parent.setUniform(name, value);
    }
    template <typename T, typename std::enable_if<(sizeof(T),mode == UniformProxyMode::Sampler), int>::type = 0>
    void operator=(T value){
      parent.setSampler(name, value);
    }
  };

  template <UniformProxyMode mode>
  class UniformBank{
    UniformBank& operator=(const UniformBank& other) = delete;
    friend class Pipeline;
    UniformBank(Pipeline& p) : parent(p){}
    Pipeline& parent;
  public:
    UniformProxy<mode> operator[](std::string name) {return UniformProxy<mode>(parent, name);}
  };

public:
  UniformBank<UniformProxyMode::Uniform> uniform = UniformBank<UniformProxyMode::Uniform>(*this);
  UniformBank<UniformProxyMode::Sampler> sampler = UniformBank<UniformProxyMode::Sampler>(*this);

protected:
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
  SGA_API FullQuadPipeline();
  SGA_API ~FullQuadPipeline();

  SGA_API void drawFullQuad();

  SGA_API void setProgram(const Program&);

  // Forbid some functions from Pipeline which make no sense for FullQuadPipeline
  SGA_API void draw(const VBO&) = delete;
  SGA_API void setFaceCull(FaceCullMode fcm = FaceCullMode::None, FaceDirection fd = FaceDirection::Clockwise) = delete;
  SGA_API void setPolygonMode(PolygonMode p) = delete;
  SGA_API void setRasterizerMode(RasterizerMode r) = delete;
  SGA_API void setLineWidth(float w) = delete;

protected:
  class Impl;
  SGA_API Impl* impl();
  const Impl* impl() const;
};

} // namespace sga

#endif // __SGA_PIPELINE_HPP__
