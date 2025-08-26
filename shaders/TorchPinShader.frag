#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;

layout(location = 4) in vec4 fragPosLightSpace;
layout(location = 5) in vec4 debug;

layout(location = 0) out vec4 outColor;

// Global UBO (set=0)
#define MAX_POINT_LIGHTS 20
layout(set = 0, binding = 0) uniform LightModelUBO {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;

    vec3 pointLightPositions[MAX_POINT_LIGHTS];
    vec4 pointLightColors[MAX_POINT_LIGHTS];
    int nPointLights;
} lightUbo;

layout(set = 1, binding = 2) uniform TimeUBO {
    float time;
} timeUbo;

layout(set = 2, binding = 0) uniform IndexUBO {
    int index;
} indexUbo;

layout(set = 2, binding = 1) uniform sampler2D burningText;

const float glowStrength = 35.0;
void main() {
    vec3 torchColor = lightUbo.pointLightColors[indexUbo.index].rgb;

    // Distortion effect based on time and UV coordinates, to simulate shimmering flames
    vec2 offset = 0.005 * (1-fragUV.y) * vec2(sin(timeUbo.time*0.5 + fragUV.y*0.5), cos(timeUbo.time*1.0 + fragUV.x*1.0));
    offset *= 0.2*length(torchColor);
    vec4 texColor = texture(burningText, fragUV + offset);

    vec3 finalColor = texColor.rgb;

    // Flickering effect based on time and position
    float flicker = 0.8 + 0.4 * sin(timeUbo.time * 1.0 + fragPos.x * 0.3 - fragPos.y * 5.0);
    vec3 emissive = texColor.rgb * texColor.a * flicker * glowStrength;
    emissive *= 0.2*length(torchColor);
    finalColor += emissive;

    outColor = vec4(finalColor, 1.0);

}