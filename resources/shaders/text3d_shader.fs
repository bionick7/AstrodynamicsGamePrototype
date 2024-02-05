#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform float ndcDepth;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main() {
    // Texel color fetching from texture sampler
    vec4 texel_color = texture(texture0, fragTexCoord);

    finalColor = texel_color * colDiffuse * fragColor;
    //finalColor.rgb = vec3(ndcDepth);
	gl_FragDepth = (gl_DepthRange.diff * ndcDepth + gl_DepthRange.far + gl_DepthRange.near) / 2.0;
}