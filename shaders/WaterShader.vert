#version 450

/**
 * WaterShader.vert
 *
 * Vertex shader for simulating an animated water surface.
 *
 * Features:
 * - Applies a sine wave displacement to the Y component of each vertex position to simulate water movement.
 * - Computes world-space normals by analytically deriving the surface gradient from the wave function.
 * - Calculates tangent and bitangent vectors in world space for correct normal mapping.
 * - Outputs two sets of UV coordinates (`fragUV1`, `fragUV2`) for animated normal mapping, each offset and scaled differently over time to create dynamic water surface detail.
 * - Passes world-space position, normal, tangent, and bitangent to the fragment shader for lighting and shading.
 *
 * Uniforms:
 * - `timeUBO`: Provides the current time for animating the wave and UVs.
 * - `UniformBufferObject`: Contains transformation matrices for model, view, projection, and normal calculations.
 *
 * Outputs:
 * - `fragPosWorld`: Vertex position in world space.
 * - `fragNormalWorld`: Normal vector in world space.
 * - `fragTangentWorld`: Tangent vector in world space.
 * - `fragBitangentWorld`: Bitangent vector in world space.
 * - `fragUV1`, `fragUV2`: UV coordinates for animated normal mapping.
 */

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec2 fragUV1;
layout(location = 1) out vec2 fragUV2;
layout(location = 2) out vec3 fragPosWorld;
layout(location = 3) out vec3 fragNormalWorld;
layout(location = 4) out vec3 fragTangentWorld;
layout(location = 5) out vec3 fragBitangentWorld;

layout(set = 0, binding = 0) uniform timeUBO {
    float time;
} time;

layout(set = 0, binding = 1) uniform UniformBufferObject {
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;
} ubo;

/**
    Parameters for the wave function representing the vertical displacement of the water surface.
    - `amplitude`: Amplitude of the wave (max displacement).
    - `waveDir`: Direction of wave propagation in the XZ plane. --> y coord should be 0.
    - `spatialFrequency`: Controls the wavelength of the wave.
            The higher the value, the more waves fit in the same space, the shorter the wavelength.
    - `waveSpeed`: Controls how fast the wave propagates over time.
            The higher the value, the faster the wave moves across the surface, the higher the frequency in time.
*/
const float amplitude = 0.3;
const vec3 waveDir = normalize(vec3(1.0, 0.0, -5.0));
const float spatialFrequency = 0.9;
const float waveSpeed = 0.9;

/**
    Normal mapping frequency for UV coordinates.
    These frequencies control how fast the UV coordinates for normal maps change over time, creating a dynamic effect.
*/
const float normFreq1 = 1e-2;
const float normFreq2 = 1e-3;

void main() {
    vec3 pos = inPosition;

    // Apply wave function to the Y component of the vertex position
    float phase = dot(pos.xz, waveDir.xz) * spatialFrequency - time.time * waveSpeed;
    pos.y = amplitude * sin(phase);

    // Transform position to world space
    vec4 worldPos = ubo.mMat * vec4(pos, 1.0);
    fragPosWorld = worldPos.xyz;

    // Normal in world space
    // Calculate the gradient of the wave function (analytically) to derive the normal vector
    float dHdx = cos(phase) * spatialFrequency * waveDir.x * amplitude;
    float dHdz = cos(phase) * spatialFrequency * waveDir.z * amplitude;
    vec3 dx = vec3(1.0, dHdx, 0.0);
    vec3 dz = vec3(0.0, dHdz, 1.0);
    vec3 N = normalize(cross(dz, dx));
    fragNormalWorld = normalize((ubo.nMat * vec4(N, 0.0)).xyz);

    // Approximate tangent and bitangent for a planar surface in XZ
    vec3 T = normalize((ubo.mMat * vec4(1.0, 0.0, 0.0, 0.0)).xyz); // X direction
    vec3 B = normalize(cross(N, T)); // Y upward, so this makes sense for a horizontal plane
    fragTangentWorld = T;
    fragBitangentWorld = B;

    // Transform position to clip space
    // This is the final position that will be used for rendering
    gl_Position = ubo.mvpMat * vec4(pos, 1.0);

    // Calculate UV coordinates for animated normal mapping
    // These UVs are offset by the position and scaled by time to create a dynamic effect
    fragUV1 = pos.xz * 0.1 + vec2(time.time * normFreq1, 0);    // UV1 changes over time in the X direction
    fragUV2 = pos.xz * 0.1 + vec2(0, time.time * normFreq2);    // UV2 changes over time in the Z direction

}
