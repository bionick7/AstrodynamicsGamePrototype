#version 330

in vec3 vertexPosition;
in vec4 vertexColor;

smooth out vec4 path;

uniform mat4 mvp;

void main() {
    gl_Position = mvp*vec4(vertexPosition, 1.0);
    path = vertexColor;
}