#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "modules/Scene.hpp"

/**
 * Represents the state of interactable points in the scene.
 * It should be made of simple control points, that the main logic can easily check to change behaviour.
 * This way, the interaction is indirectly handled by changing this point.
 * Currently, it includes
 *    - the state of torches (on/off). When the torch is on, it emits light and emissive/flickering effect is enabled
 *      When the torch is off, the corresponding point light is disabled, and only the basic texture is rendered.
 *    - the state of crane wheels (rotating or not). When rotating, the corresponding animation is played.
 */
struct InteractableState{
    std::vector<bool> torchesOn;
    std::vector<bool> craneWheelsRotating;
};

/**
 * Represents an interactable point in the scene.
 * @param id Unique identifier for the interaction.
 * @param instaceIds List of instance IDs associated with this interaction.
 *  They should be the string ids used in the list of instances (within techniques) in the scene file.
 *  Used if it is necessary to recover the actual instance of the instance object to resolve the interaction.
 * @param position 3D position of the interaction point.
 *  May not be the position of an actual instance, but only the position where the interaction should become available.
 */
struct InteractionPoint {
    std::string id;                      // Id of interaction
    std::vector<std::string> instaceIds; // IDs of instances associated with this interaction
    glm::vec3 position;                  // Position of interaction
};

/**
 * Manages interactable points within a scene.
 * Provides initialization, proximity checks, and access to interaction data.
 */
class InteractionsManager {
public:
    /**
     * Initializes the manager by loading interaction points from the scene file.
     * @param file Path to the file containing interaction point data.
     * @return Status code (0 for success, non-zero for failure), as other init methods in the project.
     */
    int init(std::string file);

    /**
     * Computes the nearest interactable point based on the player's position. Updates internal state.
     * Only the points within the maximum distance from the player are considered.
     * @param playerPos The current position of the player.
     */
    void updateNearInteractable(const glm::vec3& playerPos);

    /**
     * Returns whether there is an interactable point near the player.
     * It checks internal state (no recomputation).
     * @return True if an interactable point is nearby, false otherwise.
     */
    bool isNearInteractable() const;

    /**
     * Retrieves the nearest interactable point.
     * It checks internal state (no recomputation).
     * @return The nearest InteractionObj.
     */
    InteractionPoint getNearInteractable() const;

    /**
     * Returns a reference to all interaction points managed by this class.
     * @return Reference to the vector of InteractionObj.
     */
    inline const std::vector<InteractionPoint>& getAllInteractions() { return interactionPoints; }

    /**
     * Executes the interaction with the nearest interactable point, if any.
     * Updates the provided state accordingly.
     * @param state Interactable state of the main logic, modified by this method
     */
    void interact(InteractableState& state) const;

private:
    /**
     * Maximum distance to consider for interaction.
     */
    constexpr static float maxDistance = 3.2f;

    /**
     * List of all interaction points managed by this class.
     * The list is initialized from the "interactables" section of the scene file.
     */
    std::vector<InteractionPoint> interactionPoints;

    /**
     * Index of the currently interactable point.
     * -1 if no point is currently interactable (withing maxDistance).
     */
    int interactableIdx;
};