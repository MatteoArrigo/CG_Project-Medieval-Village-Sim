#include "ViewControls.hpp"

/**
 * Updates camera and player state for the current frame.
 * Applies movement and rotation input, updates position, orientation,
 * and handles running logic.
 * It computes both the new world matrix and the view-projection matrix.
 */
void ViewControls::updateFrame(float deltaT, glm::vec3 moveInput, glm::vec3 rotInput, bool isRunning) {
    moveSpeed = isRunning ? MOVE_SPEED_RUN : MOVE_SPEED_BASE;
    ef = exp(-10.0 * deltaT); // Exponential smoothing factor for camera damping
    playerPos = physicsMgr.getPlayerPosition();

    // Update camera rotation
    yaw = yaw - ROT_SPEED * deltaT * rotInput.y;
    pitch = pitch - ROT_SPEED * deltaT * rotInput.x;
    pitch = glm::clamp(pitch, minPitch, maxPitch);

    // Calculate movement direction based on camera orientation
    glm::vec3 ux = glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0,1,0)) * glm::vec4(1,0,0,1);
    glm::vec3 uz = glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0,1,0)) * glm::vec4(0,0,-1,1);
    glm::vec3 rawMoveDir = moveInput.x * ux - moveInput.z * uz;
    if (isFlying)
        rawMoveDir += moveInput.y * glm::vec3(0,1,0);
    if (glm::length(rawMoveDir) > 0.0f)
        rawMoveDir = glm::normalize(rawMoveDir);
    moveDir = moveSpeed * rawMoveDir;

    // Camera height adjustment
    camHeight += moveSpeed * 0.1f * (glfwGetKey(window, GLFW_KEY_Q) ? 1.0f : 0.0f) * deltaT;
    camHeight -= moveSpeed * 0.1f * (glfwGetKey(window, GLFW_KEY_E) ? 1.0f : 0.0f) * deltaT;
    camHeight = glm::clamp(camHeight, 0.5f, 3.0f);

    // Rotational independence from view with damping
    if(glm::length(glm::vec3(moveDir.x, 0.0f, moveDir.z)) > 0.001f) {
        relDir = yaw + atan2(moveDir.x, moveDir.z);
        dampedRelDir = dampedRelDir > relDir + 3.1416f ? dampedRelDir - 6.28f :
                       dampedRelDir < relDir - 3.1416f ? dampedRelDir + 6.28f : dampedRelDir;
    }
    dampedRelDir = ef * dampedRelDir + (1.0f - ef) * relDir;

    // Final results computation
    World = glm::translate(glm::mat4(1), playerPos) * glm::rotate(glm::mat4(1.0f), dampedRelDir, glm::vec3(0,1,0));
    updateViewPrj();
}

/**
 * Computes the camera's view-projection matrix based on
 * current position, orientation, and view mode.
 */
void ViewControls::updateViewPrj() {
    switch(viewMode) {
        case ViewMode::THIRD_PERSON: {
            // Projection matrix for perspective
            glm::mat4 Prj = glm::perspective(FOVy, ar, worldNearPlane, worldFarPlane);
            Prj[1][1] *= -1;

            // Target position based on physics player position
            glm::vec3 target = playerPos + glm::vec3(0.0f, camHeight, 0.0f);
            // Camera position, depending on Yaw parameter
            glm::mat4 camWorld = glm::translate(glm::mat4(1), playerPos) * glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0,1,0));
            cameraPos = camWorld * glm::vec4(0.0f, camHeight + camDist * sin(pitch), camDist * cos(pitch), 1.0);
            // Damping of camera
            dampedCamPos = ef * dampedCamPos + (1.0f - ef) * cameraPos;

            glm::mat4 View = glm::lookAt(dampedCamPos, target, glm::vec3(0,1,0));
            ViewPrj = Prj * View;
            break;
        }

        case ViewMode::FIRST_PERSON: {
            break;
        }

        case ViewMode::ISOMETRIC: {
            break;
        }

		case ViewMode::SHADOW_CLIP: {
			ViewPrj = sunLightManager.getLightVP();
			break;
		}
    }
}

/**
 * Cycles to the next camera view mode.
 */
void ViewControls::nextViewMode() {
    int mode = static_cast<int>(viewMode);
    mode = (mode + 1) % static_cast<int>(ViewMode::COUNT);
    viewMode = static_cast<ViewMode>(mode);
}
