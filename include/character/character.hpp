#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

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
    bool canInteract(float playerDistance) const;
    void interact();

    // Nome
    std::string getName() const;

    // Attributi per animazioni e skeletal animation
    void setAnimBlender(const AnimBlender& ABx) { *AB.get() = ABx; }
    AnimBlender* getAnimBlender() { return AB.get(); }

    void setSkeletalAnimation(const SkeletalAnimation& SKAx) { *SKA.get() = SKAx; }
    SkeletalAnimation* getSkeletalAnimation() { return SKA.get(); }

    std::string charStateToString(const std::string& stateName) const;
    std::vector<glm::mat4>* getTransformMatrices();

private:
    std::string name;
    glm::vec3 position;
    std::vector<std::string> stateNames;
    int currentStateIdx;
    std::vector<std::string> dialogues = {"Walking dialogue",
                                          "Running dialogue",
                                          "Idle dialogue",
                                          "Pointing dialogue",
                                          "Waving dialogue" };
    size_t currentDialogue;
    std::shared_ptr<AnimBlender> AB;
    std::shared_ptr<SkeletalAnimation> SKA;
};

// Implementazione
Character::Character(const std::string& name, const glm::vec3& pos, std::shared_ptr<AnimBlender> AB, std::shared_ptr<SkeletalAnimation> SKA, const std::vector<std::string>& stateNames)
    : name(name), position(pos), stateNames(stateNames), currentStateIdx(getStateIndex("Idle")), currentDialogue(0), AB(AB), SKA(SKA) {}

void Character::setPosition(const glm::vec3& pos) { position = pos; }
glm::vec3 Character::getPosition() const { return position; }

void Character::setState(const std::string& stateName) {
    auto it = std::find(stateNames.begin(), stateNames.end(), stateName);
    if (it != stateNames.end()) {
        currentStateIdx = static_cast<int>(std::distance(stateNames.begin(), it));
    }
}
std::string Character::getState() const {
    if (currentStateIdx >= 0 && currentStateIdx < (int)stateNames.size())
        return stateNames[currentStateIdx];
    return "Unknown";
}

int Character::getStateIndex(const std::string& stateName) const {
for (size_t i = 0; i < stateNames.size(); ++i) {
        if (stateNames[i] == stateName) {
            return static_cast<int>(i);
        }
    }
    return -1; // Stato non valido
}

int Character::getStateIndex() const {
    return currentStateIdx;
}
std::vector<std::string> Character::getStateNames() const {
    return stateNames;
}

void Character::setDialogues(const std::vector<std::string>& d) { dialogues = d; currentDialogue = 0; }
std::string Character::getCurrentDialogue() const {
    if (dialogues.empty()) return "";
    return dialogues[currentDialogue];
}
void Character::nextDialogue() {
    if (!dialogues.empty() && currentDialogue + 1 < dialogues.size())
        ++currentDialogue;
}

bool Character::canInteract(float playerDistance) const {
    return playerDistance < 2.0f && getState() == "Idle";
}

void Character::interact() {
    if (getState() == "Idle") {
        setState("Waving");
        // Trova l'indice dello stato "Waving" e avvia l'animazione corrispondente
        int wavingIdx = -1;
        for (size_t i = 0; i < stateNames.size(); ++i) {
            if (stateNames[i] == "Waving") { wavingIdx = (int)i; break; }
        }
        if (wavingIdx >= 0)
            AB->Start(wavingIdx, 0.5);
    }
}

std::string Character::getName() const { return name; }

std::string Character::charStateToString(const std::string& stateName) const {
    return stateName;
}

std::vector<glm::mat4>* Character::getTransformMatrices() {
    SKA->Sample(*AB.get());
    return SKA->getTransformMatrices();
}
