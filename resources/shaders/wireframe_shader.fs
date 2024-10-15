#version 330


// Input uniform values
uniform int render_mode;
uniform vec4 color;
uniform float time;
uniform float depth;
uniform float depth_offset;

// Output fragment color
out vec4 finalColor;
smooth in vec3 normal;

// NOTE: Add here your custom variables

const int RENDER_MODE_SOLID = 0;
const int RENDER_MODE_GRADIENT = 1;
const int RENDER_MODE_DASHED = 2;
const int RENDER_MODE_FACES = 3;

//const float PIx2 = 6.283185307;

void main() {
    finalColor.rgb = color.rgb;
    finalColor.a = 1.0;

    if (depth >= 0.0) {
        gl_FragDepth = depth;
    } else {
        gl_FragDepth = gl_FragCoord.z + depth_offset / 256.0f;
    }
}
