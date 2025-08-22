#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragTexCoord;
layout(location = 1) out int texId;
layout(location = 2) out vec4 debug;

layout(set = 0, binding = 0) uniform GeomSkyboxUBO {
	mat4 mvpMat;
    int texId;
    vec4 debug;
} geomUbo;

void main()
{
    fragTexCoord = inPosition;
    vec4 pos = geomUbo.mvpMat * vec4(inPosition, 1.0);
    gl_Position = pos.xyww;
    texId = geomUbo.texId;
    debug = geomUbo.debug;
}  