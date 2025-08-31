#version 450

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

#define N_SUNLIGHTS 4
layout(set = 2, binding = 1) uniform sampler2D normalMaps[2];
layout(set = 2, binding = 2) uniform sampler2D envMap[N_SUNLIGHTS]; // Equirectangular texture

// Convert 3D direction vector to UVs for equirectangular map
vec2 dirToEquirectUV(vec3 dir) {
    dir = normalize(dir);
    // Compute sort of spherical coordinates for dir
    float yaw = -(atan(dir.x, dir.z)/6.2831853+0.5);        // Angle in XZ plane
    float pitch = -(atan(dir.y, sqrt(dir.x*dir.x+dir.z*dir.z))/3.14159265+0.5);     // vertical angle from plane XZ
    return vec2(yaw, pitch); // Flip V to match texture convention
}

float fresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

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

const vec3 mainDiffColor = vec3(0.0, 0.2, 0.35);

void main() {
    if(indexUbo.index < 0 || indexUbo.index >= N_SUNLIGHTS)
        discard;  // Invalid index, skip rendering

    // ---- Sample 2 normal maps and blend them ----

    vec3 n1 = texture(normalMaps[0], rotateUV(fragUV1, radians(-20.0))).rgb * 2.0 - 1.0;  // [0,1] -> [-1,1]
    vec3 n2 = texture(normalMaps[1], fragUV2).rgb * 2.0 - 1.0;  // [0,1] -> [-1,1]

    // Blend the two normal maps
    // blendFactor is in [0,1], where 0 gives n1 and 1 gives n2
    float blendFactor = 0.3;
    vec3 normalTangent = normalize(mix(n1, n2, blendFactor));
    mat3 TBN = mat3(fragTangentWorld, fragBitangentWorld, fragNormalWorld);
    vec3 N = normalize(TBN * normalTangent);


    // ----- Compute reflection through cubemap sampling -----

    vec3 viewDir = normalize(lightUbo.eyePos - fragPosWorld);
    vec3 reflectDir = reflect(-viewDir, N);

    // Sample from equirectangular map
    vec2 envUV = dirToEquirectUV(reflectDir);
    vec3 reflectionColor = texture(envMap[indexUbo.index], envUV).rgb;

    // ---- Coefficients for mixing effects ----

    // Fresnel effect for water-like surface
    // Controls how much reflection vs diffuse color is used
    // For grazing angles, more reflection is used
    // Adjust F0 to control the strength of the Fresnel effect (0.04 is typical for water)
    float F0 = 0.1; // Increase to make reflections stronger
    float cosTheta = clamp(dot(viewDir, N), 0.0, 1.0);
    float fresnel = fresnelSchlick(cosTheta, F0);

    // Lambertian diffuse lighting --> Controls how much light is scattered in all directions
    float diffuse = max(dot(N, normalize(lightUbo.lightDir)), 0.0);


    // ---- Specular reflection with Blinn-Phong model, for sunlight ----

    vec3 halfwayDir = normalize(viewDir + normalize(lightUbo.lightDir));
    // Adjust the exponent to control the shininess of the surface (how large is the specular highlight)
    // The greater the exponent, the shinier the surface, the thinner the specular highlight
    float spec = pow(max(dot(N, halfwayDir), 0.0), 300.0);
    // Adjust the intensity of the specular highlight
    vec3 specular = lightUbo.lightColor.rgb * spec * fresnel * 15.0;


    // ---- Final color computation ----

    vec3 finalColor = mix(mainDiffColor * diffuse + specular, reflectionColor, fresnel);
    float alpha = mix(0.3, 0.9, fresnel); // more transparent at grazing angles
    outColor = vec4(finalColor, alpha);
}
