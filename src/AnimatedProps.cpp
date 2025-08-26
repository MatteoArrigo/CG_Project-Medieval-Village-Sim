#include "AnimatedProps.hpp"
#include <sstream>
#include <iomanip>

#define CRANE_WHEELS_COUNT 3
#define CRANE_WHEELS_SPEED 0.5f // degrees per frame

std::ostringstream oss;

AnimatedProps::AnimatedProps(InteractionsManager *im, InteractableState* is, Scene *SC) {
    this->interactionsManager = *im;
    this->interactableState = is;
    this->scene = *SC;

    init();
}


void AnimatedProps::init() {
    // Initialize crane wheel interaction states
    for (int craneWheelIdx = 0; craneWheelIdx < CRANE_WHEELS_COUNT; craneWheelIdx++) {
        interactableState->craneWheelsRotating.push_back(true);
    }
}

void AnimatedProps::update(float deltaTime) {
    for (int wheelId = 0; wheelId < 3; wheelId++) {
        if (interactableState->craneWheelsRotating[wheelId]) {
            // Get the instances related to the wheel
            oss << std::setw(2) << std::setfill('0') << wheelId;
            std::string wheelIdStr = oss.str();
            oss.str(""); // Clear the stream for next use
            auto craneWheelInstanceA = scene.InstanceIds.at("build_crane_01_wheel-00." + wheelIdStr);
            auto craneWheelInstanceB = scene.InstanceIds.at("build_crane_01_wheel-01." + wheelIdStr);
            // Rotate the wheel
            scene.I[craneWheelInstanceA]->Wm = glm::rotate(scene.I[craneWheelInstanceA]->Wm, glm::radians(CRANE_WHEELS_SPEED), glm::vec3(0,0,1));
            scene.I[craneWheelInstanceB]->Wm = glm::rotate(scene.I[craneWheelInstanceB]->Wm, glm::radians(CRANE_WHEELS_SPEED), glm::vec3(0,0,1));
        }
    }
}