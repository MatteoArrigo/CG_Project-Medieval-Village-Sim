#version 450

/**
 * Vulkan GLSL fragment shader for water rendering with physically-based reflections and Fresnel effect.
 *
 * Features:
 * - Blends two normal maps for dynamic and detailed water surface simulation.
 * - Samples a cubemap using 6 separate 2D textures to achieve environment reflections.
 * - Implements the Fresnel-Schlick approximation for realistic, angle-dependent reflectance.
 * - Uses Blinn-Phong specular highlights for visually appealing light reflections.
 * - Outputs both color and alpha, providing transparency that varies with the viewing angle.
 *
 * Inputs:
 *   - fragUV1, fragUV2: UV coordinates for sampling two normal maps.
 *   - fragPosWorld: Fragment position in world space.
 *   - fragNormalWorld, fragTangentWorld, fragBitangentWorld: TBN basis vectors in world space.
 *
 * Uniforms:
 *   - LightModelUBO (lightUbo): Contains light direction, color, and camera position.
 *   - normalMap1, normalMap2: 2D normal maps for surface detail.
 *   - facePosX, faceNegX, facePosY, faceNegY, facePosZ, faceNegZ: 2D textures representing cubemap faces.
 *
 * Output:
 *   - outColor: RGBA color with transparency based on Fresnel effect.
 *
 * Usage:
 *   Designed for Vulkan pipelines requiring realistic water rendering with dynamic lighting and reflections.
 */

layout(location = 0) in vec2 fragUV1;
layout(location = 1) in vec2 fragUV2;
layout(location = 2) in vec3 fragPosWorld;
layout(location = 3) in vec3 fragNormalWorld;
layout(location = 4) in vec3 fragTangentWorld;
layout(location = 5) in vec3 fragBitangentWorld;

layout(location = 0) out vec4 outColor;

#define MAX_POINT_LIGHTS 20
layout(set = 0, binding = 0) uniform LightModelUBO {
    vec3 lightDir;
    vec4 lightColor;
    vec3 eyePos;

    vec3 pointLightPositions[MAX_POINT_LIGHTS];
    vec4 pointLightColors[MAX_POINT_LIGHTS];
    int nPointLights;
} lightUbo;

layout(set = 2, binding = 0) uniform IndexUBO {
    int index;
} indexUbo;

#define N_LIGHT_COLORS 4
layout(set = 2, binding = 1) uniform sampler2D normalMaps[2];
layout(set = 2, binding = 2) uniform sampler2D facePosX[N_LIGHT_COLORS];
layout(set = 2, binding = 3) uniform sampler2D faceNegX[N_LIGHT_COLORS];
layout(set = 2, binding = 4) uniform sampler2D facePosY[N_LIGHT_COLORS];
layout(set = 2, binding = 5) uniform sampler2D faceNegY[N_LIGHT_COLORS];
layout(set = 2, binding = 6) uniform sampler2D facePosZ[N_LIGHT_COLORS];
layout(set = 2, binding = 7) uniform sampler2D faceNegZ[N_LIGHT_COLORS];

/**
 Function to sample a cube map from 6 faces based on direction vector
 The direction vector should be normalized and in world space
 The function returns the color sampled from the appropriate face
 The cube map faces are expected to be bound to the corresponding texture units
 facePosX, faceNegX, facePosY, faceNegY, facePosZ, faceNegZ
*/
vec4 sampleCubeFrom6Faces(vec3 dir) {
    vec3 absDir = abs(dir);
    float ma;
    vec2 uv;

    if (absDir.x >= absDir.y && absDir.x >= absDir.z) {
        ma = absDir.x;
        if (dir.x > 0.0) {
            // +X
            uv = vec2(dir.z, -dir.y) / ma;
            return texture(facePosX[indexUbo.index], uv * 0.5 + 0.5);
        } else {
            // -X
            uv = vec2(-dir.z, -dir.y) / ma;
            return texture(faceNegX[indexUbo.index], uv * 0.5 + 0.5);
        }
    } else if (absDir.y >= absDir.x && absDir.y >= absDir.z) {
        ma = absDir.y;
        if (dir.y > 0.0) {
            // +Y
            uv = vec2(dir.x, -dir.z) / ma;
            return texture(facePosY[indexUbo.index], uv * 0.5 + 0.5);
        } else {
            // -Y
            uv = vec2(dir.x, dir.z) / ma;
            return texture(faceNegY[indexUbo.index], uv * 0.5 + 0.5);
        }
    } else {
        ma = absDir.z;
        if (dir.z > 0.0) {
            // +Z
            uv = vec2(-dir.x, -dir.y) / ma;
            return texture(facePosZ[indexUbo.index], uv * 0.5 + 0.5);
        } else {
            // -Z
            uv = vec2(dir.x, -dir.y) / ma;
            return texture(faceNegZ[indexUbo.index], uv * 0.5 + 0.5);
        }
    }
}

/**
 Function to sample a cube map from 6 faces based on direction vector
 The direction vector should be normalized and in world space
 The function returns the weighted sum of the color sampled from the 3 faces in the direction of dir
 The cube map faces are expected to be bound to the corresponding texture units
 facePosX, faceNegX, facePosY, faceNegY, facePosZ, faceNegZ
*/
vec3 sampleCubeInterpolated(vec3 dir) {
    vec3 dir2 = dir * dir;

    vec3 cxp = texture(facePosX[indexUbo.index], vec2(dir.z, -dir.y) / abs(dir.x) * 0.5 + 0.5).rgb;
    vec3 cxn = texture(faceNegX[indexUbo.index], vec2(-dir.z, -dir.y) / abs(dir.x) * 0.5 + 0.5).rgb;

    vec3 cyp = texture(facePosY[indexUbo.index], vec2(dir.x, -dir.z) / abs(dir.y) * 0.5 + 0.5).rgb;
    vec3 cyn = texture(faceNegY[indexUbo.index], vec2(dir.x, dir.z) / abs(dir.y) * 0.5 + 0.5).rgb;

    vec3 czp = texture(facePosZ[indexUbo.index], vec2(-dir.x, -dir.y) / abs(dir.z) * 0.5 + 0.5).rgb;
    vec3 czn = texture(faceNegZ[indexUbo.index], vec2(dir.x, -dir.y) / abs(dir.z) * 0.5 + 0.5).rgb;

    return ((dir.x > 0.0 ? cxp : cxn) * dir2.x +
    (dir.y > 0.0 ? cyp : cyn) * dir2.y +
    (dir.z > 0.0 ? czp : czn) * dir2.z);
}


/**
 * Computes the Fresnel-Schlick approximation for reflectance.
 * @param cosTheta The cosine of the angle between the view direction and the surface normal.
 * @param F0 The reflectance at normal incidence (typically ~0.04 for dielectrics).
 * @return The reflectance value based on the Fresnel-Schlick approximation.
 */
float fresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


/**
 * Rotates UV coordinates around the center (0.5, 0.5) by the given angle in radians.
 * Used to animate or vary normal map sampling for dynamic water surface effects.
 * @param uv The original UV coordinates.
 * @param angle The rotation angle in radians.
 * @return The rotated UV coordinates.
 */
vec2 rotateUV(vec2 uv, float angle) {
    float cosA = cos(angle);
    float sinA = sin(angle);
    vec2 centeredUV = uv - 0.5;
    vec2 rotatedUV = vec2(
    cosA * centeredUV.x - sinA * centeredUV.y,
    sinA * centeredUV.x + cosA * centeredUV.y
    );
    return rotatedUV + 0.5;
}

// Main diffuse color for the water surface
const vec3 mainDiffColor = vec3(0.0, 0.4, 0.7);

void main() {
    if(indexUbo.index < 0 || indexUbo.index >= N_LIGHT_COLORS)
        discard;  // Invalid index, skip rendering

    // ---- Sample 2 normal maps and blend them ----

    vec3 n1 = texture(normalMaps[0], rotateUV(fragUV1, radians(-20.0))).rgb * 2.0 - 1.0;  // [0,1] -> [-1,1]
    vec3 n2 = texture(normalMaps[1], fragUV2).rgb * 2.0 - 1.0;  // [0,1] -> [-1,1]

    // Blend the two normal maps
    // blendFactor is in [0,1], where 0 gives n1 and 1 gives n2
    float blendFactor = 0.3;
    vec3 normalTangent = normalize(mix(n1, n2, blendFactor));

    // Construct TBN matrix and transform normalTangent to world space
    mat3 TBN = mat3(fragTangentWorld, fragBitangentWorld, fragNormalWorld);
    vec3 N = normalize(TBN * normalTangent);


    // ----- Compute reflection through cubemap sampling -----

    vec3 viewDir = normalize(lightUbo.eyePos - fragPosWorld);
    vec3 reflectDir = reflect(-viewDir, N);

    vec3 reflectionColor = sampleCubeInterpolated(reflectDir).rgb;
    //    vec3 reflectionColor = sampleCubeFrom6Faces(reflectDir).rgb;


    // ---- Coefficients for mixing effects ----

    // Fresnel effect for water-like surface
    // Controls how much reflection vs diffuse color is used
    // For grazing angles, more reflection is used
    // Adjust F0 to control the strength of the Fresnel effect (0.04 is typical for water)
    float F0 = 0.05; // Increase to make reflections stronger
    float cosTheta = clamp(dot(viewDir, N), 0.0, 1.0);
    float fresnel = fresnelSchlick(cosTheta, F0);

    // Lambertian diffuse lighting --> Controls how much light is scattered in all directions
    float diffuse = max(dot(N, normalize(lightUbo.lightDir)), 0.0);


    // ---- Specular reflection with Blinn-Phong model, for sunlight ----

    vec3 halfwayDir = normalize(viewDir + normalize(lightUbo.lightDir));
    // Adjust the exponent to control the shininess of the surface (how large is the specular highlight)
    float spec = pow(max(dot(N, halfwayDir), 0.0), 200.0);
    // Adjust the intensity of the specular highlight
    vec3 specular = lightUbo.lightColor.rgb * spec * fresnel * 20;


    // ---- Final color computation ----

    vec3 finalColor = mix(mainDiffColor * diffuse + specular, reflectionColor, fresnel);
    float alpha = mix(0.4, 0.8, fresnel); // more transparent at grazing angles
    outColor = vec4(finalColor, alpha);
//    outColor = vec4(texture(faceNegZ[indexUbo.index], fragUV1).rgb, 1.0); // For testing, replace with finalColor later)
}
