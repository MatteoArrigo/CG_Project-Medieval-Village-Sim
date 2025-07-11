#version 450
#extension GL_ARB_separate_shader_objects : enable

/**
 * Fragment shader for rendering grass with normal mapping, alpha masking,
 * Lambertian diffuse, and Blinn-Phong specular lighting.
 *
 * Inputs:
 *   - fragPosWorld: World-space position of the fragment.
 *   - fragNormalWorld: World-space normal vector.
 *   - fragUV: Texture coordinates.
 *   - fragTangent: Tangent vector for normal mapping.
 *   - fragBitangent: Bitangent vector for normal mapping.
 *
 * Uniforms:
 *   - albedoMap: Albedo (diffuse) texture.
 *   - normalMap: Normal map for detail lighting.
 *   - lightUbo: Global uniform buffer object containing light direction, color, and eye position.
 *
 * Outputs:
 *   - outColor: Final fragment color.
 *
 * Features:
 *   - Alpha test for masked transparency.
 *   - Normal mapping using TBN matrix.
 *   - Lambertian diffuse and Blinn-Phong specular lighting.
 */

layout(location = 0) in vec3 fragPosWorld;
layout(location = 1) in vec3 fragNormalWorld;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D albedoMap;
layout(set = 2, binding = 1) uniform sampler2D normalMap;

#define MAX_POINT_LIGHTS 20
layout(set = 0, binding = 0) uniform LightModelUBO {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;

    vec3 pointLightPositions[MAX_POINT_LIGHTS];
    vec4 pointLightColors[MAX_POINT_LIGHTS];
    int nPointLights;
} lightUbo;

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
    vec3 viewDir = normalize(lightUbo.eyePos - fragPosWorld);
    vec3 lightDir = normalize(lightUbo.lightDir);

    // Alpha test for MASK mode
    if (albedo.a < 0.5)
        discard;

    // Basic Lambertian diffuse + simple Blinn-Phong specular
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = albedo.rgb * NdotL;

    // Blinn-Phong specular
    vec3 halfway = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), 32.0); // hardcoded shininess
    vec3 specular = lightUbo.lightColor.rgb * spec * 0.2;

    vec3 finalColor = (diffuse + specular) * lightUbo.lightColor.rgb;

    outColor = vec4(finalColor, 1.0);
}
