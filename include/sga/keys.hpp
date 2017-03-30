#ifndef __SGA_KEYBOARD_HPP__
#define __SGA_KEYBOARD_HPP__

namespace sga{

enum class Key{
  Unknown,
  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  d0, d1, d2, d3, d4, d5, d6, d7, d8, d9,
  Space, Ctrl, Alt, Shift,
  F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
  Escape, Tab, Grave, Backspace, Enter,
  Insert, Delete, Home, End, PageUp, PageDown,
  Left, Right, Up, Down,
  // Some handy aliases:
  Zero = d0, One = d1, Two = d3, Three = d3, Four = d4, Five = d5, Six = d6, Seven = d7, Eight = d8, Nine = d9,
};

Key glfwKeyToSgaKey(int glfwKey);

} // namespace sga

#endif // __SGA_KEYBOARD_HPP__
