#version 450
#extension GL_ARB_separate_shader_objects : enable
/**
This shader implements a Physically-Based Rendering (PBR) model using the Metallic-Roughness workflow. It calculates lighting based on material properties, textures, and global parameters. The shader supports normal mapping, diffuse mapping and specular mapping.

Inputs for Vertex Shader:
fragPos: Fragment position in world space.
fragNorm: Fragment normal vector.
fragUV: Texture coordinates.
fragTan: Tangent vector for normal mapping.

Outputs:
outColor: Final color output of the fragment.

In particular the texture are handled as:
- diffuseMap: Base color texture (RGB) with baked-in shading information and alpha for transparency.Implements alpha test: if Alpha channel of albedo is less than 0.5, the fragment is discarded (invisible pixel).
- normalMap: Normal map texture (RGB) for surface detail. If the normal map is completely black (vec3(0.0)), it falls back to the default normal (surface Z = normal).
- specularMap: specular map texture (RGB).

As for how to use the metallic and roughness factors (both scalars), know that blender uses a Principled BSDF to render this textures and all the textures are exported from blender.
*/

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;
layout(location = 4) flat in int toBeDiscarded;

layout(location = 0) out vec4 outColor;

// Global UBO (set=0)
#define MAX_POINT_LIGHTS 10
layout(set = 0, binding = 0) uniform LightModelUBO {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;

    vec3 pointLightPositions[MAX_POINT_LIGHTS];
    vec4 pointLightColors[MAX_POINT_LIGHTS];
} lightUbo;

// Material factors (set=2)
layout(set = 2, binding = 0) uniform PbrFactorsUBO {
    vec3 diffuseFactor;      // UNUSED (RGB)
    vec3 specularFactor;     // UNUSED (RGB)
    float glossinessFactor;   // scalar metallic factor
    float aoFactor;  // scalar roughness factor
} matFactors;

// Textures (set = 1)
layout(set = 2, binding = 1) uniform sampler2D diffuseMap;
layout(set = 2, binding = 2) uniform sampler2D normalMap;
layout(set = 2, binding = 3) uniform sampler2D specularMap;
layout(set = 2, binding = 4) uniform sampler2D aoMap;       // UNUSED

// Constants
const float PI = 3.14159265359;

// Utility: Convert normal map from tangent space to world space
vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, fragUV).rgb;

    // If normal map is black (0,0,0), fallback to original normal
    if (tangentNormal == vec3(0.0)) return normalize(fragNorm);

    tangentNormal = tangentNormal * 2.0 - 1.0; // Transform from [0,1] to [-1,1]

    vec3 T = normalize(fragTan.xyz);
    vec3 N = normalize(fragNorm);
    vec3 B = normalize(cross(N, T));

    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

// Fresnel approximation using Schlick's approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

// Geometry Function (Smith’s method)
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = geometrySchlickGGX(NdotV, roughness);
    float ggx2 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

void main() {
    if (toBeDiscarded == 1) {
        discard;
    }

    vec4 albedoSample = texture(diffuseMap, fragUV);
    vec3 albedo = albedoSample.rgb;

    // Alpha test
    if (albedoSample.a < 0.5)
        discard;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(lightUbo.eyePos - fragPos);
    vec3 L = normalize(lightUbo.lightDir);
    vec3 H = normalize(V + L);
    vec3 R = lightUbo.lightColor.rgb; // reflect(-L, N);

    // Sample specular map
    vec3 specColor = texture(specularMap, fragUV).rgb;

    // Blend between dielectric F0 and metallic F0 (which is albedo)
    // TODO: refactor PbrFactorsUBO attributes names
    float metallicFactor = matFactors.glossinessFactor; // Use glossiness as metallic factor
    float roughnessFactor = matFactors.aoFactor; // Use aoFactor as roughness
    vec3 F0 = mix(vec3(0.04), albedo, metallicFactor);
    F0 = mix(F0, specColor, 0.5); // optional mix with specular texture

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughnessFactor);
    float G   = geometrySmith(N, V, L, roughnessFactor);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    // Diffuse reflection
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallicFactor; // Scale diffuse by metallic factor

    float NdotL = max(dot(N, L), 0.0);
    vec3 irradiance = lightUbo.lightColor.rgb * NdotL;

    vec3 Lo = (kD * albedo / PI + specular) * irradiance;

    // Final color (no IBL for simplicity)
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;

    // Gamma correction
    // color = pow(color, vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}