#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragTexCoord;
layout(location = 1) in flat int texId;
layout(location = 2) in vec4 debug;

layout(location = 0) out vec4 outColor;

#define N_SUNLIGHTS 4
layout(set = 0, binding = 1) uniform sampler2D texs[N_SUNLIGHTS];

void main() {
	if(texId<0 || texId>=N_SUNLIGHTS)
		discard;	// Invalid texture ID

	float yaw = -(atan(fragTexCoord.x, fragTexCoord.z)/6.2831853+0.5);
	float pitch = -(atan(fragTexCoord.y, sqrt(fragTexCoord.x*fragTexCoord.x+fragTexCoord.z*fragTexCoord.z))/3.14159265+0.5);
	outColor = texture(texs[texId], vec2(yaw, pitch));
}