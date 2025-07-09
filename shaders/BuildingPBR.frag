#version 450
#extension GL_ARB_separate_shader_objects : enable

/**
 This shader implements a Physically-Based Rendering (PBR) model using the Specular-Glossiness workflow.
 It calculates lighting based on material properties, textures, and global parameters.
 The shader supports normal mapping, ambient occlusion, and specular reflections.

 Inputs for Vertex Shader:
 - fragPos: Fragment position in world space.
 - fragNorm: Fragment normal vector.
 - fragUV: Texture coordinates.
 - fragTan: Tangent vector for normal mapping.
 Outputs:
 - outColor: Final color output of the fragment.

In particular the texture are handled as:
    - albedoMap: Base color texture (RGB) with alpha for transparency.
        Implements alpha test: if Alpha channel of albedo is less than 0.5, the fragment is discarded (invisible pixel).
        It is multiplied by the Diffuse Factor.
        It can be white to take only the (fractional) color from Diffuse Factor
    - normalMap: Normal map texture (RGB) for surface detail.
        If the normal map is completely black (vec3(0.0)), it falls back to the default normal (surface Z = normal).
    - specGlossMap: Combined specular (RGB) and glossiness (A) texture.
        It adds no effect if it is completely black, or Specular Factor and Glossiness Factor are both zero.
    - aoMap: Ambient occlusion texture (R) to modulate ambient light.
        It is multiplied by the Ambient Occlusion Factor.
        It adds no effect if it is completely black (vec3(0.0)) or Ambient Occlusion Factor is zero.
*/



layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;

layout(location = 4) in vec4 fragPosLightSpace;
layout(location = 5) in vec4 debug;

layout(location = 0) out vec4 outColor;

// Global UBO (set=0)
layout(binding = 0, set = 0) uniform GlobalUniformBufferObject {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;
} gubo;

// Textures (set=1)
layout(binding = 1, set = 1) uniform sampler2D albedoMap;
layout(binding = 2, set = 1) uniform sampler2D normalMap;
layout(binding = 3, set = 1) uniform sampler2D specGlossMap;
layout(binding = 4, set = 1) uniform sampler2D aoMap;  // ambient occlusion map

layout(binding = 5, set = 1) uniform sampler2D shadowMap;

// Material factors (set=2)
layout(binding = 0, set = 2) uniform SgAoMaterialFactors {
    vec3 diffuseFactor;      // (RGB)
    vec3 specularFactor;     // (RGB)
    float glossinessFactor;  // scalar
    float aoFactor;          // scalar ambient occlusion factor
} matFactors;

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
float ShadowCalculation(vec4 fragPosLightSpace)
{
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

mat3 computeTBN(vec3 N, vec3 T, float tangentW) {
    vec3 B = cross(N, T) * tangentW;
    return mat3(normalize(T), normalize(B), normalize(N));
}

vec3 getNormalFromMap(mat3 TBN) {
    vec3 tangentNormal = texture(normalMap, fragUV).xyz;

    // Check for completely black normal map (vec3(0.0))
    if (length(tangentNormal) < 1e-5) {
        return TBN[2]; // fallback to default normal: surface Z = normal
    }

    tangentNormal = tangentNormal * 2.0 - 1.0;
    return normalize(TBN * tangentNormal);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionBlinnPhong(vec3 N, vec3 H, float glossiness) {
    float NdotH = max(dot(N, H), 0.0001f);
    return ((glossiness + 2.0) / (2.0 * PI)) * pow(NdotH, glossiness);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0001f), roughness) *
    GeometrySchlickGGX(max(dot(N, L), 0.0001f), roughness);
}

void main() {
    if(debug.x == 1.0) {
        float shadow = ShadowCalculation(fragPosLightSpace);
        outColor = vec4(shadow, shadow, shadow, 1.0);
    } else {

        // Texture sampling
        vec4 texDiffuse = texture(albedoMap, fragUV);
        vec4 texSpecGloss = texture(specGlossMap, fragUV);
        float ao = texture(aoMap, fragUV).r;  // any of the 3 values (RGB) is ok

        // Alpha test for MASK mode
        if (texDiffuse.a < 0.5)
        discard;

        vec3 N = normalize(fragNorm);
        vec3 T = normalize(fragTan.xyz);
        float w = fragTan.w;
        mat3 TBN = computeTBN(N, T, w);
        vec3 Nmap = getNormalFromMap(TBN);

        vec3 V = normalize(gubo.eyePos - fragPos);
        vec3 L = normalize(gubo.lightDir);
        vec3 H = normalize(V + L);
        vec3 radiance = gubo.lightColor.rgb;

        // Apply Specular/Glossiness factors to diffuse color (albedo)
        vec3 diffuseColor = texDiffuse.rgb * matFactors.diffuseFactor;
        vec3 specularColor = texSpecGloss.rgb * matFactors.specularFactor;
        float glossiness = texSpecGloss.a * matFactors.glossinessFactor;

        float roughness = 1.0 - glossiness;

        // Specular BRDF
        float NDF = DistributionBlinnPhong(Nmap, H, glossiness * 256.0); // scale gloss to exponent
        float G = GeometrySmith(Nmap, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), specularColor);

        vec3 specular = (NDF * G * F) /
        max(4.0 * max(dot(Nmap, V), 0.0001f) * max(dot(Nmap, L), 0.0), 0.0001f);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        float NdotL = max(dot(Nmap, L), 0.0);
        float shadow = ShadowCalculation(fragPosLightSpace);

        vec3 Lo = (kD * diffuseColor / PI + specular) * radiance * NdotL * shadow;

        // Minimal ambient approximation
        vec3 ambient = matFactors.aoFactor * ao * diffuseColor;

        vec3 color = ambient + Lo;

        outColor = vec4(color, texDiffuse.a);
    }
}
