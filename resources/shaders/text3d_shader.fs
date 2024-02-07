#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform float ndcDepth;
uniform int useSdf;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main() {
    // Texel color fetching from texture sampler
	gl_FragDepth = (gl_DepthRange.diff * ndcDepth + gl_DepthRange.far + gl_DepthRange.near) / 2.0;
    if (useSdf > 0) {
        float outside_distance = texture(texture0, fragTexCoord).a - 0.5;
        float inside_distance = 0.5 - texture(texture0, fragTexCoord).a;
        float d_dist_d_frag = length(vec2(dFdx(outside_distance), dFdy(outside_distance))) * 0.5;
        float alpha = smoothstep(d_dist_d_frag, -d_dist_d_frag, outside_distance);
        finalColor = colDiffuse * fragColor;
        finalColor.a *= alpha;
    } else {
        vec4 texel_color = texture(texture0, fragTexCoord);
        finalColor = texel_color * colDiffuse * fragColor;
    }
}