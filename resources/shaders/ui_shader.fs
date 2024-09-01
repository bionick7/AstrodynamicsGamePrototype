#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 background_color;
uniform float depth;
//uniform float dSDFsFrag;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main() {
    vec4 col = texture(texture0, fragTexCoord);
    //if (col.a <= 0.5) discard; else col.a = 1.0;  // Currently doesn't support transparency
    finalColor = mix(background_color, fragColor * colDiffuse, col.a);
    //finalColor.a = 1.0;
    gl_FragDepth = depth;
}