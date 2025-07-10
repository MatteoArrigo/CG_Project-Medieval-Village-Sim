#version 450

layout(location = 0) in vec2 fragUV; // Input UV coordinates from the vertex shader

/** Albedo map used for alpha test */
layout(set = 0, binding = 1) uniform sampler2D albedoMap;

void main() {
    if( texture(albedoMap, fragUV).a < 0.5 ) {
        // If the alpha value of the texture is less than 0.5, discard this vertex
        // This is useful for alpha testing to avoid considering shadows for transparent parts of the model
        discard;
    }
}
