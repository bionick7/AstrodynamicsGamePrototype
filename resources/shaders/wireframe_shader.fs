#version 330

// Input uniform values
uniform int render_mode;
uniform vec4 color;
uniform float time;

// Output fragment color
out vec4 finalColor;
in float path;

// NOTE: Add here your custom variables

const int RENDER_MODE_SOLID = 0;
const int RENDER_MODE_GRADIENT = 1;
const int RENDER_MODE_DASHED = 2;

//const float PIx2 = 6.283185307;

void main() {
    finalColor.rgb = color.rgb;
    finalColor.a = 1.0;
    finalColor.r = fract(path*100.);
    //float anim_time = fract(time*.01);
    //if (path > anim_time) discard;
}
