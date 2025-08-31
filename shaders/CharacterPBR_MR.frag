#version 450
#extension GL_ARB_separate_shader_objects : enable
/**
This shader implements a Physically-Based Rendering (PBR) model using the Metallic-Roughness workflow.
It calculates lighting based on material properties, textures, and global parameters.
The shader supports normal mapping, diffuse mapping and specular mapping.

Inputs for Vertex Shader:
fragPos: Fragment position in world space.
fragNorm: Fragment normal vector.
fragUV: Texture coordinates.
fragTan: Tangent vector for normal mapping.

Outputs:
outColor: Final color output of the fragment.

In particular the texture are handled as:
- diffuseMap: Base color texture (RGB) with baked-in shading information and alpha for transparency.
    Implements alpha test: if Alpha channel of albedo is less than 0.5, the fragment is discarded (invisible pixel).
- normalMap: Normal map texture (RGB) for surface detail.
    If the normal map is completely black (vec3(0.0)), it falls back to the default normal (surface Z = normal).
- specularMap: specular map texture (RGB).

The shader uses the Metallic-Roughness workflow, where:
- Metallic Factor: Controls how metallic the surface is (0.0 = non-metallic, 1.0 = metallic).
- Roughness Factor: Controls the surface roughness (0.0 = smooth, 1.0 = rough).

*/

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;
layout(location = 4) flat in int toBeDiscarded;

layout(location = 5) in vec4 fragPosLightSpace;
layout(location = 6) in vec4 debug;

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

// Material factors (set=2)
layout(set = 2, binding = 0) uniform PbrMRFactorsUBO {
    float metallicFactor;   // scalar metallic factor
    float roughnessFactor;  // scalar roughness factor
} matFactors;

// Textures (set = 1)
layout(set = 2, binding = 1) uniform sampler2D diffuseMap;
layout(set = 2, binding = 2) uniform sampler2D normalMap;
layout(set = 2, binding = 3) uniform sampler2D specularMap;

layout(set = 2, binding = 4) uniform sampler2D shadowMap;

// Constants
const float PI = 3.14159265359;

/**
 * Calculates the shadow factor for the current fragment using the shadow map.
 * This function determines whether the fragment is in shadow by comparing its depth
 * from the light's perspective to the nearby closest depths stored in the shadow map.
 * It uses Percentage Closer Filtering (PCF) to smooth the shadow edges by sampling multiple'
 * points around the fragment's projected position in the shadow map.
 *
* Steps (with PCF):
*    1. Converts the fragment's position from light space clip coordinates to normalized device coordinates (NDC).
*    2. Maps NDC coordinates from [-1, 1] to UV coordinates in [0, 1] for texture sampling.
*    3. Checks if the projected coordinates are outside the shadow map bounds; if so, returns 1.0 (fully lit).
*    4. Uses Percentage Closer Filtering (PCF) by sampling multiple points around the projected UV in the shadow map.
*    5. For each sample, compares the current fragment's depth (with a small bias to prevent shadow acne) to the closest depth.
*       - If the fragment is further than the closest depth, it is in shadow for that sample.
*    6. Averages the results of all samples to smooth shadow edges and returns the final shadow factor.
 *
 * @param fragPosLightSpace vec4 - The fragment's position in light space (clip coordinates).
 * @return float - Shadow factor: 1.0 if lit, 0.0 if in shadow.
 */
const float shadowBias = 0.001; // Bias to avoid shadow acne
const int pcfRadius = 1; // Radius for PCF sampling
const int pcfStride = 1; // Radius for PCF sampling
const int pcfNSamples = 9; // number of samples considered in PCF sampling
float ShadowCalculation(vec4 fragPosLightSpace) {
    // Convert from light space clip coordinates to normalized device coordinates (NDC)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Convert NDC [-1,1] to UV coordinates [0,1]
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    // Discard fragments outside shadow map bounds
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
    return 1.0;  // fully lit, outside shadow map

    float currentDepth = projCoords.z;                          // Current fragment depth from light's POV

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));  // Get texel size in UV space

    // Perform PCF sampling, controlled by pcfRadius and pcfStride
    for (int x = -pcfRadius; x <= pcfRadius; x+=pcfStride) {
        for (int y = -pcfRadius; y <= pcfRadius; y+=pcfStride) {
            vec2 offset = vec2(x, y) * texelSize;
            float sampleDepth = texture(shadowMap, projCoords.xy + offset).r;
            shadow += currentDepth > sampleDepth + shadowBias ? 1.0 : 0.0;
            // 1 if is in shadow --> I'm counting how many considered samples are in shadow
        }
    }
    shadow /= pcfNSamples;  // Normalize to [0,1] --> is percentage of samples in shadow
    return 1-shadow;        // Actual factor must be inverted, 1.0 if fully lit, 0.0 if fully in shadow
}

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

    if(debug.x == 1.0) {
        // Show only shadow (black) or light (white)
        float shadow = ShadowCalculation(fragPosLightSpace);
        outColor = vec4(shadow, shadow, shadow, 1.0);
    } else if(debug.x == 2.0) {
        // Show only distance from one light
        // Almost-white is point near to at least one point light, black otherwise
        for (int i = 0; i < lightUbo.nPointLights; ++i) {
            vec3 lightPos = lightUbo.pointLightPositions[i];
            vec3 Lp = lightPos - fragPos;
            float distance = length(Lp);
            if( distance < 4.0) {
                outColor = vec4(0.9, 0.9, 0.9, 1.0); // Close to a point light
                return;
            }
        }
        outColor = vec4(0.0, 0.0, 0.0, 1.0); // Far from all point lights
    } else {
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
        float metallicFactor = matFactors.metallicFactor;
        float roughnessFactor = matFactors.roughnessFactor;
        vec3 F0 = mix(vec3(0.04), albedo, metallicFactor);
        F0 = mix(F0, specColor, 0.5); // optional mix with specular texture

        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughnessFactor);
        float G = geometrySmith(N, V, L, roughnessFactor);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular = numerator / denominator;

        // Diffuse reflection
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallicFactor; // Scale diffuse by metallic factor

        float NdotL = max(dot(N, L), 0.0);
        float shadow = ShadowCalculation(fragPosLightSpace);
        vec3 irradiance = lightUbo.lightColor.rgb * NdotL;

        vec3 Lo = (kD * albedo / PI + specular) * irradiance * shadow;

        // ===== Point lights contribution =====
        for (int i = 0; i < lightUbo.nPointLights; ++i) {
            vec3 L = normalize(lightUbo.pointLightPositions[i] - fragPos);
            vec3 H = normalize(V + L);

            float distance = length(lightUbo.pointLightPositions[i] - fragPos);
            float attenuation = 1.0 / (distance * distance); // simple inverse square

            vec3 NDF = vec3(distributionGGX(N, H, roughnessFactor));
            float G = geometrySmith(N, V, L, roughnessFactor);
            vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
            vec3 specular = numerator / denominator;

            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallicFactor;

            float NdotL = max(dot(N, L), 0.0);
            vec3 irradiance = lightUbo.pointLightColors[i].rgb * NdotL * attenuation;

            Lo += (kD * albedo / PI + specular) * irradiance;
        }

        // Final color (no IBL for simplicity)
        vec3 ambient = vec3(0.15) * albedo;
        vec3 color = ambient + Lo;

        outColor = vec4(color, 1.0);
    }
}