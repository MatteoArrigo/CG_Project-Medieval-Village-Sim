#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>
#include "modules/Animations.hpp"

struct Instance;


/**
 * Represents a character in the scene with position, state, dialogues, and animation capabilities.
 */
class Character {
public:
    /** Constructor to initialize a Character with a name, position, animation blender, skeletal animation, and state names.
     * @param name The name of the character.
     * @param pos The initial position of the character in 3D space.
     * @param AB A shared pointer to the AnimBlender for handling animations.
     * @param SKA A shared pointer to the SkeletalAnimation for skeletal animation handling.
     * @param stateNames A vector of strings representing the possible states of the character (e.g., "Idle", "Running").
     */
    Character(const std::string& name, const glm::vec3& pos, std::shared_ptr<AnimBlender> AB, std::shared_ptr<SkeletalAnimation> SKA, const std::vector<std::string>& stateNames);

    // State and Position
    void setPosition(const glm::vec3& pos);
    glm::vec3 getPosition() const;
    void setState(const std::string& stateName);
    std::string getCurrentState() const;
    int getStateIndex() const;
    int getStateIndex(const std::string& stateName) const;
    std::vector<std::string> getStateNames() const;

    // Dialogues
    void setDialogues(const std::vector<std::string>& dialogues);
    std::string getCurrentDialogue() const;
    void nextDialogue();

    // Interaction
    void interact();

    // Name
    std::string getName() const;

    // Attributes for animations and skeletal animation
    void setAnimBlender(const AnimBlender& ABx) { *AB.get() = ABx; }
    AnimBlender* getAnimBlender() { return AB.get(); }

    void setSkeletalAnimation(const SkeletalAnimation& SKAx) { *SKA.get() = SKAx; }
    SkeletalAnimation* getSkeletalAnimation() { return SKA.get(); }

    void setInstances(const std::vector<Instance*>& inst) { instances = inst; }
    std::vector<Instance*>& getInstances() { return instances; }

    std::string charStateToString(const std::string& stateName) const;
    std::vector<glm::mat4>* getTransformMatrices();

    // Sets the character state to "Idle" and resets dialogue index and animation
    void setIdle();

private:
    std::string name;
    glm::vec3 position;
    std::vector<std::string> stateNames;
    int currentStateIdx;
    std::vector<std::string> dialogues;
    size_t currentDialogue;
    std::shared_ptr<AnimBlender> AB;
    std::shared_ptr<SkeletalAnimation> SKA;
    std::vector<Instance*> instances;
};