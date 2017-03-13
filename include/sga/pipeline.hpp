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

/** This class represents the state and configuration of a rendering
    pipeline. Once it is configured, it may then be used for rendering onto a
    window or image surface.
*/
class Pipeline{
public:
  ~Pipeline();

  /** Configures the pipeline to render onto the provided window. */
  void setTarget(std::shared_ptr<Window> window);
  /** Performs rendering using vertex data from the provided VBO. This method
      blocks until rendering completes. Before rendering pipeline configuration
      is valdiated, and you will be notified of any errors or
      inconsistencies. */
  void drawVBO(std::shared_ptr<VBO>);

  /** Configures the clear color of this pipeline, i.e. the color used for
      clearing the target surface before at the beginning of a render. */
  void setClearColor(float r, float g, float b);

  void setProgram(std::shared_ptr<Program>);
  
  //@{
  /** Sets the value of a named uniform within this pipeline to the provided
      value. The uniform name must correspond to a uniform previously declared
      during shader preparation. These methods will refuse to set a standard
      uniform (`u.sga*`). Using these methods is insanely fast. All changes are
      cached and writes are postponed until next render. */
  void setUniform(std::string name, float value);
  void setUniform(std::string name, int value);
  void setUniform(std::string name, unsigned int value);
  void setUniform(std::string name, std::array<float,2> value);
  void setUniform(std::string name, std::array<float,3> value);
  void setUniform(std::string name, std::array<float,4> value);
  void setUniform(std::string name, double value);

  template <typename T>
  void setUniform(std::string name, T value, DataType dt){
    setUniform(dt, name, (char*)&value, sizeof(value));
  }
  //@}

  /** Creates a new unconfigured Pipeline. */
  static std::shared_ptr<Pipeline> create(){
    return std::shared_ptr<Pipeline>(new Pipeline());
  }
private:
  Pipeline();
  void setUniform(DataType dt, std::string name, char* pData, size_t size);
  
  class Impl;
  pimpl_unique_ptr<Impl> impl;
};

} // namespace sga

#endif // __SGA_PIPELINE_HPP__
