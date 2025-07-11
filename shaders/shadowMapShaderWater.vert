#version 450
#extension GL_ARB_separate_shader_objects : enable

// VDsimp Vertex Descriptor
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;

void main() {
    // empty, the water is not to be considered for shadow mapping
}
