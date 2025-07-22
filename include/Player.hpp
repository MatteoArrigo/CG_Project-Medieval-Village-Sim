#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <glm/vec3.hpp>

#include "character/character.hpp"

struct Instance;
class Character;

enum class PlayerMovementState {
    Idle,
    Walking,
    Running
};

enum class PlayerActionState {
    NoAction,
    Jumping
};

class Player {
private:
    // Reference to the player Character instance
    std::shared_ptr<Character> playerCharacter;

    // Scaling factor for the player character model. Hardcoded and should be adjusted based on the imported model.
    glm::vec3 playerScale = glm::vec3(1.33f);;

    // Current player state (idle at start)
    PlayerMovementState playerMovementState = PlayerMovementState::Idle;
    PlayerActionState playerActionState = PlayerActionState::NoAction;

    // Key interaction constants
    bool debounce = false;
    int curDebounce = 0;

    // Key press statuses
    bool isKeyPressed_SPACE = false;
    bool isKeyPressed_W = false;
    bool isKeyPressed_A = false;
    bool isKeyPressed_S = false;
    bool isKeyPressed_D = false;
    bool isKeyPressed_SHIFT = false;

    // Time counter
    double time = 0.0;
    double lastJumpTime = 0.0;

    // Actions for animation blending
    void jump();
    void walk();
    void run();
    void idle();

    void handleKeyToggle(GLFWwindow* window, int key, bool& debounce, int& curDebounce, const std::function<void()>& action);
    void handleKeyStateChange(GLFWwindow* window, int key, bool& prevState, std::function<void()> onPress, std::function<void()> onRelease);

public:
    explicit Player(const std::shared_ptr<Character>& playerCharacter);
    ~Player();

    // Player movement in scene
    void move(glm::vec3 position, float rotation);
    void handleKeyActions(GLFWwindow * window, double deltaT);

    // Utils
    std::shared_ptr<Character> getCharacter();
    std::vector<Instance*>& getModelInstances();

};
