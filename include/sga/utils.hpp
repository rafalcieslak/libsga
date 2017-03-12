#ifndef __SGA_UTILS_HPP__
#define __SGA_UTILS_HPP__

namespace sga{

/** Displays some basic info about libSGA to the standard output. */
void info();

double getTime();

enum class VerbosityLevel{
  Quiet = 0,
  Verbose = 1,
  Debug = 2,
};

// TODO: Bitfield?
enum class ErrorStrategy{
  None, /// SGA will ignore all errors. Do not use this option unless you are
        /// certain what you are doing! This strategy will lead to internal
        /// crashes.
  Message, /// SGA will print out diagnostic information about the encoutered
           /// exception, but will continue execution. This will cause internal
           /// crashes!
  Throw, /// On errors, SGA will throw exceptions.
  MessageThrow, /// SGA will print out a diagnostic mesasge and throw an
                /// appropriate exception. This is the default and recommended
                /// behavior.
  Abort, /// SGA will simply abort current process when problems happen.
  MessageAbort, /// SGA will print out a diagnostic message and abort current
                /// process.
};

/** Prepares SGA. Chooses a device to use and prepares it for rendering.  You
    may choose the verbosity level SGA will use, default is Quiet. If you call
    init(), you must call terminate() before your application closes! */
void init(VerbosityLevel level = VerbosityLevel::Verbose,
          ErrorStrategy stragety = ErrorStrategy::MessageThrow);

/** Deinitializes SGA. You MUST call terminate() before your application exits,
    if you called init() before. */
void terminate();

} // namespace sga

#endif // __SGA_UTILS_HPP__
