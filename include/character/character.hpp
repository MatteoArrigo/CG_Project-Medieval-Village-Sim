#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>
#include "modules/Animations.hpp"

struct Instance;

class Character {
public:
    Character(const std::string& name, const glm::vec3& pos, std::shared_ptr<AnimBlender> AB, std::shared_ptr<SkeletalAnimation> SKA, const std::vector<std::string>& stateNames);

    // Stato e posizione
    void setPosition(const glm::vec3& pos);
    glm::vec3 getPosition() const;
    void setState(const std::string& stateName);
    std::string getState() const;
    int getStateIndex() const;
    int getStateIndex(const std::string& stateName) const;
    std::vector<std::string> getStateNames() const;

    // Dialoghi
    void setDialogues(const std::vector<std::string>& dialogues);
    std::string getCurrentDialogue() const;
    void nextDialogue();

    // Interazione
    void interact();

    // Nome
    std::string getName() const;

    // Attributi per animazioni e skeletal animation
    void setAnimBlender(const AnimBlender& ABx) { *AB.get() = ABx; }
    AnimBlender* getAnimBlender() { return AB.get(); }

    void setSkeletalAnimation(const SkeletalAnimation& SKAx) { *SKA.get() = SKAx; }
    SkeletalAnimation* getSkeletalAnimation() { return SKA.get(); }

    void setInstances(const std::vector<Instance*>& inst) { instances = inst; }
    std::vector<Instance*>& getInstances() { return instances; }

    std::string charStateToString(const std::string& stateName) const;
    std::vector<glm::mat4>* getTransformMatrices();

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