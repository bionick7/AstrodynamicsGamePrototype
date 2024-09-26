#version 330

in vec3 fragModelPos;
in vec4 fragColor;
in float proj_w;
in vec2 fragTexCoord;

uniform vec4 bgColor;
uniform float innerRad;
uniform mat4 mvp;

out vec4 finalColor;

vec2 get_screen_pos(vec3 world_pos) {
	vec4 clip = mvp * vec4(world_pos, 1);
	return clip.xy / clip.w;
}

void main() {
	vec2 uv = fragTexCoord * 2.0 - vec2(1);
	float r = length(uv);

	vec4 frag_pos = mvp * vec4(fragModelPos, 1);

	// TODO: Convert to analytic expression
	// d/dx [(M * v).y * inv((M * v).w)]
	// (M * v).y * d/dx[inv((M * v).w)] + d/dx[(M * v).y] * inv((M * v).w)
	// (M * v).y * -M_wx/(M * v).w^2 + M_yx / (M * v).w
	// 1 / (M * v).w * [M_yx - M_wx/(M * v).w]

	float w = (mvp * vec4(fragModelPos, 1)).w;
	vec2 screen_pos    = get_screen_pos(fragModelPos);
	vec2 screen_pos_dr = get_screen_pos(fragModelPos + vec3(uv/r * 0.01,0).xzy);
	float dscreen_drad = length(screen_pos_dr - screen_pos) / 0.01;

	float delta = 0.002 / dscreen_drad;
	float r_min = innerRad + delta;
	float r_max = 1.0 - delta;
	if (r < r_min - delta || r > r_max + delta) {
		discard;
	}
    float mask_outer = smoothstep(r_max + delta, r_max, r) * smoothstep(r_max - delta, r_max, r);
    float mask_inner = smoothstep(r_min + delta, r_min, r) * smoothstep(r_min - delta, r_min, r);
    float gradient = pow((r - r_min) / (r_max - r_min), 3.) * 0.3;
    //gradient = 1.0 - (1.0 - mask_outer) * (1.0 - mask_inner) * (1.0 - gradient);
	float mask = smoothstep(r_max + delta, r_max, r) * smoothstep(r_min - delta, r_min, r);

	finalColor = mix(bgColor, fragColor, gradient);
	finalColor.a = mask;

	//finalColor.rg = dscreen_duv * (uv / r);
	//finalColor.ba = vec2(1);
}