#include <sga/window.hpp>

#include <iostream>

#include <vkhlf/vkhlf.h>
#include <GLFW/glfw3.h>

namespace sga{

class Window::Impl{
public:
  Impl(unsigned int width, unsigned int height, std::string title){
    // Do not create OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
  }
private:
  GLFWwindow* window;
};

Window::Window(unsigned int width, unsigned int height, std::string title) :
  impl(std::make_unique<Window::Impl>(width, height, title)) {}
Window::~Window() = default;
  
} // namespace sga
