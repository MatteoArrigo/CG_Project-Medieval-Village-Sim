#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec3 fragTangentWorld;
layout(location = 4) out vec3 fragBitangentWorld;

layout(set = 0, binding = 0) uniform timeUBO {
    float time;
} time;

layout(set = 0, binding = 1) uniform UniformBufferObject {
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;
} ubo;

void main() {
    vec3 pos = inPosition;

    float frequency = 2.0;
    float amplitude = 0.1;
    pos.y = sin(pos.x * frequency + time.time) * amplitude
    + cos(pos.z * frequency + time.time) * amplitude;

    vec4 worldPos = ubo.mMat * vec4(pos, 1.0);
    fragPosWorld = worldPos.xyz;

    // Normal in world space
    vec3 N = normalize((ubo.nMat * vec4(inNormal, 0.0)).xyz);
    fragNormalWorld = N;

    // Approximate tangent and bitangent for a planar surface in XZ
    vec3 T = normalize((ubo.mMat * vec4(1.0, 0.0, 0.0, 0.0)).xyz); // X direction
    vec3 B = normalize(cross(N, T)); // Y upward, so this makes sense for a horizontal plane

    fragTangentWorld = T;
    fragBitangentWorld = B;

    gl_Position = ubo.mvpMat * vec4(pos, 1.0);
    fragUV = pos.xz * 0.1 + vec2(time.time * 0.05);
}
