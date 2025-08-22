#include "InteractionObjManager.hpp"
#include <json.hpp>

/**
 * Initializes the manager by loading interaction objects from the scene file.
 * @param file Path to the file containing interaction object data.
 * @return Status code (0 for success, non-zero for failure), as other init methods in the project.
 */
int InteractionObjManager::init(std::string file) {
    nlohmann::json js;
    std::ifstream ifs(file);
    if (!ifs.is_open()) {
        std::cout << "Error! Scene file >" << file << "< not found!";
        exit(-1);
    }
    ifs >> js;
    ifs.close();
    
    std::cout << "Parsing interactable objects\n";
    nlohmann::json interactables = js["interactables"];
    for (const auto& interactable : interactables) {
        std::cout << "Read interactable object: " << interactable["id"] << "\n";
        InteractionObj obj;
        obj.id = interactable["id"];
        nlohmann::json pos = interactable["pos"];
        if (pos.is_array() && pos.size() == 3) {
            obj.position = glm::vec3(pos[0], pos[1], pos[2]);
        } else {
            std::cout << "Invalid position for interactable: " << obj.id << "\n";
            return -1;
        }
        nlohmann::json instanceIds = interactable["instanceIds"];
        if (instanceIds.is_array())
            for (const auto& id : instanceIds)
                obj.instaceIds.push_back(id.get<std::string>());

        interactionObjects.push_back(obj);
    }

    return 0;
}

/**
 * Computes the nearest interactable object based on the player's position. Updates internal state.
 * Only the objects within the maximum distance from the player are considered.
 * @param playerPos The current position of the player.
 */
void InteractionObjManager::updateNearInteractable(const glm::vec3& playerPos) {
    interactableIdx = -1;
    float minDist = maxDistance;
    for (size_t i = 0; i < interactionObjects.size(); ++i) {
        float distance = glm::length(playerPos - interactionObjects[i].position);
        if (distance <= minDist) {
            minDist = distance;
            interactableIdx = static_cast<int>(i);
        }
    }
}

/**
 * Returns whether there is an interactable object near the player.
 * It checks internal state (no recomputation).
 * @return True if an interactable object is nearby, false otherwise.
 */
bool InteractionObjManager::isNearInteractable() const {
    return interactableIdx != -1;
}

/**
 * Retrieves the nearest interactable object.
 * It checks internal state (no recomputation).
 * @return The nearest InteractionObj.
 */
InteractionObj InteractionObjManager::getNearInteractable() const {
    if (interactableIdx >= 0 && interactableIdx < static_cast<int>(interactionObjects.size())) {
        return interactionObjects[interactableIdx];
    } else {
        std::cout << "No interactable object found.\n";
        return InteractionObj{};
    }
}