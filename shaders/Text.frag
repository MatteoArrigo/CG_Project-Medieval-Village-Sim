#version 450#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConsts {	vec4 FGcolor;
	vec4 BGcolor;
	vec4 SHcolor;} pushConsts;

void main() {
	vec4 Tx = texture(texSampler, fragTexCoord);
	outColor = Tx.r * pushConsts.FGcolor +			   Tx.g * pushConsts.BGcolor +			   Tx.b * pushConsts.SHcolor;
}