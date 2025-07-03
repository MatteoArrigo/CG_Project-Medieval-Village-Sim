#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec2 fragUV1;
layout(location = 1) out vec2 fragUV2;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out vec3 fragNormalWorld;
layout(location = 4) out vec3 fragTangentWorld;
layout(location = 5) out vec3 fragBitangentWorld;

layout(set = 0, binding = 0) uniform timeUBO {
    float time;
} time;

layout(set = 0, binding = 1) uniform UniformBufferObject {
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;
} ubo;

const float amplitude = 0.3; // Height of the wave
const vec3 waveDir = normalize(vec3(3.0, 0.0, -1.0));       // propagation direction in xz plane
const float spatialFrequency = 0.9; // Controls wavelength
const float waveSpeed = 0.9;        // Controls how fast the wave propagates

const float normFreq1 = 1e-2;
const float normFreq2 = 1e-3;

void main() {
    vec3 pos = inPosition;

    float phase = dot(pos.xz, waveDir.xz) * spatialFrequency - time.time * waveSpeed;
    pos.y = sin(phase) * amplitude;

    vec4 worldPos = ubo.mMat * vec4(pos, 1.0);
    fragPosWorld = worldPos.xyz;

    // Normal in world space
    float dHdx = cos(phase) * spatialFrequency * waveDir.x * amplitude;
    float dHdz = cos(phase) * spatialFrequency * waveDir.z * amplitude;
    vec3 dx = vec3(1.0, dHdx, 0.0);
    vec3 dz = vec3(0.0, dHdz, 1.0);
    vec3 N = normalize(cross(dz, dx));
    fragNormalWorld = normalize((ubo.nMat * vec4(N, 0.0)).xyz);

    // Approximate tangent and bitangent for a planar surface in XZ
    vec3 T = normalize((ubo.mMat * vec4(1.0, 0.0, 0.0, 0.0)).xyz); // X direction
    vec3 B = normalize(cross(N, T)); // Y upward, so this makes sense for a horizontal plane

    fragTangentWorld = T;
    fragBitangentWorld = B;

    gl_Position = ubo.mvpMat * vec4(pos, 1.0);
    fragUV1 = pos.xz * 0.1 + vec2(time.time * normFreq1, 0);
    fragUV2 = pos.xz * 0.1 + vec2(0, time.time * normFreq2);

}
