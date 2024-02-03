#version 330

// Input vertex attributes (from vertex shader)
in float path_offset;

// Input uniform values
uniform int render_mode;
uniform vec4 color;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

const int RENDER_MODE_SOLID = 0;
const int RENDER_MODE_GRADIENT = 1;
const int RENDER_MODE_DASHED = 2;

void main() {
    finalColor.rgb = color.rgb;
    if (render_mode == RENDER_MODE_SOLID) {
        finalColor.a = 1.0;
    }
    else if (render_mode == RENDER_MODE_GRADIENT) {
        finalColor.a = path_offset;
    }
}
