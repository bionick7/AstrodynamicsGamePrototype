#version 330

in vec2 uv;
in vec4 world_pos_4;

out vec4 finalColor;

uniform float edge;
uniform float space;
uniform float radius;
uniform float screenWidth;

uniform vec3 cameraPos;  // in world_pos
uniform vec3 centerPos;
uniform vec3 normal;
uniform vec4 rimColor;
uniform vec4 fillColor;

uniform mat4 transform;
uniform mat4 mvp;


float trace(vec3 ray_origin, vec3 ray_dir, float r) {
	float b = 2.0 * dot(ray_dir, ray_origin);
	float c = dot(ray_origin, ray_origin) - r*r;
	float delta = b*b - 4.*c;  // a = 1
	if (delta < 0.) {
		return -1.0;
	} 
	float l = (-b - sqrt(delta)) / 2.;
	if (l < 0.0) l = (-b + sqrt(delta)) / 2.;
	return l;
}

void main() {
	vec3 world_pos = world_pos_4.xyz;
	//vec3 world_pos = (transform * vec4(uv, 0, 1)).xyz;
	vec3 cam_pos = world_pos - cameraPos;
	float cam_dist = length(cam_pos);
	vec3 camera_ray_dir = cam_pos / cam_dist;
	float horizon_angle = acos(dot(camera_ray_dir, normal)*.999) - 3.141592 / 2.0;  // both normalized
	float duv_dclip = cam_dist / (radius  * sin(horizon_angle) * sin(horizon_angle));
	float delta = duv_dclip * 1 / screenWidth;

	// raytracing
	float l = trace(cameraPos - centerPos, camera_ray_dir, radius);
	vec3 true_world_pos = cameraPos + camera_ray_dir * l;

	// geometry to figure out radius
	/*float plane_cam_dist = cos(horizon_angle) * cam_dist;
	float plane_size = length(transform[0].xyz);
	float viewing_angle = atan(plane_size / plane_cam_dist);
	float border_ray = plane_cam_dist / cos(viewing_angle);
	float R = border_ray * tan(viewing_angle);*/

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

	finalColor.rgb = mix(fillColor.rgb, rimColor.rgb, mask);
	finalColor.a = 1.0;
	
	//finalColor.rgba = vec4(0,0,0,1);
	//finalColor.rgb = true_world_pos;
	//finalColor.rg = uv;

	vec4 p_clip = mvp * vec4(world_pos, 1.0);

	float ndc_depth = p_clip.z / p_clip.w;
	gl_FragDepth = (gl_DepthRange.diff * ndc_depth + gl_DepthRange.far + gl_DepthRange.near) / 2.0;
}