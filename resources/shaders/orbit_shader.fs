#version 330

// Input vertex attributes (from vertex shader)
smooth in float anomaly;

// Input uniform values
uniform int render_mode;
uniform float current_anomaly;
uniform vec4 color;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

const int RENDER_MODE_SOLID = 0;
const int RENDER_MODE_GRADIENT = 1;
const int RENDER_MODE_DASHED = 2;

const float PIx2 = 6.283185307;

void main() {
	float path_offset = fract((anomaly - current_anomaly + PIx2) / PIx2);
    finalColor.rgb = color.rgb;
    if (render_mode == RENDER_MODE_SOLID) {
        finalColor.a = 1.0;
    }
    else if (render_mode == RENDER_MODE_GRADIENT) {
        finalColor.a = path_offset * (1. - step(0.997, path_offset));
    }
}
