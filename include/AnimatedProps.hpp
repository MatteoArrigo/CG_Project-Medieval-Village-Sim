#pragma once

#include "InteractionsManager.hpp"

class AnimatedProps {

    Scene scene;
    InteractionsManager interactionsManager;
    InteractableState *interactableState;

    public:
        explicit AnimatedProps(InteractionsManager* im, InteractableState* is, Scene* SC);
        void update(float deltaTime);

    private:
        void init();
};