#ifndef __GLOBAL_HPP__
#define __GLOBAL_HPP__

#include <sga/utils.hpp>

namespace sga{

class impl_global{
public:
  //TODO: error strategy is very inconsistent, this settning is ignored!
  static ErrorStrategy error_strategy;
  static VerbosityLevel verbosity;
  static bool initialized;
};

} // namespace sga

#endif // __GLOBAL_HPP__
