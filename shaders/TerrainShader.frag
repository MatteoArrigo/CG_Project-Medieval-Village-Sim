#version 450

/**
 * Fragment shader for terrain rendering with texture blending, normal mapping,
 * ambient occlusion, and physically-based lighting. Supports mask-based blending between
 * main and terrain textures, configurable tiling, and advanced lighting calculations.
 *
 * Inputs:
 *   - fragPos: Fragment world position
 *   - fragNorm: Fragment normal
 *   - fragUV: Fragment UV coordinates
 *   - fragTan: Fragment tangent (xyz) and handedness (w)
 *   - fragPosLightSpace: Coordinate in light clip space, used to sample the shadow map.
 *
*  Texture samplers:
*    - mAlbedoMap, tAlbedoMap: Albedo (base color) maps for main and terrain
*    - mNormalMap, tNormalMap: Normal maps for main and terrain
*    - mSGMap, tSGMap: Specular-gloss maps for main and terrain
*    - maskMap: Alpha mask for blending main and terrain textures
*    - aoMap, aoMapHighPassed: Ambient occlusion maps (base and high-passed)
*  Uniform factors:
*   - maskBlendFactor: Controls the strength of blending between main and terrain textures.
        When alpha channel of maskMap is 0.0, only main terrain texture is used.
        When alpha channel of maskMap is 1.0, The maskBlendFactor is applied to mix the main and tiled textures.
*   - tilingFactor: controls the tiling factor to apply for secondary texture, mixed where alpha channle of maskMap is 1.0.
 * Uniform Global lighting and camera parameters
 *
 * Outputs:
 *   - outColor: Final fragment color (vec4)
 */


layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;

layout(location = 4) in vec4 fragPosLightSpace;
layout(location = 5) in vec4 debug;

layout(location = 0) out vec4 outColor;

layout(binding = 1, set = 1) uniform sampler2D maskMap;

layout(binding = 2, set = 1) uniform sampler2D mAlbedoMap;
layout(binding = 3, set = 1) uniform sampler2D mNormalMap;
layout(binding = 4, set = 1) uniform sampler2D mSGMap;

layout(binding = 5, set = 1) uniform sampler2D tAlbedoMap;
layout(binding = 6, set = 1) uniform sampler2D tNormalMap;
layout(binding = 7, set = 1) uniform sampler2D tSGMap;

layout(binding = 8, set = 1) uniform sampler2D aoMap;
layout(binding = 9, set = 1) uniform sampler2D aoMapHighPassed;

layout(binding = 10, set = 1) uniform sampler2D shadowMap;

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;
} gubo;

layout(set = 2, binding = 0) uniform TerrainFactorsUBO {
    float maskBlendFactor;
    float tilingFactor;
} terrainFactors;

const float PI = 3.14159265359;
const float aoBaseFactor = 0.8;
const float aoHighPassFactor = 0.8;

const vec3  DIFFUSE_FACTOR     = vec3(0.5);   // full albedo color
const vec3  SPECULAR_FACTOR    = vec3(0.05);  // very low specular (non-metal)
const float GLOSSINESS_FACTOR  = 0.25;        // rough surface
const float AO_FACTOR          = 0.4;         // full ambient occlusion

/**
 * Calculates the shadow factor for the current fragment using the shadow map.
 * This function determines whether the fragment is in shadow by comparing its depth
 * from the light's perspective to the closest depth stored in the shadow map.
 *
 * Steps:
 *    1. Converts the fragment's position from light space clip coordinates to normalized device coordinates (NDC).
 *    2. Maps NDC coordinates from [-1, 1] to UV coordinates in [0, 1] for texture sampling.
 *    3. Checks if the projected coordinates are outside the shadow map bounds; if so, returns 1.0 (fully lit).
 *    4. Samples the shadow map at the projected UV to get the closest depth to the light.
 *    5. Compares the current fragment's depth (with a small bias to prevent shadow acne) to the closest depth.
 *       - If the fragment is further than the closest depth, it is in shadow (returns 0.0).
 *       - Otherwise, it is lit (returns 1.0).
 *
 * @param fragPosLightSpace vec4 - The fragment's position in light space (clip coordinates).
 * @return float - Shadow factor: 1.0 if lit, 0.0 if in shadow.
 */
const float shadowBias = 0.001; // Bias to avoid shadow acne
float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Convert from light space clip coordinates to normalized device coordinates (NDC)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Convert NDC [-1,1] to UV coordinates [0,1]
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    // Discard fragments outside shadow map bounds
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;  // fully lit, outside shadow map

    float closestDepth = texture(shadowMap, projCoords.xy).r;   // Read closest depth from shadow map
    float currentDepth = projCoords.z;                          // Current fragment depth from light's POV
    float shadow = (currentDepth - shadowBias > closestDepth) ? 0.0 : 1.0;      // Shadow comparison

    return shadow;
}

// Function to get the mask blend factor based on the mask texture
// The mask texture's alpha channel is used to blend between the main and terrain textures.
// When the alpha channel is 0.0, only the main texture is used.
// When the alpha channel is 1.0, the maskBlendFactor is applied to mix the main and tiled textures.
float getMaskBlend() {
    return (texture(maskMap, fragUV).a) * terrainFactors.maskBlendFactor;
//    return (1.0 - texture(maskMap, fragUV).a) * terrainFactors.maskBlendFactor;
}

vec4 getBlendedTexture(sampler2D mTex, sampler2D tTex) {
    return mix(texture(mTex, fragUV), texture(tTex, fragUV * terrainFactors.tilingFactor), getMaskBlend());
}

vec3 getNormalFromMap(mat3 TBN, sampler2D mNormal, sampler2D tNormal) {
    vec3 mN = texture(mNormal, fragUV).xyz * 2.0 - 1.0;
    vec3 tN = texture(tNormal, fragUV * terrainFactors.tilingFactor).xyz * 2.0 - 1.0;
    vec3 normalTS = normalize(mix(mN, tN, getMaskBlend()));
    return normalize(TBN * normalTS);
}

float getCombinedAO() {
    float ao = texture(aoMap, fragUV).r;
    float high = texture(aoMapHighPassed, fragUV).r;
    return clamp(ao * aoBaseFactor + high * aoHighPassFactor, 0.0, 1.0);
}

mat3 computeTBN() {
    vec3 T = normalize(fragTan.xyz);
    vec3 N = normalize(fragNorm);
    vec3 B = normalize(cross(N, T) * fragTan.w);
    return mat3(T, B, N);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionBlinnPhong(vec3 N, vec3 H, float glossiness) {
    float NdotH = max(dot(N, H), 0.0001);
    return ((glossiness + 2.0) / (2.0 * PI)) * pow(NdotH, glossiness);
}

void main() {
    if(debug.x == 1.0) {
        float shadow = ShadowCalculation(fragPosLightSpace);
        outColor = vec4(shadow, shadow, shadow, 1.0);
    } else {

        mat3 TBN = computeTBN();
        vec3 N = getNormalFromMap(TBN, mNormalMap, tNormalMap);

        vec4 albedoTex = getBlendedTexture(mAlbedoMap, tAlbedoMap);
        vec4 sgTex = getBlendedTexture(mSGMap, tSGMap);

        vec3 albedo = albedoTex.rgb * DIFFUSE_FACTOR;
        vec3 specularColor = sgTex.rgb * SPECULAR_FACTOR;
        float glossiness = sgTex.a * GLOSSINESS_FACTOR;
        float shininess = max(glossiness * 256.0, 1.0);

        vec3 V = normalize(gubo.eyePos - fragPos);
        vec3 L = normalize(gubo.lightDir);
        vec3 H = normalize(V + L);

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        vec3 F = fresnelSchlick(VdotH, specularColor);
        float specularTerm = pow(NdotH, shininess);
        vec3 specular = F * specularTerm;

        float oneMinusSpec = 1.0 - max(max(specularColor.r, specularColor.g), specularColor.b);
        vec3 diffuse = albedo * oneMinusSpec / PI;

        float ao = getCombinedAO();
        vec3 lightColor = gubo.lightColor.rgb;

        float shadow = ShadowCalculation(fragPosLightSpace);

        vec3 ambient = vec3(AO_FACTOR) * albedo * ao;
        vec3 color = (diffuse + specular) * lightColor * NdotL * shadow + ambient;
        outColor = vec4(color, 1.0);
    }
}
