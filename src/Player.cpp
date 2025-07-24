#include "Player.hpp"

#include <iostream>
#include <stdexcept>
#include <GLFW/glfw3.h>

#include "PhysicsManager.hpp"
#include "character/character.hpp"
#include "modules/Scene.hpp"
#include "Utils.hpp"

#define ANIM_BLEND_T 0.5f

#define ANIM_WALKING_IDX 0
#define ANIM_IDLE_IDX 1
#define ANIM_JUMP_IDX 2
#define ANIM_RUNNING_IDX 3

#define ANIM_JUMP_TIME 0.8f


Player::Player(const std::shared_ptr<Character>& playerCharacter, PhysicsManager * physicsManager)
    : playerCharacter(playerCharacter), physicsManager(physicsManager) {
    if (!playerCharacter) {
        throw std::runtime_error("Player character cannot be null");
    }
    if (!physicsManager) {
        throw std::runtime_error("PhysicsManager cannot be null");
    }
}

Player::~Player() = default;

std::vector<Instance*>& Player::getModelInstances() {
    return playerCharacter->getInstances();
}

std::shared_ptr<Character> Player::getCharacter() {
    return playerCharacter;
}

void Player::moveModelInScene(glm::vec3 position, float rotation) {
    for (Instance* I : getModelInstances()) {
        I->Wm = glm::translate(glm::mat4(1.0f), position) *
                glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0,1,0)) *
                glm::scale(glm::mat4(1.0f), playerScale);
    }
}

void Player::movePlayerPhysics(glm::vec3 direction) {
		physicsManager->movePlayer(direction, playerMovementState == PlayerMovementState::Running);
}

void Player::move(glm::vec3 direction, float rotation) {
    glm::vec3 playerPosition = physicsManager->getPlayerPosition();

    moveModelInScene(playerPosition, rotation);

    movePlayerPhysics(direction);
}

void Player::jump() {
//    std::cout << "jump()\n";
    playerActionState = PlayerActionState::Jumping;
    playerCharacter->getAnimBlender()->Start(ANIM_JUMP_IDX, ANIM_BLEND_T);
    physicsManager->jumpPlayer();
}

void Player::walk() {
//    std::cout << "walk()\n";
    playerMovementState = PlayerMovementState::Walking;
    playerCharacter->getAnimBlender()->Start(ANIM_WALKING_IDX, ANIM_BLEND_T);
}

void Player::run() {
//    std::cout << "run()\n";
    playerMovementState = PlayerMovementState::Running;
    playerCharacter->getAnimBlender()->Start(ANIM_RUNNING_IDX, ANIM_BLEND_T);
}

void Player::idle() {
    playerMovementState = PlayerMovementState::Idle;
    playerCharacter->getAnimBlender()->Start(ANIM_IDLE_IDX, ANIM_BLEND_T);
}


void Player::handleKeyActions(GLFWwindow * window, double deltaT) {
    time += deltaT;

    // Jump (modifier)
    handleKeyToggle(window, GLFW_KEY_SPACE, debounce, curDebounce, [&]() {
        lastJumpTime = time;
        jump();
    });

    // Walk
    handleKeyStateChange(window, GLFW_KEY_W, isKeyPressed_W, [&]() {
        isKeyPressed_W = true;
        (playerMovementState == PlayerMovementState::Running) ? run() : walk();
    }, [&]() {
        isKeyPressed_W = false;
    });
    handleKeyStateChange(window, GLFW_KEY_A, isKeyPressed_A, [&]() {
        isKeyPressed_A = true;
        (playerMovementState == PlayerMovementState::Running) ? run() : walk();
    }, [&]() {
        isKeyPressed_A = false;
    });
    handleKeyStateChange(window, GLFW_KEY_S, isKeyPressed_S, [&]() {
        isKeyPressed_S = true;
        (playerMovementState == PlayerMovementState::Running) ? run() : walk();
    }, [&]() {
        isKeyPressed_S = false;
    });
    handleKeyStateChange(window, GLFW_KEY_D, isKeyPressed_D, [&]() {
        isKeyPressed_D = true;
        (playerMovementState == PlayerMovementState::Running) ? run() : walk();
    }, [&]() {
        isKeyPressed_D = false;
    });

    // Run (modifier)
    handleKeyStateChange(window, GLFW_KEY_LEFT_SHIFT, isKeyPressed_SHIFT, [&]() {
        isKeyPressed_SHIFT = true;
        if (playerMovementState == PlayerMovementState::Walking || playerMovementState == PlayerMovementState::Running) {
            run();
        }
    }, [&]() {
        isKeyPressed_SHIFT = false;
        // Get back to walking if SHIFT is released and movement keys are pressed
        if (isKeyPressed_W || isKeyPressed_A || isKeyPressed_S || isKeyPressed_D) walk();
    });

    // Handle conclusion of jump animation with smooth transitioning
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

    // Fallback to idle state if no movement keys are pressed
    if (!(isKeyPressed_W || isKeyPressed_A || isKeyPressed_S || isKeyPressed_D)) {
        playerActionState = PlayerActionState::NoAction;
        idle();
    }

}
