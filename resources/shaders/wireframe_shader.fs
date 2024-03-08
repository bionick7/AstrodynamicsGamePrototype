#version 330

// Input uniform values
uniform int render_mode;
uniform vec4 color;
uniform float time;

// Output fragment color
out vec4 finalColor;
smooth in vec4 path;

// NOTE: Add here your custom variables

const int RENDER_MODE_SOLID = 0;
const int RENDER_MODE_GRADIENT = 1;
const int RENDER_MODE_DASHED = 2;

//const float PIx2 = 6.283185307;

void main() {
    finalColor.rgb = color.rgb;
    finalColor.a = 1.0;
    //float anim_time = fract(time*.01);
    //finalColor.a = sin(path.a*100.0 - time*0.1) * .5 + .5;
}
