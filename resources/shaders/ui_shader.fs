#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float depth;
//uniform float dSDFsFrag;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main() {
    finalColor = fragColor * colDiffuse * texture(texture0, fragTexCoord);
    gl_FragDepth = depth;
}