#include <sga/pipeline.hpp>

#include <iostream>
#include <cassert>

#include <vkhlf/vkhlf.h>

#include "global.hpp"
#include "utils.hpp"

namespace sga{

class Pipeline::Impl{
public:
  Impl(){
    
  }
  
  void setTarget(std::shared_ptr<Window> tgt){
    cooked = false;
    target_is_window = true;
    target = tgt;
    // TODO: Reset shared ptr to target image
  }
  void drawTestTriangle(){
    cook();

    // Perform draws
  }

  void cook(){
    if(cooked) return;

    // Prepare vkPipeline etc.
    std::cout << "Cooking a pipeline." << std::endl;
    
    cooked = true;
  }
private:
  bool cooked = false;
  bool target_is_window;
  std::shared_ptr<Window> target;
};

Pipeline::Pipeline() : impl(std::make_unique<Pipeline::Impl>()) {}
Pipeline::~Pipeline() = default;


void Pipeline::setTarget(std::shared_ptr<Window> target) {impl->setTarget(target);}
void Pipeline::drawTestTriangle() {impl->drawTestTriangle();}

} // namespace sga
