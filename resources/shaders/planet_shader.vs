#version 330 core

in vec3 vertexPosition;
out vec2 uv;
out vec3 world_pos;

uniform mat4 transform;
uniform mat4 mvp;

void main() {
	vec4 intermediate = transform * vec4(vertexPosition, 1);
	world_pos = intermediate.xyz;
	gl_Position = mvp * intermediate;
	uv = vertexPosition.xy;
}