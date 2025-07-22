#include "Player.hpp"

#include <iostream>
#include <stdexcept>
#include <GLFW/glfw3.h>

#include "character/character.hpp"
#include "modules/Scene.hpp"

#define ANIM_BLEND_T 0.5f

#define ANIM_WALKING_IDX 0
#define ANIM_IDLE_IDX 1
#define ANIM_JUMP_IDX 2
#define ANIM_RUNNING_IDX 3

#define ANIM_JUMP_TIME 0.8f


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
void Player::handleKeyToggle(GLFWwindow* window, int key, bool& debounce, int& curDebounce, const std::function<void()>& action) {
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

void Player::handleKeyStateChange(GLFWwindow* window, int key, bool& prevState, std::function<void()> onPress, std::function<void()> onRelease) {
    bool currentState = glfwGetKey(window, key) == GLFW_PRESS;
    if (currentState && !prevState) {
        onPress();
    } else if (!currentState && prevState) {
        onRelease();
    }
    prevState = currentState;
}

Player::Player(const std::shared_ptr<Character>& playerCharacter)
    : playerCharacter(playerCharacter) {
    if (!playerCharacter) {
        throw std::runtime_error("Player character cannot be null");
    }
}

Player::~Player() = default;

std::vector<Instance*>& Player::getModelInstances() {
    return playerCharacter->getInstances();
}

std::shared_ptr<Character> Player::getCharacter() {
    return playerCharacter;
}

void Player::move(glm::vec3 position, float rotation) {
    for (Instance* I : getModelInstances()) {
        I->Wm = glm::translate(glm::mat4(1.0f), position) *
                glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0,1,0)) *
                glm::scale(glm::mat4(1.0f), playerScale);
    }
}

void Player::jump() {
    std::cout << "jump()\n";
    playerActionState = PlayerActionState::Jumping;
    playerCharacter->getAnimBlender()->Start(ANIM_JUMP_IDX, ANIM_BLEND_T);
}

void Player::walk() {
    std::cout << "walk()\n";
    playerMovementState = PlayerMovementState::Walking;
    playerCharacter->getAnimBlender()->Start(ANIM_WALKING_IDX, ANIM_BLEND_T);
}

void Player::run() {
    std::cout << "run()\n";
    playerMovementState = PlayerMovementState::Running;
    playerCharacter->getAnimBlender()->Start(ANIM_RUNNING_IDX, ANIM_BLEND_T);
}

void Player::idle() {
    playerMovementState = PlayerMovementState::Idle;
    playerCharacter->getAnimBlender()->Start(ANIM_IDLE_IDX, ANIM_BLEND_T);
}


void Player::handleKeyActions(GLFWwindow * window, double deltaT) {
    time += deltaT;

    // Jump
    handleKeyToggle(window, GLFW_KEY_SPACE, debounce, curDebounce, [&]() {
        lastJumpTime = time;
        jump();
    });

    // Walk
    handleKeyStateChange(window, GLFW_KEY_W, isKeyPressed_W, [&]() {
        isKeyPressed_W = true;
        walk();
    }, [&]() {
        isKeyPressed_W = false;
    });

    handleKeyStateChange(window, GLFW_KEY_A, isKeyPressed_A, [&]() {
        isKeyPressed_A = true;
        walk();
    }, [&]() {
        isKeyPressed_A = false;
    });

    handleKeyStateChange(window, GLFW_KEY_S, isKeyPressed_S, [&]() {
        isKeyPressed_S = true;
        walk();
    }, [&]() {
        isKeyPressed_S = false;
    });

    handleKeyStateChange(window, GLFW_KEY_D, isKeyPressed_D, [&]() {
        isKeyPressed_D = true;
        walk();
    }, [&]() {
        isKeyPressed_D = false;
    });

    handleKeyStateChange(window, GLFW_KEY_LEFT_SHIFT, isKeyPressed_SHIFT, [&]() {
        isKeyPressed_SHIFT = true;
        if (playerMovementState == PlayerMovementState::Walking || playerMovementState == PlayerMovementState::Running) {
            run();
        }
    }, [&]() {
        isKeyPressed_SHIFT = false;
    });


    if (playerActionState == PlayerActionState::Jumping && lastJumpTime + ANIM_JUMP_TIME < time) {
        playerActionState = PlayerActionState::NoAction;
        if (playerMovementState == PlayerMovementState::Walking) {
            walk();
        } else if (playerMovementState == PlayerMovementState::Running) {
            run();
        } else {
            idle();
        }
    }

    if (!(isKeyPressed_W || isKeyPressed_A || isKeyPressed_S || isKeyPressed_D)) {
        playerActionState = PlayerActionState::NoAction;
        idle();
    }

}
