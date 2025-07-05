#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragPosWorld;
layout(location = 1) out vec3 fragNormalWorld;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;

// Object-local transform
layout(binding = 0, set = 1) uniform UniformBufferObject {
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;
} ubo;

// Wind UBO
layout(binding = 1, set = 1) uniform TimeUBO {
    float time;
} timeUBO;

// Wind parameters
const vec3 windDir = normalize(vec3(1.0, 0.0, 1.0));  // Wind blowing along +X (you can rotate this)
const float frequency = 2.0;                         // Speed of swaying
const float amplitude = 0.1;                         // Sway amount

void main() {
    vec3 pos = inPos;

    // Wind effect: sway based on position and time
    // Sway is stronger at the top of the grass blade (pos.y = height) and null at the base (pos.y = 0)
    // Remember that pos.y is the height of the grass blade in local coordinates
    float sway = sin(pos.y * 5.0 + timeUBO.time * frequency) * amplitude;
    pos += windDir * sway * clamp(pos.y, 0.0, 2.0);

    // World-space calculations
    vec4 worldPos = ubo.mMat * vec4(pos, 1.0);
    fragPosWorld = worldPos.xyz;
    fragNormalWorld = normalize(ubo.nMat * vec4(inNorm, 0.0)).xyz;
    fragUV = inUV;

    // Hardcoded tangent space basis (quad vertical orientation)
    vec3 up = vec3(0.0, 1.0, 0.0); // adjust to model orientation if needed
    vec3 tangent = normalize(cross(up, inNorm));
    vec3 bitangent = normalize(cross(inNorm, tangent));

    fragTangent = mat3(ubo.nMat) * tangent;
    fragBitangent = mat3(ubo.nMat) * bitangent;

    gl_Position = ubo.mvpMat * vec4(pos, 1.0);
}
