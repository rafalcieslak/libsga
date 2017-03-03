#ifndef __SGA_UTILS_HPP__
#define __SGA_UTILS_HPP__

namespace sga{

/** Displays some basic info about libSGA to the standard output. */
void info();

enum class VerbosityLevel{
  Quiet,
  Verbose,
  Debug
};

enum class ErrorStrategy{
  /// SGA will always throw exceptions when errors happen.
  Fail, 
  /// In many cases (thogh not all) SGA will ignore errors and continue. Crashes
  /// and undefined behavior are pretty much guaranteed if you use this option,
  /// but it will suppress most exceptions. Only use this option if you are
  /// certain what you are doing!
  Ignore 
};

/** Prepares SGA. Chooses a device to use and prepares it for rendering.
    You may choose the verbosity level SGA will use, default is Quiet. */
void init(VerbosityLevel level = VerbosityLevel::Quiet, ErrorStrategy stragety = ErrorStrategy::Fail);

/** Deinitializes SGA. Gracefully closes all handles SGA used. After cleanup,
    you may call init() again. You do NOT have to call this function before
    your aplication closes. */
void cleanup();

} // namespace sga

#endif // __SGA_UTILS_HPP__
