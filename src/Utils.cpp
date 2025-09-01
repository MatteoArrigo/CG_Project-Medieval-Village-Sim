#include "Utils.hpp"
#include <sstream>

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

/**
 * Normalizes an angle to the range [0, 2π] radians.
 *
 * This function takes any angle value and maps it to the equivalent angle
 * within the standard circle range of [0, 2π]. It handles both positive
 * and negative input angles correctly.
 *
 * @param angle The input angle in radians (can be any value)
 * @return The normalized angle in the range [0, 2π] radians
 *
 * @example
 * normalizeAngle(3π) -> π
 * normalizeAngle(-π/2) -> 3π/2
 * normalizeAngle(π/4) -> π/4
 */
float normalizeAngle(float angle) {
    return fmod(std::fmod(angle, 2.0f * glm::pi<float>()) + 2.0f * glm::pi<float>(), 2.0f * glm::pi<float>());
}

/**
 * Calculates the shortest angular difference between two angles.
 *
 * Given two angles, this function computes the shortest rotational path
 * between them, taking into account the circular nature of angles.
 * The result will always be in the range [-π, π], where:
 * - Positive values indicate clockwise rotation from 'from' to 'to'
 * - Negative values indicate counter-clockwise rotation
 *
 * Both input angles should be normalized to [0, 2π] for optimal performance,
 * but the function will work correctly with any angle values.
 *
 * @param from The starting angle in radians
 * @param to The target angle in radians
 * @return The shortest angular difference in radians, range [-π, π]
 *
 * @example
 * shortestAngularDiff(π/4, 3π/4) -> π/2 (rotate clockwise π/2)
 * shortestAngularDiff(3π/4, π/4) -> -π/2 (rotate counter-clockwise π/2)
 * shortestAngularDiff(π/8, 15π/8) -> π/4 (shortest path, not 15π/8 - π/8 = 7π/4)
 */
float shortestAngularDiff(float from, float to) {
    float diff = to - from;
    if (diff > glm::pi<float>()) {
        diff -= 2.0f * glm::pi<float>();
    } else if (diff < -glm::pi<float>()) {
        diff += 2.0f * glm::pi<float>();
    }
    return diff;
}

std::string wrapText(const std::string &text, size_t maxWidth) {
	std::istringstream iss(text);
	std::string word;
	std::string result;
	std::string line;

	while (iss >> word) {
		if (line.empty()) {
			line = word;
		} else if (line.size() + 1 + word.size() <= maxWidth) {
			line += " " + word;
		} else {
			result += line + "\n";
			line = word;
		}
	}
	if (!line.empty())
		result += line;

	return result;
}

