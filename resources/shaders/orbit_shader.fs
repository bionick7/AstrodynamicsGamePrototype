#version 330

// Input vertex attributes (from vertex shader)
smooth in float anomaly;
smooth in float lateral_offset;
flat in int orbit_index;

// Input uniform values
uniform int render_mode[100];
uniform float current_anomaly[100];
uniform vec4 color[100];

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

const int RENDER_MODE_SOLID = 0;
const int RENDER_MODE_GRADIENT = 1;
const int RENDER_MODE_DASHED = 2;

const float PIx2 = 6.283185307;

void main() {
	float path_offset = fract((anomaly - current_anomaly[orbit_index] + PIx2) / PIx2);
    finalColor = color[orbit_index];
    if (render_mode[orbit_index] == RENDER_MODE_SOLID) {
        finalColor.a = 1.0;
    }
    else if (render_mode[orbit_index] == RENDER_MODE_GRADIENT) {
        finalColor.a = path_offset * (1. - step(0.997, path_offset));
    }
    //finalColor.a = smoothstep(1.0f, 0.0f, lateral_offset*lateral_offset);
    finalColor.a *= smoothstep(1.0f, 0.0f, abs(lateral_offset));
    //finalColor.a = 1.0;
}
