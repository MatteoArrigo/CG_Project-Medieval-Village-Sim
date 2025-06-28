#version 450

// Inputs from the vertex shader
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;

// Output to the framebuffer
layout(location = 0) out vec4 outColor;

// Uniforms
layout(binding = 1, set = 1) uniform sampler2D terrainTexture;
// layout(set = 0, binding = 1) uniform TilingData {
//     float tileFactor; // how many times the texture should repeat over the terrain
// };

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;
} gubo;

void main() {
    float tileFactor = 50.0;
    vec3 baseColor = vec3(1,1,1);  // optional tint to the terrain texture
    float ambientStrength = 0.4;

    // Tiled UV
    vec2 tiledUV = fragUV * tileFactor;

    // Sample the texture with tiled UVs
    vec4 texColor = texture(terrainTexture, tiledUV);

    // Simple diffuse lighting
    float diff = max(dot(normalize(fragNorm), -gubo.lightDir), 0.0);
    vec3 diffuse = gubo.lightColor.xyz * diff;

    // Ambient + Diffuse
    vec3 lighting = ambientStrength + diffuse;

    // Final color with optional tint
    vec3 finalColor = texColor.rgb * lighting * baseColor;

    outColor = vec4(finalColor, texColor.a);
}