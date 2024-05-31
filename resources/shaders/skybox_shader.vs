#version 330

in vec3 vertexPosition;
in vec4 vertexColor;
in vec2 vertexTexCoord;

out vec2 fragTexCoord;
out vec4 fragColor;

void main() {
	fragColor = vertexColor;
	fragTexCoord = vertexPosition.xy;
	gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0.5, 1.0);
}