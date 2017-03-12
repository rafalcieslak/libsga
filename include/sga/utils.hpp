#ifndef __SGA_UTILS_HPP__
#define __SGA_UTILS_HPP__

namespace sga{

/** Displays some basic info about libSGA status to the standard output. */
void info();

/** Returns the number of seconds elapsed since SGA was initialized, as a
 * double-precision floating point value. You may find it convenient to use this
 * function for timing and animation in your application. The same value is
 * available for shaders via the `u.sgaTime` uniform. */
double getTime();

enum class VerbosityLevel{
  Quiet = 0, /// SGA will never use standard output.
  Verbose = 1, /// SGA will print out useful status information to the standard output.
  Debug = 2, /// SGA will print out a lot of debug information to the standard output.
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
    may choose the verbosity level SGA will use, default is Verbose. The
    verbosity level may be overridden at runtime using `LIBSGA_DEBUG`
    environental variable, which may be set to `quiet`, `verbose`, or
    `debug`. This is useful for debugging applications that were compiled with
    verbosity level set to quiet.

    If you call init(), you must call terminate() before your application
    closes! */
void init(VerbosityLevel level = VerbosityLevel::Verbose,
          ErrorStrategy stragety = ErrorStrategy::MessageThrow);

/** Deinitializes SGA. You MUST call terminate() before your application exits,
    if you called init() before. */
void terminate();

} // namespace sga

#endif // __SGA_UTILS_HPP__
