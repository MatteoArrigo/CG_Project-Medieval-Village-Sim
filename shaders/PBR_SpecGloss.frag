#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;
layout(location = 3) in vec4 fragTan;

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

// Material factors (set=2)
layout(binding = 0, set = 2) uniform SpecularGlossinessUBO {
    vec3 diffuseFactor;      // (RGB)
    vec3 specularFactor;     // (RGB)
    float glossinessFactor;  // scalar
} matFactors;

const float PI = 3.14159265359;

mat3 computeTBN(vec3 N, vec3 T, float tangentW) {
    vec3 B = cross(N, T) * tangentW;
    return mat3(normalize(T), normalize(B), normalize(N));
}

vec3 getNormalFromMap(mat3 TBN) {
    vec3 tangentNormal = texture(normalMap, fragUV).xyz * 2.0 - 1.0;
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
    vec3 N = normalize(fragNorm);
    vec3 T = normalize(fragTan.xyz);
    float w = fragTan.w;
    mat3 TBN = computeTBN(N, T, w);
    vec3 Nmap = getNormalFromMap(TBN);

    vec3 V = normalize(gubo.eyePos - fragPos);
    vec3 L = normalize(gubo.lightDir);
    vec3 H = normalize(V + L);
    vec3 radiance = gubo.lightColor.rgb;

    // Texture sampling
    vec4 texDiffuse  = texture(albedoMap, fragUV);
    vec4 texSpecGloss = texture(specGlossMap, fragUV);

    vec3 diffuseColor = texDiffuse.rgb * matFactors.diffuseFactor;
    vec3 specularColor = texSpecGloss.rgb * matFactors.specularFactor;
    float glossiness = texSpecGloss.a * matFactors.glossinessFactor;

    float roughness = 1.0 - glossiness;

    // Specular BRDF
    float NDF = DistributionBlinnPhong(Nmap, H, glossiness * 256.0); // scale gloss to exponent
    float G   = GeometrySmith(Nmap, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), specularColor);

    vec3 specular = (NDF * G * F) /
    max(4.0 * max(dot(Nmap, V), 0.0001f) * max(dot(Nmap, L), 0.0), 0.0001f);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;

    float NdotL = max(dot(Nmap, L), 0.0);
    vec3 Lo = (kD * diffuseColor / PI + specular) * radiance * NdotL;

    // Minimal ambient approximation
    vec3 ambient = vec3(0.02) * diffuseColor;

    vec3 color = ambient + Lo;

    outColor = vec4(color, texDiffuse.a);
}
