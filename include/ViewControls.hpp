#pragma once

#include <glm/glm.hpp>
#include "PhysicsManager.hpp"

#define N_VIEW_MODES 3
/**
 * Enum to indicate the current view mode (how 3D world is projected to 2D screen)
 */
enum class ViewMode {
    THIRD_PERSON = 0,
    FIRST_PERSON,
    ISOMETRIC
};

/**
 * Controls the camera view and movement in the 3D world.
 *
 * Handles view mode switching, camera position, orientation, and movement logic.
 * Manages camera projection and world transformation matrices.
 * Integrates with physics for player movement and camera damping.
 */
class ViewControls{
public:
    ViewControls(bool isFlying_, GLFWwindow* window_, const float& ar_, const PhysicsManager& physicsManager_) :
        isFlying(isFlying_), window(window_), ar(ar_), physicsMgr(physicsManager_),
        moveSpeed(MOVE_SPEED_BASE), moveDir(0.0f) {}

	/**
	 * Updates camera and player state for the current frame.
	 * Applies movement and rotation input, updates position, orientation,
	 * and handles running logic.
	 * It computes both the new world matrix and the view-projection matrix.
	 */
	void updateFrame(float deltaT, glm::vec3 moveInput, glm::vec3 rotInput, bool isRunning);
	/**
	 * Cycles to the next camera view mode.
	 */
	void nextViewMode();

    float getYaw() const {return yaw;}
    float getPitch() const {return pitch;}
    const glm::vec3& getMoveDir() const {return moveDir;}
    const glm::mat4& getViewPrj() const {return ViewPrj;}
    const glm::vec3& getCameraPos() const {return cameraPos;}
    const glm::mat4& getWorld() const {return World;}
	//TODO: capisci a che serve World, non sembra essere usata...

private:
	/**
	 * Computes the camera's view-projection matrix based on
	 * current position, orientation, and view mode.
	 */
	void updateViewPrj();

    const bool isFlying;
    GLFWwindow* window;
    const float& ar;
    const PhysicsManager& physicsMgr;
    ViewMode viewMode = ViewMode::THIRD_PERSON;

    // Camera Pitch limits
    const float minPitch = glm::radians(-40.0f);
    const float maxPitch = glm::radians(80.0f);
    // Rotation and motion reference speed
    const float ROT_SPEED = glm::radians(120.0f);
    const float MOVE_SPEED_BASE = PlayerConfig::moveSpeed;
    const float MOVE_SPEED_RUN = PlayerConfig::runSpeed;
    const float JUMP_FORCE = PlayerConfig::jumpForce;

    // Camera FOV-y, Near Plane and Far Plane
    const float FOVy = glm::radians(45.0f);
    const float worldNearPlane = 0.1f;
    const float worldFarPlane = 500.f;

    // Camera rotation controls
    float yaw = glm::radians(0.0f);
    float pitch = glm::radians(0.0f);
    float roll = glm::radians(0.0f);
    float ef; // Exponential smoothing factor for camera damping
    float relDir = glm::radians(0.0f);
    float dampedRelDir = glm::radians(0.0f);
    glm::vec3 dampedCamPos = PlayerConfig::startPosition;

    // Camera target height and distance
    float camHeight = 1.5;
    const float camDist = 4.5f;

    // Player position and movement
    float moveSpeed;
    glm::vec3 moveDir;
    glm::vec3 playerPos;

    // Final results per frame
    glm::mat4 ViewPrj;
    glm::mat4 World;
    glm::vec3 cameraPos;
};