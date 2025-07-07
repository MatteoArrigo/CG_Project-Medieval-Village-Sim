#version 450

/**
DEBUG VERSION
Sets a constant value of depth buffer
The initial clear value is 1.0, which will result in white terrain color (see TerrainShader.frag)
Instead if the depth buffer is being written correctly, it will be 0.1, resulting in almost black terrain color
*/

void main() {
    gl_FragDepth = 0.1;
}
