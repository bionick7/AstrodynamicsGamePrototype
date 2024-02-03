#version 330 core

in vec2 uv;
in vec3 world_pos;

uniform float edge;
uniform float space;
uniform float radius;
uniform float screenWidth;

uniform vec3 cameraPos;  // in world_pos
uniform vec3 cameraDir;
uniform vec3 normal;
uniform vec4 rim_color;
uniform vec4 fill_color;

out vec4 finalColor;

void main() {
	vec3 cam_pos = cameraPos - world_pos;
	float cam_dist = length(cam_pos);
	float horizon_angle = acos(dot(-cam_pos / cam_dist, normal)) - 3.141592 / 2.0;  // both normalized
	float duv_dclip = cam_dist / (radius  * sin(horizon_angle) * sin(horizon_angle));
	float delta = duv_dclip * 1 / screenWidth;

	float draw_max = 1;

	float vis_max = 1 - edge * delta;
	float vis_min = vis_max - space * delta;

	float r = dot(uv, uv);
	
    const float feathering = 3.0;

	if(r > draw_max*draw_max + delta*feathering){
		discard;
	}

    float mask_outer = smoothstep(vis_max*vis_max + delta*feathering, vis_max*vis_max, r);
    float mask_inner = smoothstep(vis_min*vis_min - delta*feathering, vis_min*vis_min, r);
    float gradient = pow(r / (draw_max*draw_max), 10.0) * 0.3;
    float mask = 1.0 - (1.0 - mask_outer*mask_inner) * (1.0 - gradient);

	finalColor.rgb = mix(fill_color.rgb, rim_color.rgb, mask);
	finalColor.a = 1.0;
}