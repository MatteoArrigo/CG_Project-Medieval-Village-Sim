#include "InteractionsManager.hpp"
#include <json.hpp>

/**
 * Initializes the manager by loading interaction points from the scene file.
 * @param file Path to the file containing interaction points data.
 * @return Status code (0 for success, non-zero for failure), as other init methods in the project.
 */
int InteractionsManager::init(std::string file) {
    nlohmann::json js;
    std::ifstream ifs(file);
    if (!ifs.is_open()) {
        std::cout << "Error! Scene file >" << file << "< not found!";
        exit(-1);
    }
    ifs >> js;
    ifs.close();
    
    std::cout << "Parsing interactable points\n";
    nlohmann::json interactables = js["interactables"];
    for (const auto& interactable : interactables) {
        std::cout << "Read interactable point: " << interactable["id"] << "\n";
        InteractionPoint interaction;
        interaction.id = interactable["id"];
        nlohmann::json pos = interactable["pos"];
        if (pos.is_array() && pos.size() == 3) {
            interaction.position = glm::vec3(pos[0], pos[1], pos[2]);
        } else {
            std::cout << "Invalid position for interactable: " << interaction.id << "\n";
            return -1;
        }
        nlohmann::json instanceIds = interactable["instanceIds"];
        if (instanceIds.is_array())
            for (const auto& id : instanceIds)
                interaction.instaceIds.push_back(id.get<std::string>());

        interactionPoints.push_back(interaction);
    }

    return 0;
}

/**
 * Computes the nearest interactable point based on the player's position. Updates internal state.
 * Only the interactable points within the maximum distance from the player are considered.
 * @param playerPos The current position of the player.
 */
void InteractionsManager::updateNearInteractable(const glm::vec3& playerPos) {
    interactableIdx = -1;
    float minDist = maxDistance;
    for (size_t i = 0; i < interactionPoints.size(); ++i) {
        float distance = glm::length(playerPos - interactionPoints[i].position);
        if (distance <= minDist) {
            minDist = distance;
            interactableIdx = static_cast<int>(i);
        }
    }
}

/**
 * Returns whether there is an interactable point near the player.
 * It checks internal state (no recomputation).
 * @return True if an interactable point is nearby, false otherwise.
 */
bool InteractionsManager::isNearInteractable() const {
    return interactableIdx != -1;
}

/**
 * Retrieves the nearest interactable points.
 * It checks internal state (no recomputation).
 * @return The nearest InteractionPoint.
 */
InteractionPoint InteractionsManager::getNearInteractable() const {
    if (interactableIdx >= 0 && interactableIdx < static_cast<int>(interactionPoints.size())) {
        return interactionPoints[interactableIdx];
    } else {
        std::cout << "No interactable point found.\n";
        return InteractionPoint{};
    }
}

/**
 * Executes the interaction with the nearest interactable point, if any.
 * Updates the provided state accordingly.
 * @param state Interactable state of the main logic, modified by this method
 */
void InteractionsManager::interact(InteractableState& state) const {
    if (!isNearInteractable())
        return;
    InteractionPoint interaction = getNearInteractable();
    // Cases of interactions handled simply through list of ifs, recognized by querying the id of interactable point

    std::string id = interaction.id;
    if (id.find("torch_fire") != std::string::npos) {
        // *** Interaction with TORCHES ***
        int torchIdx = std::stoi(id.substr(id.find_last_of('.') + 1));
        if (torchIdx >= 0 && torchIdx < static_cast<int>(state.torchesOn.size())) {
            state.torchesOn[torchIdx] = !state.torchesOn[torchIdx];
        } else {
            std::cout << "Invalid torch index: " << torchIdx << "\n";
        }
    } else if (id.find("crane_wheel") != std::string::npos) {
        // *** Interaction with CRANE WHEELS ***
        int craneWheelIdx = std::stoi(id.substr(id.find_last_of('.') + 1));
        if (craneWheelIdx >= 0 && craneWheelIdx < static_cast<int>(state.craneWheelsRotating.size())) {
            state.craneWheelsRotating[craneWheelIdx] = !state.craneWheelsRotating[craneWheelIdx];
        } else {
            std::cout << "Invalid crane wheel index: " << craneWheelIdx << "\n";
        }

    } else {
        std::cout << "Interaction ID not recognized\n";
    }
}