#include "ViewControls.hpp"

/**
 * Updates camera and player state for the current frame.
 * Applies movement and rotation input, updates position, orientation,
 * and handles running logic.
 * It computes both the new world matrix and the view-projection matrix.
 */
void ViewControls::updateFrame(float deltaT, glm::vec3 moveInput, glm::vec3 rotInput, bool isRunning) {
    moveSpeed = isRunning ? MOVE_SPEED_RUN : MOVE_SPEED_BASE;
    efCamera = exp(-10.0 * deltaT);		// Exponential smoothing factor for camera damping
	efPlayerYaw = exp(-8.0f * deltaT);	// Exponential smoothing factor for player yaw
    playerPos = physicsMgr.getPlayerPosition();

    // Update camera rotation
    yaw = yaw - ROT_SPEED * deltaT * rotInput.y;
    pitch = pitch - ROT_SPEED * deltaT * rotInput.x;
    pitch = glm::clamp(pitch, minPitch, maxPitch);

    // Update player yaw
    // NOTE: we always keep playerYaw in [0, 2π] range
    float oldPlayerYaw = playerYaw;
    float newPlayerYaw = glm::length(moveInput) > 0.0f ? normalizeAngle(yaw) : oldPlayerYaw;
    // Modify player yaw accordingly if user moves player left, right or backwards
    if (glm::length(moveInput) > 0.0f) {
        float moveAngle = atan2(-moveInput.x, -moveInput.z); // Angle of movement input in the XZ plane
        newPlayerYaw = normalizeAngle(newPlayerYaw + moveAngle);
    }

    // Smooth player yaw transition using shortest path
    float yawDiff = shortestAngularDiff(oldPlayerYaw, newPlayerYaw);
    playerYaw = normalizeAngle(oldPlayerYaw + (1.0f - efPlayerYaw) * yawDiff);

    // Calculate movement direction based on camera orientation
    glm::vec3 ux = glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0,1,0)) * glm::vec4(1,0,0,1);
    glm::vec3 uz = glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0,1,0)) * glm::vec4(0,0,-1,1);
    glm::vec3 rawMoveDir = moveInput.x * ux - moveInput.z * uz;
    if (isFlying)
        rawMoveDir += moveInput.y * glm::vec3(0,1,0);
    if (glm::length(rawMoveDir) > 0.0f)
        rawMoveDir = glm::normalize(rawMoveDir);
    moveDir = moveSpeed * rawMoveDir;

    // Final results computation
    updateViewPrj();
}

/**
 * Computes the camera's view-projection matrix based on
 * current position, orientation, and view mode.
 */
void ViewControls::updateViewPrj() {
	glm::mat4 Prj, View;

    switch(viewMode) {
        case ViewMode::THIRD_PERSON: {
			Prj = glm::perspective(FOVy, ar, prospNearPlane, prospFarPlane);
			Prj[1][1] *= -1;

            glm::vec3 target = playerPos + glm::vec3(0.0f, camHeight, 0.0f);
            glm::mat4 camWorld = glm::translate(glm::mat4(1), playerPos) *
					glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0,1,0));
            cameraPos = camWorld * glm::vec4(0.0f, camHeight - camDist * sin(pitch), camDist * cos(pitch), 1.0);
            dampedCamPos = efCamera * dampedCamPos + (1.0f - efCamera) * cameraPos;
            View = glm::lookAt(dampedCamPos, target, glm::vec3(0,1,0));

			ViewPrj = Prj * View;
			break;
        }

        case ViewMode::FIRST_PERSON: {
			Prj = glm::perspective(FOVy, ar, prospNearPlane, prospFarPlane);
			Prj[1][1] *= -1;

			cameraPos = playerPos + glm::vec3(0,2.1,0);		// More or less elevated to player's head
			View =	glm::rotate(glm::mat4(1.0f), -roll, glm::vec3(0,0,1)) *
					glm::rotate(glm::mat4(1.0f), -pitch, glm::vec3(1,0,0)) *
					glm::rotate(glm::mat4(1.0f), -yaw, glm::vec3(0,1,0)) *
					glm::translate(glm::mat4(1), -cameraPos);

			ViewPrj = Prj * View;
			break;
        }

		case ViewMode::SHADOW_CLIP: {
			ViewPrj = sunLightManager.getLightVP();
			break;
		}

        case ViewMode::ISOMETRIC: {
			Prj = glm::ortho(-orthoSize, orthoSize,
							 -orthoSize / ar,orthoSize / ar,
							 orthoNearPlane, orthoFarPlane );
			Prj[1][1] *= -1;

			View =	glm::rotate(glm::mat4(1), glm::radians(35.26f), glm::vec3(1,0,0)) *
					glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(0,1,0)) *
					glm::translate(glm::mat4(1), -playerPos);
			cameraPos = playerPos;

			ViewPrj = Prj * View;
			break;
        }

		case ViewMode::DIMETRIC: {
			Prj = glm::ortho(-orthoSize, orthoSize,
							 -orthoSize / ar,orthoSize / ar,
							 orthoNearPlane, orthoFarPlane );
			Prj[1][1] *= -1;

			View =	glm::rotate(glm::mat4(1), glm::radians(20.0f), glm::vec3(1,0,0)) *
					  glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(0,1,0)) *
					  glm::translate(glm::mat4(1), -playerPos);
			cameraPos = playerPos;

			ViewPrj = Prj * View;
			break;
		}

		case ViewMode::TRIMETRIC: {
			Prj = glm::ortho(-orthoSize, orthoSize,
							 -orthoSize / ar,orthoSize / ar,
							 orthoNearPlane, orthoFarPlane );
			Prj[1][1] *= -1;

			View =	glm::rotate(glm::mat4(1), glm::radians(30.0f), glm::vec3(1,0,0)) *
					  glm::rotate(glm::mat4(1), glm::radians(-60.0f), glm::vec3(0,1,0)) *
					  glm::translate(glm::mat4(1), -playerPos);
			cameraPos = playerPos;

			ViewPrj = Prj * View;
			break;
		}

		case ViewMode::CABINET: {
			Prj = glm::ortho(-orthoSize, orthoSize,
							 -orthoSize / ar, orthoSize / ar,
							 orthoNearPlane, orthoFarPlane );
			Prj[1][1] *= -1;

			// cabinet shear factors
			float alpha = glm::radians(45.0f); // angle of projection
			float l = 0.5f;                     // depth scale factor
			glm::mat4 shear(1.0f);
			shear[2][0] = -l * cos(alpha); // z affects x
			shear[2][1] = -l * sin(alpha); // z affects y
			cameraPos = playerPos;

			// just translate the scene by -playerPos
			View = shear * glm::translate(glm::mat4(1.0f), -playerPos);

			ViewPrj = Prj * View;
			break;
		}

		case ViewMode::COUNT:
			break;
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

const std::string ViewControls::getViewModeStr() {
	switch(viewMode) {;
		case ViewMode::FIRST_PERSON: return "First Person";
		case ViewMode::THIRD_PERSON: return "Third Person";
		case ViewMode::ISOMETRIC: return "Isometric";
		case ViewMode::DIMETRIC: return "Dimetric";
		case ViewMode::TRIMETRIC: return "Trimetric";
		case ViewMode::CABINET: return "Cabinet";
		case ViewMode::SHADOW_CLIP: return "Shadow Clip";
		case ViewMode::COUNT: return "";
	}
}
