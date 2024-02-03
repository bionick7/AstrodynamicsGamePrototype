#version 330

in float vertexPosition;
out float path_offset;

uniform mat4 orbit_transform;
uniform float semi_latus_rectum;
uniform float eccentricity;
uniform float current_anomaly;
uniform mat4 mvp;

const float PIx2 = 6.283185307;

vec4 get_point(float x){
    float r = semi_latus_rectum / (1 + eccentricity * cos(x));
	return orbit_transform * vec4(
		r * cos(x), 0, r * sin(x), 1
	);
}

void main() {
    float anomaly = vertexPosition;
	gl_Position = mvp * get_point(anomaly);
	path_offset = 1.0 - fract((anomaly + current_anomaly + PIx2) / PIx2);
}