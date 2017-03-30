#include <sga/keys.hpp>

#include <GLFW/glfw3.h>

#include <map>

namespace sga{

Key glfwKeyToSgaKey(int glfwKey){
  static std::map<int, Key> m = {
    {GLFW_KEY_0, Key::d0},
    {GLFW_KEY_1, Key::d1},
    {GLFW_KEY_2, Key::d2},
    {GLFW_KEY_3, Key::d3},
    {GLFW_KEY_4, Key::d4},
    {GLFW_KEY_5, Key::d5},
    {GLFW_KEY_6, Key::d6},
    {GLFW_KEY_7, Key::d7},
    {GLFW_KEY_8, Key::d8},
    {GLFW_KEY_9, Key::d9},
    
    {GLFW_KEY_A, Key::A},
    {GLFW_KEY_B, Key::B},
    {GLFW_KEY_C, Key::C},
    {GLFW_KEY_D, Key::D},
    {GLFW_KEY_E, Key::E},
    {GLFW_KEY_F, Key::F},
    {GLFW_KEY_G, Key::G},
    {GLFW_KEY_H, Key::H},
    {GLFW_KEY_I, Key::I},
    {GLFW_KEY_J, Key::J},
    {GLFW_KEY_K, Key::K},
    {GLFW_KEY_L, Key::L},
    {GLFW_KEY_M, Key::M},
    {GLFW_KEY_N, Key::N},
    {GLFW_KEY_O, Key::O},
    {GLFW_KEY_P, Key::P},
    {GLFW_KEY_Q, Key::Q},
    {GLFW_KEY_R, Key::R},
    {GLFW_KEY_S, Key::S},
    {GLFW_KEY_T, Key::T},
    {GLFW_KEY_U, Key::U},
    {GLFW_KEY_V, Key::V},
    {GLFW_KEY_W, Key::W},
    {GLFW_KEY_X, Key::X},
    {GLFW_KEY_Y, Key::Y},
    {GLFW_KEY_Z, Key::Z},

    {GLFW_KEY_SPACE, Key::Space},
    {GLFW_KEY_LEFT_CONTROL, Key::Ctrl},
    {GLFW_KEY_RIGHT_CONTROL, Key::Ctrl},
    {GLFW_KEY_LEFT_ALT, Key::Alt},
    {GLFW_KEY_RIGHT_ALT, Key::Alt},
    {GLFW_KEY_LEFT_SHIFT, Key::Shift},
    {GLFW_KEY_RIGHT_SHIFT, Key::Shift},

    {GLFW_KEY_F1, Key::F1},
    {GLFW_KEY_F2, Key::F2},
    {GLFW_KEY_F3, Key::F3},
    {GLFW_KEY_F4, Key::F4},
    {GLFW_KEY_F5, Key::F5},
    {GLFW_KEY_F6, Key::F6},
    {GLFW_KEY_F7, Key::F7},
    {GLFW_KEY_F8, Key::F8},
    {GLFW_KEY_F9, Key::F9},
    {GLFW_KEY_F10, Key::F10},
    {GLFW_KEY_F11, Key::F11},
    {GLFW_KEY_F12, Key::F12},

    {GLFW_KEY_ESCAPE, Key::Escape},
    {GLFW_KEY_TAB, Key::Tab},
    {GLFW_KEY_GRAVE_ACCENT, Key::Grave},
    {GLFW_KEY_BACKSPACE, Key::Backspace},
    {GLFW_KEY_ENTER, Key::Enter},
    {GLFW_KEY_KP_ENTER, Key::Enter},
    
    {GLFW_KEY_INSERT, Key::Insert},
    {GLFW_KEY_DELETE, Key::Delete},
    {GLFW_KEY_HOME, Key::Home},
    {GLFW_KEY_END, Key::End},
    {GLFW_KEY_PAGE_UP, Key::PageUp},
    {GLFW_KEY_PAGE_DOWN, Key::PageDown},

    {GLFW_KEY_LEFT, Key::Left},
    {GLFW_KEY_RIGHT, Key::Right},
    {GLFW_KEY_UP, Key::Up},
    {GLFW_KEY_DOWN, Key::Down},
  };

  auto it = m.find(glfwKey);
  if(it == m.end())
    return Key::Unknown;
  return it->second;
}

} // namespace sga
