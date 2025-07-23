#ifndef CGPROJECT_SUNLIGHT_HPP
#define CGPROJECT_SUNLIGHT_HPP


/**
 * Parameters used for orthogonal projection of the scene from light pov, in shadow mapping render pass
*/
struct LightClipBorders {
    float left;
    float right;
    float bottom;
    float top;
    float near;
    float far;
};

/**
 * Represents the unique directional light in the scene, simulating sunlight.
 * Stores light color, direction, and rotation matrix for shadow mapping.
 * The direction is computed by applying the rotation matrix to the +z axis.
 * The color is modulated by intensity.
 */
class SunLight {
public:
    /**
     * Constructs a SunLight instance with specified color, rotation angles, and intensity.
     * The rotation angles are applied to +z axis in right order (z, x, y) to compute the light direction.
     * @param color Light color (RGB) modulated by intensity.
     * @param rotX Rotation around the X-axis in degrees.
     * @param rotY Rotation around the Y-axis in degrees.
     * @param rotZ Rotation around the Z-axis in degrees (default is 0).
     * @param intensity Light intensity multiplier (default is 1.0).
     */
    SunLight(const glm::vec3& color, float rotX, float rotY, float rotZ = 0.0f, float intensity = 1.0f)
            : color{glm::vec4(color*intensity, 1.0f)} {

        rotMat = glm::rotate(glm::mat4(1), glm::radians(rotY), glm::vec3(0.0f,1.0f,0.0f)) *
                 glm::rotate(glm::mat4(1), glm::radians(rotX), glm::vec3(1.0f,0.0f,0.0f)) *
                 glm::rotate(glm::mat4(1), glm::radians(rotZ), glm::vec3(0.0f,0.0f,1.0f));

        direction = glm::vec3(rotMat * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    }

    const glm::vec3& getDirection() const { return direction; }
    const glm::mat4& getRotationMatrix() const { return rotMat; }
    const glm::vec4& getColor() const { return color; }

private:
    /**
     * Light color, modulated by intensity --> may exceed 1.0 bounds
     */
    glm::vec4 color;
    /**
     * Matrix defining the light rotation to apply to +z axis to get the light direction.
     * It is used
     *  - applied to +z axis to compute light direction
     *  - applied in its inverse form to compute the light projection matrix for shadow map
     * It is chosen from a vector of directions, of the same size of lightColors
     */
    glm::mat4 rotMat;
    /**
     * Directional of the unique directional light in the scene --> Represents the sun light
     * It points towards the light source
     */
    glm::vec3 direction;
};


/**
 * Stores a collection of SunLight objects, the orthographic projection parameters, and computes
 * the view-projection matrices for rendering the scene from each light's point of view.
 * Handles switching between different lights and provides access to the current light's properties.
 *
 * - Computes Vulkan-corrected orthographic projection matrices for shadow mapping.
 * - Maintains an index to select the active SunLight.
 * - Provides accessors for direction, color, and view-projection matrix of the current light.
 */
class SunLightManager {
public:

    /**
     * Constructs a SunLightManager with specified light clip borders, number of lights, and a vector of SunLight objects.
     * Validates the size of the lights vector against nLights.
     * Computes the orthographic projection matrix and view-projection matrices for each light.
     *
     * @param borders_ The light clip borders for orthographic projection.
     * @param nLights_ The number of SunLight objects.
     * @param lights_ A vector of SunLight objects.
     */
    SunLightManager(const LightClipBorders& borders_, int nLights, std::vector<SunLight>&& lights_)
        : borders(borders_), lights(std::move(lights_)), index(0) {
        if (lights.size() != static_cast<size_t>(nLights)) {
            std::cerr << "Error: sunLights vector size (" << lights.size()
                      << ") does not match nLights (" << nLights << ")." << std::endl;
            std::exit(-1);
        }

        // ------ LIGHT PROJECTINO MAT COMPUTATION ------
        /* Light projection matrix is computed using an orthographic projection
         * A rotation is beforehand applied, to take into account the light direction --> projection from light's pov
         *      To do this, the inverse of lightRotation matrix is applied
         *      (inverse because we need to invert the rotation of the world scene to get the light's view)
         * We need as output NDC coordinates (Normalized Device/Screen Coord) the range [-1,1] for x and y, and [0,1] for z
         * To do this used glm::orth, fixing
            - y: is inverted wrt to vulkan    --> apply scale of factor -1
            - z: in vulkan is [0,1], but ortho (for glm) computes it in [-1,1]    --> apply scale of factor 0.5 and translation of 0.5
        */
        auto vulkanCorrection =
                glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, 0.5f)) *   // translation of axis z
                glm::scale(glm::mat4(1.0), glm::vec3(1.0f, -1.0f, 0.5f));       // scale of axis y and z
        lightProj = vulkanCorrection * glm::ortho(borders.left, borders.right, borders.bottom, borders.top, borders.near, borders.far);

        // Compute the light view-projection matrices for each light
        lightVPs.reserve(nLights);
        for(int i = 0; i < nLights; ++i) {
            lightVPs[i] = lightProj * glm::inverse(lights[i].getRotationMatrix());
        }
    }

    /**
     * Changes the current light to the next one in the vector, in circular fashion.
     */
    void nextLight() {
        index = (index + 1) % lights.size();
    }

    const glm::vec3& getDirection() const { return lights[index].getDirection(); }
    const glm::vec4& getColor() const {
        return lights[index].getColor(); }
    const glm::mat4& getLightVP() const { return lightVPs[index]; }
    const int getIndex() const { return index; }

private:
    /**
     * Index of the current light color in scene wrt the array lightColors
     * NOTE: Also rendered skybox should be changed accordingly, and for this
     *   lightColors.size() textures for skybox instance are expected in the json file
     */
    size_t index;
    /**
     * Borders for the orthographic projection of the scene from light pov, in shadow mapping render pass
     * Used to compute the light projection matrix
     */
    const LightClipBorders borders;
    /**
     * Vector of SunLight objects representing the different light conditions in the scene.
     * The size of this vector must match nLights.
     */
    const std::vector<SunLight> lights;
    /**
     * Light Projection matrix
     * Used to compute the orthographic projection from light pov, in shadow mapping render pass
     * It is computed using the light clip borders and the inverse of the light rotation matrix
     */
    glm::mat4 lightProj;
    /**
     * Light View Projection matrices
     * Actual projection matrix used to render the scene from light pov, in shadow mapping render pass
     */
    std::vector<glm::mat4> lightVPs;
};


#endif //CGPROJECT_SUNLIGHT_HPP
