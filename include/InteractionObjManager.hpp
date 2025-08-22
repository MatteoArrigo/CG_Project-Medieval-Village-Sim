#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "modules/Scene.hpp"

/**
 * Represents an interactable object in the scene.
 * @param id Unique identifier for the interaction.
 * @param instaceIds List of instance IDs associated with this interaction.
 *  They should be the string ids used in the list of instances (within techniques) in the scene file.
 *  Used if it is necessary to recover the actual instance of the object to resolve the interaction.
 * @param position 3D position of the interaction object.
 *  May not be the position of an actual instance, but only the position where the interaction should become available.
 */
struct InteractionObj {
    std::string id;                      // Id of interaction
    std::vector<std::string> instaceIds; // IDs of instances associated with this interaction
    glm::vec3 position;                  // Position of interaction
};

/**
 * Manages interactable objects within a scene.
 * Provides initialization, proximity checks, and access to interaction data.
 */
class InteractionObjManager {
public:
    /**
     * Initializes the manager by loading interaction objects from the scene file.
     * @param file Path to the file containing interaction object data.
     * @return Status code (0 for success, non-zero for failure), as other init methods in the project.
     */
    int init(std::string file);

    /**
     * Computes the nearest interactable object based on the player's position. Updates internal state.
     * Only the objects within the maximum distance from the player are considered.
     * @param playerPos The current position of the player.
     */
    void updateNearInteractable(const glm::vec3& playerPos);

    /**
     * Returns whether there is an interactable object near the player.
     * It checks internal state (no recomputation).
     * @return True if an interactable object is nearby, false otherwise.
     */
    bool isNearInteractable() const;

    /**
     * Retrieves the nearest interactable object.
     * It checks internal state (no recomputation).
     * @return The nearest InteractionObj.
     */
    InteractionObj getNearInteractable() const;

    /**
     * Returns a reference to all interaction objects managed by this class.
     * @return Reference to the vector of InteractionObj.
     */
    inline const std::vector<InteractionObj>& getAllInteractionObj() { return interactionObjects; }

private:
    /**
     * Maximum distance to consider for interaction.
     */
    constexpr static float maxDistance = 3.2f;  // TODO: parametro da tunare

    /**
     * List of all interaction objects managed by this class.
     * The list is initialized from the "interactables" section of the scene file.
     */
    std::vector<InteractionObj> interactionObjects;

    /**
     * Index of the currently interactable object.
     * -1 if no object is currently interactable (withing maxDistance).
     */
    int interactableIdx;
};