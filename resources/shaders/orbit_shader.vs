#version 330

in vec2 linear_input_buffer;
smooth out float anomaly;
smooth out float lateral_offset;

uniform vec3 center;
uniform float semi_latus_rectum;
uniform float eccentricity;
uniform float current_anomaly;
uniform float focal_bound_start;
uniform float focal_range;
uniform vec2 window_size;

uniform mat4 mvp;

vec3 get_point(vec2 sc) {
    float r = semi_latus_rectum / (1 + eccentricity * sc.y);
	vec3 offset = r * vec3(sc.y, 0.0f, sc.x);
	return vec3(center + offset);
}

vec3 get_direction(vec2 sc) {	
    float cot_gamma = -eccentricity * sc.x / (1 + eccentricity * sc.y);
    vec2 local_vel = normalize(vec2(1, cot_gamma));
	//vec2 d = mat2(sc.yx, sc * vec2(-1,1)) * local_vel;
	vec2 d = mat2(sc * vec2(-1,1), -sc.yx) * local_vel;
	return vec3(d.x, 0, d.y);
}

const float EPSILON = 0.01;

void main() {
    anomaly = focal_bound_start + linear_input_buffer.x * focal_range;
	vec2 sc = vec2(sin(anomaly), cos(anomaly));
	vec3 point_model = get_point(sc);
	vec3 direction_model = get_direction(sc);
	//point_model += linear_input_buffer.y * direction_model * 0.1;

	vec4 clip_pos = mvp * vec4(point_model, 1);
	vec4 direction_clip_delta = mvp * vec4(point_model + direction_model * EPSILON, 1);
	vec2 direction_clip = (direction_clip_delta.xy / direction_clip_delta.w - clip_pos.xy / clip_pos.w) / EPSILON;

	vec2 orth_direction_clip = normalize(direction_clip).yx * vec2(1,-1);
	vec2 ndc_offset = 3.0f / window_size;
	clip_pos.xy += linear_input_buffer.y * clip_pos.w * ndc_offset * orth_direction_clip;
	lateral_offset = linear_input_buffer.y;
	gl_Position = clip_pos;
}
