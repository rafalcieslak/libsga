#ifndef __SGA_UTILS_HPP__
#define __SGA_UTILS_HPP__

namespace sga{

/** Displays some basic info about libSGA to the standard output. */
void info();

double getTime();

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

/** Prepares SGA. Chooses a device to use and prepares it for rendering.  You
    may choose the verbosity level SGA will use, default is Quiet. If you call
    init(), you must call terminate() before your application closes! */
void init(VerbosityLevel level = VerbosityLevel::Quiet, ErrorStrategy stragety = ErrorStrategy::Fail);

/** Deinitializes SGA. You MUST call terminate() before your application exits,
 * if you called init() before. */
void terminate();

} // namespace sga

#endif // __SGA_UTILS_HPP__
