#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform float ndcDepth;
uniform int useSdf;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 backgroundColor;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main() {
    // Texel color fetching from texture sampler
	gl_FragDepth = (gl_DepthRange.diff * ndcDepth + gl_DepthRange.far + gl_DepthRange.near) / 2.0;
    float alpha = 1.0;
    if (useSdf > 0) {
        float outside_distance = texture(texture0, fragTexCoord).a - 0.5;
        float inside_distance = 0.5 - texture(texture0, fragTexCoord).a;
        float d_dist_d_frag = length(vec2(dFdx(outside_distance), dFdy(outside_distance))) * 0.5;
        alpha = smoothstep(d_dist_d_frag, -d_dist_d_frag, outside_distance);
    } else {
        alpha = texture(texture0, fragTexCoord).a;
    }
    finalColor = mix(backgroundColor, colDiffuse * fragColor, alpha);
    finalColor.a = fragColor.a;
    if (fragColor.a < 0.05) {
        discard;
    }
}