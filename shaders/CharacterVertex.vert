#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MAX_JOINTS 65
layout(set = 1, binding = 0) uniform CharUBO {
	vec4 debug1;
	mat4 mvpMat[MAX_JOINTS];
	mat4 mMat[MAX_JOINTS];
	mat4 nMat[MAX_JOINTS];
} charUbo;

layout(set = 1, binding = 1) uniform ShadowClipUBO {
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
layout(location = 3) in uvec4 inJointIndex;
layout(location = 4) in vec4 inJointWeight;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec2 debug2;

void main() {
	if(charUbo.debug1.x == 1.0f) {
		gl_Position = charUbo.mvpMat[0] * vec4(inPosition, 1.0);
		fragPos = (charUbo.mMat[0] * vec4(inPosition, 1.0)).xyz;
		fragNorm = (charUbo.nMat[0] * vec4(inNorm, 0.0)).xyz;
	} else {
		if(shadowClipUbo.debug.y == 1.0)
			gl_Position = inJointWeight.x * shadowClipUbo.lightVP * charUbo.mMat[inJointIndex.x] * vec4(inPosition, 1.0);
		else
			gl_Position = inJointWeight.x * charUbo.mvpMat[inJointIndex.x] * vec4(inPosition, 1.0);
		fragPos = inJointWeight.x * (charUbo.mMat[inJointIndex.x] * vec4(inPosition, 1.0)).xyz;
		fragNorm = inJointWeight.x * (charUbo.nMat[inJointIndex.x] * vec4(inNorm, 0.0)).xyz;

		if(shadowClipUbo.debug.y == 1.0)
			gl_Position += inJointWeight.y * shadowClipUbo.lightVP * charUbo.mMat[inJointIndex.y] * vec4(inPosition, 1.0);
		else
			gl_Position += inJointWeight.y * charUbo.mvpMat[inJointIndex.y] * vec4(inPosition, 1.0);
		fragPos += inJointWeight.y *
				  (charUbo.mMat[inJointIndex.y] * vec4(inPosition, 1.0)).xyz;
		fragNorm += inJointWeight.y * 
				   (charUbo.nMat[inJointIndex.y] * vec4(inNorm, 0.0)).xyz;
		
		if(shadowClipUbo.debug.y == 1.0)
			gl_Position += inJointWeight.z * shadowClipUbo.lightVP * charUbo.mMat[inJointIndex.z] * vec4(inPosition, 1.0);
		else
			gl_Position += inJointWeight.z * charUbo.mvpMat[inJointIndex.z] * vec4(inPosition, 1.0);
		fragPos += inJointWeight.z * 
				  (charUbo.mMat[inJointIndex.z] * vec4(inPosition, 1.0)).xyz;
		fragNorm += inJointWeight.z * 
				   (charUbo.nMat[inJointIndex.z] * vec4(inNorm, 0.0)).xyz;
		
		if(shadowClipUbo.debug.y == 1.0)
			gl_Position += inJointWeight.w * shadowClipUbo.lightVP * charUbo.mMat[inJointIndex.w] * vec4(inPosition, 1.0);
		else
			gl_Position += inJointWeight.w * charUbo.mvpMat[inJointIndex.w] * vec4(inPosition, 1.0);
		fragPos += inJointWeight.w * 
				  (charUbo.mMat[inJointIndex.w] * vec4(inPosition, 1.0)).xyz;
		fragNorm += inJointWeight.w * 
				   (charUbo.nMat[inJointIndex.w] * vec4(inNorm, 0.0)).xyz;
	}
	fragUV = inUV;
	debug2 = vec2(charUbo.debug1.y,
		 ((int(charUbo.debug1.z) == inJointIndex.x) ? inJointWeight.x : 0.0f) +
		 ((int(charUbo.debug1.z) == inJointIndex.y) ? inJointWeight.y : 0.0f) +
		 ((int(charUbo.debug1.z) == inJointIndex.z) ? inJointWeight.z : 0.0f) +
		 ((int(charUbo.debug1.z) == inJointIndex.w) ? inJointWeight.w : 0.0f)
		);
}