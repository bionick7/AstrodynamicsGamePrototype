#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main() {
    // Texel color fetching from texture sampler
    //float texel_value = texture(texture0, fragTexCoord).r;
    //if (texel_value < 0.5) discard;
    float outside_distance = texture(texture0, fragTexCoord).a - 0.5;
    float inside_distance = 0.5 - texture(texture0, fragTexCoord).a;
    float d_dist_d_frag = length(vec2(dFdx(outside_distance), dFdy(outside_distance))) * 0.5;
    float alpha = smoothstep(d_dist_d_frag, -d_dist_d_frag, outside_distance);
    finalColor.rgb = fragColor.rgb;
    finalColor.a = fragColor.a * alpha;
}
