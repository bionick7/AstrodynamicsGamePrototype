#version 330

// Input vertex attributes (from vertex shader)
in float path_offset;

// Input uniform values
uniform vec4 colDiffuse;
uniform vec4 color;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main() {
    finalColor.rgb = color.rgb;
    finalColor.a = path_offset;
}

