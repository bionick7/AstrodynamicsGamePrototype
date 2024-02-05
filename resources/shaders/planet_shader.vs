#version 330

in vec3 vertexPosition;

out vec2 uv;
out vec4 world_pos_4;

uniform mat4 transform;
uniform mat4 mvp;

void main() {
	uv = vertexPosition.xy;
	world_pos_4 = transform * vec4(vertexPosition, 1);
	gl_Position = mvp * world_pos_4;
}