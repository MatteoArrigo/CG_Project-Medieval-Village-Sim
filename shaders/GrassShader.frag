#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPosWorld;
layout(location = 1) in vec3 fragNormalWorld;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

layout(location = 0) out vec4 outColor;

layout(binding = 2, set = 1) uniform sampler2D albedoMap;
layout(binding = 3, set = 1) uniform sampler2D normalMap;

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;
} gubo;

vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, fragUV).rgb * 2.0 - 1.0;

    vec3 T = normalize(fragTangent);
    vec3 B = normalize(fragBitangent);
    vec3 N = normalize(fragNormalWorld);
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
    vec4 albedo = texture(albedoMap, fragUV);
    vec3 normal = getNormalFromMap();
    vec3 viewDir = normalize(gubo.eyePos - fragPosWorld);
    vec3 lightDir = normalize(gubo.lightDir);

    // Alpha test for MASK mode
    if (albedo.a < 0.5)
        discard;

    // Basic Lambertian diffuse + simple Blinn-Phong specular
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = albedo.rgb * NdotL;

    vec3 halfway = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), 32.0); // hardcoded shininess
    vec3 specular = gubo.lightColor.rgb * spec * 0.2;

    vec3 finalColor = (diffuse + specular) * gubo.lightColor.rgb;

    outColor = vec4(finalColor, 1.0);
}
