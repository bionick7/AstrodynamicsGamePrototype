#version 330

in float linear_input_buffer;
smooth out float anomaly;

uniform float semi_latus_rectum;
uniform float eccentricity;
uniform float current_anomaly;
uniform float focal_bound_start;
uniform float focal_range;

uniform mat4 mvp;

vec4 get_point(float x){
    float r = semi_latus_rectum / (1 + eccentricity * cos(x));
	return vec4(
		r * cos(x), 0.0f, r * sin(x), 1.0f
	);
}

void main() {
    anomaly = focal_bound_start + linear_input_buffer * focal_range;
	gl_Position = mvp * get_point(anomaly);
}
