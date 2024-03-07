#version 330

in vec3 vertexPosition;
in vec4 vertexColor;

out float path;

uniform mat4 mvp;

void main() {
    gl_Position = mvp*vec4(vertexPosition, 1.0);
    path = vertexColor.a;
}