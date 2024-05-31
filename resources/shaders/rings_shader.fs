#version 330

in vec4 fragColor;
in float proj_w;
in vec2 fragTexCoord;

uniform vec4 bgColor;
uniform float innerRad;
uniform mat4 mvp;

out vec4 finalColor;

void main() {
	vec2 uv = fragTexCoord * 2.0 - vec2(1);
	float r = length(uv);

	vec2 dscreen_duvx = (mvp * vec4(1, 0, 0, 0)).xy / proj_w;
	vec2 dscreen_duvy = (mvp * vec4(0, 0, 1, 0)).xy / proj_w;
	mat2 dscreen_duv = mat2(dscreen_duvx, dscreen_duvy);  // Jacobian
	float d_screen_drad = length(dscreen_duv * (uv / r));
	//float d_screen_drad = dot(dscreen_duvx, dscreen_duvx) + dot(dscreen_duvy, dscreen_duvy);
	//float d_screen_drad = length(vec4(dscreen_duvx, dscreen_duvy));

	float delta = 0.002 / d_screen_drad;
	float r_min = innerRad + delta;
	float r_max = 1.0 - delta;
	if (r < r_min - delta || r > r_max + delta) {
		discard;
	}
    float mask_outer = smoothstep(r_max + delta, r_max, r) * smoothstep(r_max - delta, r_max, r);
    float mask_inner = smoothstep(r_min + delta, r_min, r) * smoothstep(r_min - delta, r_min, r);
    float gradient = pow((r - r_min) / (r_max - r_min), 3.) * 0.3;
    gradient = 1.0 - (1.0 - mask_outer) * (1.0 - mask_inner) * (1.0 - gradient);
	float mask = smoothstep(r_max + delta, r_max, r) * smoothstep(r_min - delta, r_min, r);

	finalColor = mix(bgColor, fragColor, gradient);
	finalColor.a = mask;

	//finalColor.rg = dscreen_duvy;
}