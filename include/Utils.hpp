#pragma once
#include <functional>
#include <GLFW/glfw3.h>

class Utils {
  public:
  static void handleKeyToggle(GLFWwindow* window, int key, bool& debounce, int& curDebounce, const std::function<void()>& action);
  static void handleKeyStateChange(GLFWwindow* window, int key, bool& prevState, std::function<void()> onPress, std::function<void()> onRelease);
};
