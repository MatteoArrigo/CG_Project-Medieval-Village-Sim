#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0, set = 1) uniform UniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 nMat;
} ubo;

layout(binding = 11, set = 1) uniform ShadowUBO {
	mat4 lightVP;
	mat4 model;
} shadowUbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec4 fragTan;
layout(location = 4) out vec4 fragPosLightSpace;	// Needed to sample shadow map


// FIXME
// For now, comment a line and the comment the other to choose the light's view
// or the normal perspective view

void main() {
//	gl_Position = shadowUbo.lightVP * shadowUbo.model * vec4(inPosition, 1.0);
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
	fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;
	fragNorm = normalize((ubo.nMat * vec4(inNorm, 0.0)).xyz);
	fragUV = inUV;
	fragTan = vec4(normalize(mat3(ubo.mMat) * inTangent.xyz), inTangent.w);

	fragPosLightSpace = shadowUbo.lightVP * shadowUbo.model * vec4(inPosition, 1.0);
}
