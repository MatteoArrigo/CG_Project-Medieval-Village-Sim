#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;
layout(location = 4) in uvec4 inJointIndex;
layout(location = 5) in vec4 inJointWeight;

layout(location = 0) out vec2 fragUV;

vec2 positions[3] = vec2[](
vec2(-1.0, -1.0),
vec2(3.0, -1.0),
vec2(-1.0, 3.0)
);

vec2 uvs[3] = vec2[](
vec2(0.0, 0.0),
vec2(2.0, 0.0),
vec2(0.0, 2.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragUV = uvs[gl_VertexIndex];
}