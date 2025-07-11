#version 450
#extension GL_ARB_separate_shader_objects : enable

/**
 * Shadow Map Vertex Shader, specific for Character.
 *
 * It has same structure and purpose of shadowMapShader.vert for VDtan vertex descriptor. See it for greater details
 * Besides the input variables, the greater difference is the uniform
 * For characters with multiple joints, each of its joint has its own model matrix
 * Hence the model is now an array of model matrices, and the computation of gl_Position
 * takes into account the weights of each coordinate (see CharacterVertex.vert)
 */

// VDchar Vertex Descriptor
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTan;
layout(location = 4) in uvec4 inJointIndex;
layout(location = 5) in vec4 inJointWeight;

layout(location = 0) out vec2 fragUV;

#define MAX_JOINTS 100
layout(set = 0, binding = 0) uniform ShdaowMapUBOChar {
    mat4 lightVP;
    mat4 model[MAX_JOINTS];
} ubo;

void main() {
    // Transform vertex position from model space to light clip space
    gl_Position = inJointWeight.x *
                  ubo.lightVP * ubo.model[inJointIndex.x] * vec4(inPosition, 1.0);
    gl_Position += inJointWeight.y *
                  ubo.lightVP * ubo.model[inJointIndex.y] * vec4(inPosition, 1.0);
    gl_Position += inJointWeight.z *
                    ubo.lightVP * ubo.model[inJointIndex.z] * vec4(inPosition, 1.0);
}
