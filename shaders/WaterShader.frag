#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Global UBO (set=0)
layout(binding = 0, set = 1) uniform GlobalUniformBufferObject {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;
} gubo;

layout(binding = 1, set = 1) uniform sampler2D normalMap;

void main() {
    // Sample normal from normal map in tangent space
    vec3 normal = texture(normalMap, fragUV).rgb;
    normal = normalize(normal * 2.0 - 1.0); // Convert from [0,1] to [-1,1]

    // Simple diffuse lighting
    float diffuse = max(dot(normal, normalize(gubo.lightDir)), 0.0);

    vec3 waterColor = vec3(0.0, 0.4, 0.7);
//    outColor = vec4(waterColor, 1.0);
    outColor = vec4(waterColor * diffuse, 1.0);
}
