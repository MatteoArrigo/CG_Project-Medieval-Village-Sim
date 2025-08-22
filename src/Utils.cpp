#include "Utils.hpp"

/**
 * Handles key toggle events with debounce logic.
 *
 * @param window       Pointer to the GLFW window.
 * @param key          The GLFW key code to check.
 * @param debounce     Reference to a debounce flag to prevent repeated triggers.
 * @param curDebounce  Reference to the currently debounced key.
 * @param action       Function to execute when the key is toggled.
 *
 * When the specified key is pressed, the action is executed only once until the key is released.
 * This prevents multiple triggers from a single key press.
 */
void handleKeyToggle(GLFWwindow* window, int key, bool& debounce, int& curDebounce, const std::function<void()>& action) {
    if (glfwGetKey(window, key)) {
        if (!debounce) {
            debounce = true;
            curDebounce = key;
            action();  // Execute the custom logic
        }
    } else if (curDebounce == key && debounce) {
        debounce = false;
        curDebounce = 0;
    }
}

/**
 * Handles key press and release with callback functions for both of them.
 *
 * @param window       Pointer to the GLFW window.
 * @param key          The GLFW key code to check.
 * @param prevState    Reference to the previous state of the key (pressed or not).
 * @param onPress      Function to execute when the key is pressed.
 * @param onRelease    Function to execute when the key is released.
 *
 */
void handleKeyStateChange(GLFWwindow* window, int key, bool& prevState, std::function<void()> onPress, std::function<void()> onRelease) {
    bool currentState = glfwGetKey(window, key) == GLFW_PRESS;
    if (currentState && !prevState) {
        onPress();
    } else if (!currentState && prevState) {
        onRelease();
    }
    prevState = currentState;
}
