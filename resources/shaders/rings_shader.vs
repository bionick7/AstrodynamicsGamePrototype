#version 330

in vec3 vertexPosition;
in vec4 vertexColor;
in vec2 vertexTexCoord;

out vec4 fragColor;
out float proj_w;
out vec2 fragTexCoord;

uniform mat4 mvp;

void main() {
	fragColor = vertexColor;
	fragTexCoord = vertexTexCoord;

	vec4 pos4 = mvp * vec4(vertexPosition, 1);
	proj_w = pos4.w;
	gl_Position = pos4;
}