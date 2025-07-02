#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec2 fragUV;

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

    // Basic sine wave based on position and time
    float frequency = 2.0;
    float amplitude = 0.1;
    pos.y = sin(pos.x * frequency + time.time) * amplitude
    + cos(pos.z * frequency + time.time) * amplitude;

    gl_Position = ubo.mvpMat * vec4(pos, 1.0);
//    fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;
//    fragNorm = (ubo.nMat * vec4(inNorm, 0.0)).xyz;

    fragUV = pos.xz * 0.1 + vec2(time.time * 0.05); // Scroll over time
}
