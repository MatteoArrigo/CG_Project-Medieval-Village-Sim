#version 450
#extension GL_ARB_separate_shader_objects : enable
#define MAX_ANIMATION_JOINTS 100

layout(binding = 0, set = 1) uniform UniformBufferObject {
    vec4 debug1;
    mat4 mvpMat[MAX_ANIMATION_JOINTS];
    mat4 mMat[MAX_ANIMATION_JOINTS];
    mat4 nMat[MAX_ANIMATION_JOINTS];
    int jointsCount;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in uvec4 inJointIndex;
layout(location = 5) in vec4 inJointWeight;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec4 fragTan;
layout(location = 4) out int toBeDiscarded;

void main() {
    if(
        inJointIndex.x >= ubo.jointsCount ||
        inJointIndex.y >= ubo.jointsCount ||
        inJointIndex.z >= ubo.jointsCount ||
        inJointIndex.w >= ubo.jointsCount
    ) {
        toBeDiscarded = 1;
        return;
    } else {
        toBeDiscarded = 0;

        gl_Position = inJointWeight.x *
        ubo.mvpMat[inJointIndex.x] * vec4(inPosition, 1.0);
        fragPos = inJointWeight.x *
        (ubo.mMat[inJointIndex.x] * vec4(inPosition, 1.0)).xyz;
        fragNorm = inJointWeight.x *
        (ubo.nMat[inJointIndex.x] * vec4(inNorm, 0.0)).xyz;

        gl_Position += inJointWeight.y *
        ubo.mvpMat[inJointIndex.y] * vec4(inPosition, 1.0);
        fragPos += inJointWeight.y *
        (ubo.mMat[inJointIndex.y] * vec4(inPosition, 1.0)).xyz;
        fragNorm += inJointWeight.y *
        (ubo.nMat[inJointIndex.y] * vec4(inNorm, 0.0)).xyz;

        gl_Position += inJointWeight.z *
        ubo.mvpMat[inJointIndex.z] * vec4(inPosition, 1.0);
        fragPos += inJointWeight.z *
        (ubo.mMat[inJointIndex.z] * vec4(inPosition, 1.0)).xyz;
        fragNorm += inJointWeight.z *
        (ubo.nMat[inJointIndex.z] * vec4(inNorm, 0.0)).xyz;

        gl_Position += inJointWeight.w *
        ubo.mvpMat[inJointIndex.w] * vec4(inPosition, 1.0);
        fragPos += inJointWeight.w *
        (ubo.mMat[inJointIndex.w] * vec4(inPosition, 1.0)).xyz;
        fragNorm += inJointWeight.w *
        (ubo.nMat[inJointIndex.w] * vec4(inNorm, 0.0)).xyz;

        fragUV = inUV;

        vec3 tan = vec3(0.0);
        tan += inJointWeight.x * (mat3(ubo.mMat[inJointIndex.x]) * inTangent.xyz);
        tan += inJointWeight.y * (mat3(ubo.mMat[inJointIndex.y]) * inTangent.xyz);
        tan += inJointWeight.z * (mat3(ubo.mMat[inJointIndex.z]) * inTangent.xyz);
        tan += inJointWeight.w * (mat3(ubo.mMat[inJointIndex.w]) * inTangent.xyz);
        tan = normalize(tan);
        fragTan = vec4(tan, inTangent.w);
    }
}