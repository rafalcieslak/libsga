#ifndef __SGA_EXCEPTIONS_HPP__
#define __SGA_EXCEPTIONS_HPP__

#include <stdexcept>

namespace sga{

struct SGAException : public std::runtime_error{
  std::string name;
  std::string info;
  std::string desc;
  SGAException(std::string name, std::string information, std::string description = "") :
    std::runtime_error(name),
    name(name),
    info(information),
    desc(description) {}
  
  void raise();
  virtual void raise_this() = 0;
  virtual ~SGAException() {} 
};

struct SystemError : public SGAException{
  SystemError(std::string name, std::string information, std::string description = "")
    : SGAException(name, information, description) {}
  virtual void raise_this() override { throw *this; }
};
struct ProgramConfigError : public SGAException{
  ProgramConfigError(std::string name, std::string information, std::string description = "")
    : SGAException(name, information, description) {}
  virtual void raise_this() override { throw *this; }
};
struct PipelineConfigError : public SGAException{
  PipelineConfigError(std::string name, std::string information, std::string description = "")
    : SGAException(name, information, description) {}
  virtual void raise_this() override { throw *this; }
};
struct DataFormatError : public SGAException{
  DataFormatError(std::string name, std::string information, std::string description = "")
    : SGAException(name, information, description) {}
  virtual void raise_this() override { throw *this; }
};
struct VBOSizeError : public SGAException{
  VBOSizeError(std::string name, std::string information, std::string description = "")
    : SGAException(name, information, description) {}
  virtual void raise_this() override { throw *this; }
};
struct ShaderCompilationError : public SGAException{
  ShaderCompilationError(std::string name, std::string information, std::string description)
    : SGAException(name, information, description) {}
  virtual void raise_this() override { throw *this; }
};
struct ShaderLinkingError : public ShaderCompilationError{
  ShaderLinkingError(std::string information, std::string description)
    : ShaderCompilationError("ShaderLinkingError", information, description) {}
  virtual void raise_this() override { throw *this; }
};
struct ShaderParsingError : public ShaderCompilationError{
  ShaderParsingError(std::string information, std::string description)
    : ShaderCompilationError("ShaderParsingError", information, description) {}
  virtual void raise_this() override { throw *this; }
};

}

#endif 
