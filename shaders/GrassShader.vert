#version 450
#extension GL_ARB_separate_shader_objects : enable

/**
 * Vertex shader for rendering animated grass blades with wind sway effect.
 *
 * Inputs:
 *   - inPos (vec3, location = 0): Local position of the vertex.
 *   - inNorm (vec3, location = 1): Local normal vector.
 *   - inUV (vec2, location = 2): Texture coordinates.
 *   - inTangent (vec4, location = 3): Tangent vector (xyz) and handedness (w).
 *
 * Outputs:
 *   - fragPosWorld (vec3, location = 0): World-space position of the vertex.
 *   - fragNormalWorld (vec3, location = 1): World-space normal.
 *   - fragUV (vec2, location = 2): Texture coordinates passed to fragment shader.
 *   - fragTangent (vec3, location = 3): World-space tangent.
 *   - fragBitangent (vec3, location = 4): World-space bitangent.
 *
 * Uniform Buffers:
 *   - GeomUBO (mvpMat, mMat, nMat)
 *   - TimeUBO
 *
 * Constants:
 *   - windDir (vec3): Direction of wind.
 *   - frequency (float): Speed of swaying.
 *   - amplitude (float): Sway amount.
 *
 * Main Function:
 *   - Applies wind sway to the grass blade based on vertex height and time.
 *   - Transforms positions and normals to world space.
 *   - Computes tangent and bitangent in world space.
 *   - Outputs transformed vertex position for rasterization.
 */

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec3 fragPosWorld;
layout(location = 1) out vec3 fragNormalWorld;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;

// Object-local transform
layout(set = 1, binding = 0) uniform GeomUBO {
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;
} geomUbo;

layout(set = 1, binding = 1) uniform ShaodowClipUBO {
    mat4 lightVP;

/** Debug vector for shadow map rendering.
	 * If debug.x == 1.0, the terrain renders only white if lit and black if in shadow
	 * If debug.y == 1.0, the light's clip space is visualized instead of the basic perspective view
	 */
    vec4 debug;
} shadowClipUbo;

// Wind UBO
layout(set = 1, binding = 2) uniform TimeUBO {
    float time;
} timeUBO;


// Wind parameters
const vec3 windDir = normalize(vec3(1.0, 0.0, 1.0));  // Wind blowing along +X (you can rotate this)
const float frequency = 2.0;                         // Speed of swaying
const float amplitude = 0.1;                         // Sway amount

void main() {
    vec3 pos = inPos;

    // Wind effect: sway based on position and time
    // Sway is stronger at the top of the grass blade (pos.y = height) and null at the base (pos.y = 0)
    // Remember that pos.y is the height of the grass blade in local coordinates
    float sway = sin(pos.y * 5.0 + timeUBO.time * frequency) * amplitude;
    pos += windDir * sway * clamp(pos.y, 0.0, 2.0);

    // World-space calculations
    vec4 worldPos = geomUbo.mMat * vec4(pos, 1.0);
    fragPosWorld = worldPos.xyz;
    fragNormalWorld = normalize(geomUbo.nMat * vec4(inNorm, 0.0)).xyz;
    fragUV = inUV;

    // Compute tangent space basis from inTangent and inNorm
    vec3 localTangent = inTangent.xyz;
    vec3 localBitangent = cross(inNorm, localTangent) * inTangent.w;
    fragTangent = normalize((geomUbo.nMat * vec4(localTangent, 0.0)).xyz);
    fragBitangent = normalize((geomUbo.nMat * vec4(localBitangent, 0.0)).xyz);

    gl_Position = geomUbo.mvpMat * vec4(pos, 1.0);
}
