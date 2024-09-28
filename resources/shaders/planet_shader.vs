#version 330

in vec3 vertexPosition;

smooth out vec2 uv;
smooth out vec4 world_pos_4;
flat out int planet_index;

uniform mat4 mvp;
uniform mat4 transform[100];

void main() {
	planet_index = gl_VertexID/6;
	uv = vertexPosition.xy;
	world_pos_4 = transform[planet_index] * vec4(vertexPosition, 1);
	gl_Position = mvp * world_pos_4;
}