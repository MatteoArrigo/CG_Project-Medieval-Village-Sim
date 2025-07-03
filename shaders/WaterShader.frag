#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec3 fragTangentWorld;
layout(location = 4) in vec3 fragBitangentWorld;

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 1) uniform GlobalUniformBufferObject {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;
} gubo;

layout(binding = 1, set = 1) uniform sampler2D normalMap;
layout(binding = 2, set = 1) uniform sampler2D facePosX;
layout(binding = 3, set = 1) uniform sampler2D faceNegX;
layout(binding = 4, set = 1) uniform sampler2D facePosY;
layout(binding = 5, set = 1) uniform sampler2D faceNegY;
layout(binding = 6, set = 1) uniform sampler2D facePosZ;
layout(binding = 7, set = 1) uniform sampler2D faceNegZ;

// Function to sample a cube map from 6 faces based on direction vector
// The direction vector should be normalized and in world space
// The function returns the color sampled from the appropriate face
// The cube map faces are expected to be bound to the corresponding texture units
// facePosX, faceNegX, facePosY, faceNegY, facePosZ, faceNegZ
vec4 sampleCubeFrom6Faces(vec3 dir) {
    vec3 absDir = abs(dir);
    float ma;
    vec2 uv;

    if (absDir.x >= absDir.y && absDir.x >= absDir.z) {
        ma = absDir.x;
        if (dir.x > 0.0) {
            // +X
            uv = vec2(dir.z, -dir.y) / ma;
            return texture(facePosX, uv * 0.5 + 0.5);
        } else {
            // -X
            uv = vec2(-dir.z, -dir.y) / ma;
            return texture(faceNegX, uv * 0.5 + 0.5);
        }
    } else if (absDir.y >= absDir.x && absDir.y >= absDir.z) {
        ma = absDir.y;
        if (dir.y > 0.0) {
            // +Y
            uv = vec2(dir.x, -dir.z) / ma;
            return texture(facePosY, uv * 0.5 + 0.5);
        } else {
            // -Y
            uv = vec2(dir.x, dir.z) / ma;
            return texture(faceNegY, uv * 0.5 + 0.5);
        }
    } else {
        ma = absDir.z;
        if (dir.z > 0.0) {
            // +Z
            uv = vec2(-dir.x, -dir.y) / ma;
            return texture(facePosZ, uv * 0.5 + 0.5);
        } else {
            // -Z
            uv = vec2(dir.x, -dir.y) / ma;
            return texture(faceNegZ, uv * 0.5 + 0.5);
        }
    }
}

// Fresnel-Schlick approximation for reflectance
// F0 is the reflectance at normal incidence, typically around 0.04 for dielectrics
// cosTheta is the cosine of the angle between the view direction and the normal
// Returns the reflectance value
float fresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    // Sample the normal map and remap from [0,1] to [-1,1]
    vec3 normalSample = texture(normalMap, fragUV).rgb;
    vec3 normalTangent = normalize(normalSample * 2.0 - 1.0);

    // Construct TBN matrix and transform to world space
    mat3 TBN = mat3(fragTangentWorld, fragBitangentWorld, fragNormalWorld);
    vec3 N = normalize(TBN * normalTangent);

    vec3 viewDir = normalize(gubo.eyePos - fragPosWorld);
    vec3 reflectDir = reflect(-viewDir, N);

    vec3 reflectionColor = sampleCubeFrom6Faces(reflectDir).rgb;

    float diffuse = max(dot(N, normalize(gubo.lightDir)), 0.0);
    vec3 waterColor = vec3(0.0, 0.4, 0.7);

    // Fresnel effect for water-like surface
    float F0 = 0.05; // Water-like reflectivity
    float cosTheta = clamp(dot(viewDir, N), 0.0, 1.0);
    float fresnel = fresnelSchlick(cosTheta, F0);

    // Specular highlight
    // Using a simple Blinn-Phong model for specular reflection
    // Adjust shininess factor as needed
    vec3 halfwayDir = normalize(viewDir + normalize(gubo.lightDir));
    float spec = pow(max(dot(N, halfwayDir), 0.0), 1000.0); // Adjust shininess
    vec3 specular = gubo.lightColor.rgb * spec;

//    vec3 finalColor = mix(waterColor * diffuse + specular, reflectionColor, fresnel);
    vec3 finalColor = mix(waterColor * diffuse + specular, reflectionColor, fresnel);
    outColor = vec4(finalColor, 1.0);
}
