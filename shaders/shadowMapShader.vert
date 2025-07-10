#version 450
#extension GL_ARB_separate_shader_objects : enable

/**
 * Shadow Map Vertex Shader
 *
 * This shader transforms each vertex of a 3D model from model space into the light's clip space.
 * The resulting positions are used to render a depth map (shadow map) from the light's perspective.
 * Actually, only gl_Position.z will be written to depth buffer.
 * And if 2 models are projected to the same (x,y) position, only the one with smaller z will be written to depth buffer.
 *
 * Notes:
 *  - We use orthographic projection as light is directional (parallel rays)
 *  - Smaller z value means closer to the light source.
 * If rendered as grayscale image, the produces shaodw map will be black for points closer to the light,
*   and white for points farther away.
 */

// VDTan vertex shader
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec2 fragUV;

/**
* Uniform buffer object for passing transformation matrices.
* This includes the light's view-projection matrix and the model matrix.
*/
layout(set = 0, binding = 0) uniform ShadowMapUBO {
    mat4 lightVP;  // Light's view-projection matrix (orthographic)
    mat4 model;    // Model matrix of the object
} shadowMapUbo;

void main() {
    // Transform vertex position from model space to light clip space
    gl_Position = shadowMapUbo.lightVP * shadowMapUbo.model * vec4(inPosition, 1.0);
    fragUV = inUV;
}
