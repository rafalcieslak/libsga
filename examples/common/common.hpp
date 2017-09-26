#ifndef __EXAMPLES_COMMON_HPP__
#define __EXAMPLES_COMMON_HPP__


#if defined(WIN32) || defined(__WIN32)
#define EXAMPLE_DATA_DIR "../examples/data/"
#else
#define EXAMPLE_DATA_DIR "examples/data/"
#endif

bool renderdoc_present = false;
#if defined(WIN32) || defined(__WIN32)
inline void renderdoc_tryenable(){
  return; // Not supported.
}
inline void renderdoc_capture_start(){
  return; // Not supported.
}
inline void renderdoc_capture_end(){
  return; // Not supported.
}
#else
#include <dlfcn.h>
#include <iostream>
#include "renderdoc_app.h"
RENDERDOC_API_1_1_1* renderdoc_api;
inline void renderdoc_tryenable(){
  void* renderdoc = dlopen("librenderdoc.so", RTLD_NOLOAD | RTLD_NOW | RTLD_GLOBAL);
  if(renderdoc){
    std::cout << "Renderdoc is present!" << std::endl;
    pRENDERDOC_GetAPI getapi = (pRENDERDOC_GetAPI)dlsym(renderdoc, "RENDERDOC_GetAPI");
    getapi(eRENDERDOC_API_Version_1_1_1, (void**)&renderdoc_api);
  }
}
inline void renderdoc_capture_start(){
  if(renderdoc_api){
    std::cout << "=== Renderdoc capture start." << std::endl;
    renderdoc_api->StartFrameCapture(0,0);
  }
}
inline void renderdoc_capture_end(){
  if(renderdoc_api){
    std::cout << "=== Renderdoc capture end." << std::endl;
    renderdoc_api->TriggerCapture();
    renderdoc_api->EndFrameCapture(0,0);
  }
}
#endif

#endif // __EXAMPLES_COMMON_HPP__
