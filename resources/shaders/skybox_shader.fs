#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

const int palette_size = 4;

uniform vec2 resolution;
uniform float fov;
uniform vec4 palette[palette_size];
uniform mat4 matCamera;

uniform sampler2D starMap;

out vec4 finalColor;
const float PI = 3.141592;

void main() {
	//finalColor = mix(bgColor, fragColor, step(0.8, rand(lookDir)));
	float aspect_ratio = resolution.x / resolution.y;
	
	vec3 ray = vec3(
		fragTexCoord
		 * tan(fov / 2.0) * vec2(aspect_ratio, 1.0),
		-1.0);

	vec3 lookDir = normalize(mat3(matCamera) * ray);
	vec2 spherical = vec2(
		atan(lookDir.z, lookDir.x),
		asin(lookDir.y)
	);
	spherical.x = spherical.x / (2.0*PI) + .5;
	spherical.y = spherical.y / PI + .5;
	float texture_luma = dot(texture(starMap, spherical).rgb, vec3(.3333));

	finalColor = palette[1];
	for(int i=0; i < palette_size; i++) {
		if (texture_luma*palette_size <= i+1) {
			finalColor = palette[i];
			break;
			
		}
	}
	gl_FragDepth = 1.0;
}
