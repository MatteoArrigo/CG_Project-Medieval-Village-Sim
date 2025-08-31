#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_JOINTS 100
layout(set = 1, binding = 0) uniform CharUBO {
	mat4 mvpMat[MAX_JOINTS];
	mat4 mMat[MAX_JOINTS];
	mat4 nMat[MAX_JOINTS];
	int jointsCount;
} charUbo;

layout(set = 1, binding = 1) uniform ShadowClipUBO {
	mat4 lightVP;

/** Debug vector for shadow map rendering.
	 * If debug.x == 1.0, the character renders the greyscale version of the shadow coefficient used for shadow computations
	 * If debug.x == 2.0, the character renders only (almost-)white if lit by at least one torch, black otherwise
	 * If debug.y == 1.0, the light's clip space is visualized instead of the basic perspective view
	 */
	vec4 debug;
} shadowClipUbo;


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

layout(location = 5) out vec4 fragPosLightSpace;
layout(location = 6) out vec4 debug;

void main() {
	if(	inJointIndex.x >= charUbo.jointsCount ||
		inJointIndex.y >= charUbo.jointsCount ||
		inJointIndex.z >= charUbo.jointsCount ||
		inJointIndex.w >= charUbo.jointsCount
	) {
		toBeDiscarded = 1;
		return;
	} else
		toBeDiscarded = 0;

	gl_Position = inJointWeight.x * charUbo.mvpMat[inJointIndex.x] * vec4(inPosition, 1.0);
	fragPos = inJointWeight.x * (charUbo.mMat[inJointIndex.x] * vec4(inPosition, 1.0)).xyz;
	fragNorm = inJointWeight.x * (charUbo.nMat[inJointIndex.x] * vec4(inNorm, 0.0)).xyz;

	gl_Position += inJointWeight.y * charUbo.mvpMat[inJointIndex.y] * vec4(inPosition, 1.0);
	fragPos += inJointWeight.y * (charUbo.mMat[inJointIndex.y] * vec4(inPosition, 1.0)).xyz;
	fragNorm += inJointWeight.y * (charUbo.nMat[inJointIndex.y] * vec4(inNorm, 0.0)).xyz;

	gl_Position += inJointWeight.z * charUbo.mvpMat[inJointIndex.z] * vec4(inPosition, 1.0);
	fragPos += inJointWeight.z * (charUbo.mMat[inJointIndex.z] * vec4(inPosition, 1.0)).xyz;
	fragNorm += inJointWeight.z * (charUbo.nMat[inJointIndex.z] * vec4(inNorm, 0.0)).xyz;

	gl_Position += inJointWeight.w * charUbo.mvpMat[inJointIndex.w] * vec4(inPosition, 1.0);
	fragPos += inJointWeight.w * (charUbo.mMat[inJointIndex.w] * vec4(inPosition, 1.0)).xyz;
	fragNorm += inJointWeight.w * (charUbo.nMat[inJointIndex.w] * vec4(inNorm, 0.0)).xyz;

	fragPosLightSpace =
		inJointWeight.x * shadowClipUbo.lightVP * charUbo.mMat[inJointIndex.x] * vec4(inPosition, 1.0) +
		inJointWeight.y * shadowClipUbo.lightVP * charUbo.mMat[inJointIndex.y] * vec4(inPosition, 1.0) +
		inJointWeight.z * shadowClipUbo.lightVP * charUbo.mMat[inJointIndex.z] * vec4(inPosition, 1.0) +
		inJointWeight.w * shadowClipUbo.lightVP * charUbo.mMat[inJointIndex.w] * vec4(inPosition, 1.0);
    debug = shadowClipUbo.debug;

	fragUV = inUV;

	vec3 tanTmp = vec3(0.0);
	tanTmp += inJointWeight.x * (mat3(charUbo.mMat[inJointIndex.x]) * inTangent.xyz);
	tanTmp += inJointWeight.y * (mat3(charUbo.mMat[inJointIndex.y]) * inTangent.xyz);
	tanTmp += inJointWeight.z * (mat3(charUbo.mMat[inJointIndex.z]) * inTangent.xyz);
	tanTmp += inJointWeight.w * (mat3(charUbo.mMat[inJointIndex.w]) * inTangent.xyz);
	tanTmp = normalize(tanTmp);
	fragTan = vec4(tanTmp, inTangent.w);
}