#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0, set = 1) uniform UniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 nMat;
} ubo;

layout(binding = 11, set = 1) uniform ShaodowClipUBO {
	mat4 lightVP;

	/** Debug vector for shadow map rendering.
	 * If debug.x == 1.0, the terrain renders only white if lit and black if in shadow
	 * If debug.y == 1.0, the light's clip space is visualized instead of the basic perspective view
	 */
	vec4 debug;
} shadowClipUbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec4 fragTan;
layout(location = 4) out vec4 fragPosLightSpace;	// Needed to sample shadow map
layout(location = 5) out vec4 debug; // Debug output


void main() {
	if(shadowClipUbo.debug.y == 1.0)
		gl_Position = shadowClipUbo.lightVP * ubo.mMat * vec4(inPosition, 1.0);
	else
		gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
	fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;
	fragNorm = normalize((ubo.nMat * vec4(inNorm, 0.0)).xyz);
	fragUV = inUV;
	fragTan = vec4(normalize(mat3(ubo.mMat) * inTangent.xyz), inTangent.w);

	fragPosLightSpace = shadowClipUbo.lightVP * ubo.mMat * vec4(inPosition, 1.0);
	debug = shadowClipUbo.debug;
}
