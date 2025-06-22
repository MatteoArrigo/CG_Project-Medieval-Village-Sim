#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0, set = 1) uniform UniformBufferObject {
	vec4 debug1;
	mat4 mvpMat[65];
	mat4 mMat[65];
	mat4 nMat[65];
} ubo;


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
	if(ubo.debug1.x == 1.0f) {
		gl_Position = ubo.mvpMat[0] * vec4(inPosition, 1.0);
		fragPos = (ubo.mMat[0] * vec4(inPosition, 1.0)).xyz;
		fragNorm = (ubo.nMat[0] * vec4(inNorm, 0.0)).xyz;
	} else {
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
	}
	fragUV = inUV;
	debug2 = vec2(ubo.debug1.y, 
		 ((int(ubo.debug1.z) == inJointIndex.x) ? inJointWeight.x : 0.0f) +
		 ((int(ubo.debug1.z) == inJointIndex.y) ? inJointWeight.y : 0.0f) +
		 ((int(ubo.debug1.z) == inJointIndex.z) ? inJointWeight.z : 0.0f) +
		 ((int(ubo.debug1.z) == inJointIndex.w) ? inJointWeight.w : 0.0f) 
		);
}