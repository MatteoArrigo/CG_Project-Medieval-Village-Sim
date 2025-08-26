#include "character/character.hpp"

Character::Character(const std::string& name, const glm::vec3& pos, std::shared_ptr<AnimBlender> AB, std::shared_ptr<SkeletalAnimation> SKA, const std::vector<std::string>& stateNames)
    : name(name), position(pos), stateNames(stateNames), currentStateIdx(getStateIndex("Idle")), currentDialogue(0), AB(AB), SKA(SKA) {
    
    // Inizializza i dialoghi di default
    dialogues = {"Hello there!",
                 "Running dialogue",
                 "Idle dialogue",
                 "Pointing dialogue",
                 "Waving dialogue"};
}

void Character::setPosition(const glm::vec3& pos) {
    position = pos;
}

glm::vec3 Character::getPosition() const {
    return position;
}

void Character::setState(const std::string& stateName) {
    auto it = std::find(stateNames.begin(), stateNames.end(), stateName);
    if (it != stateNames.end()) {
        currentStateIdx = static_cast<int>(std::distance(stateNames.begin(), it));
    }
}

std::string Character::getCurrentState() const {
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

void Character::setDialogues(const std::vector<std::string>& d) {
    dialogues = d;
    currentDialogue = 0;
}

std::string Character::getCurrentDialogue() const {
    if (dialogues.empty()) return "";
    return dialogues[currentDialogue];
}

void Character::nextDialogue() {
    if (!dialogues.empty() && currentDialogue + 1 < dialogues.size())
        ++currentDialogue;
}

void Character::interact() {
    if (getCurrentState() == "Idle") {
        setState(stateNames[1]); // Cambia allo succesivo a quello Idle (assumendo che sia il secondo stato)
        // Trova l'indice dello stato "Salute" e avvia l'animazione corrispondente
        int nextAnimId = getStateIndex(getCurrentState());
        if (nextAnimId >= 0)
            AB->Start(nextAnimId, 0.5);
    }
}

std::string Character::getName() const {
    return name;
}

std::string Character::charStateToString(const std::string& stateName) const {
    return stateName;
}

std::vector<glm::mat4>* Character::getTransformMatrices() {
    SKA->Sample(*AB.get());
    return SKA->getTransformMatrices();
}

void Character::setIdle() {
    setState("Idle");
    AB->Start(getStateIndex("Idle"), 0.5);
    currentDialogue = 0;
    return;
}