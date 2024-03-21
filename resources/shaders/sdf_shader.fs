#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 background_color;
uniform float depth;
//uniform float dSDFsFrag;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main() {

    float sdf = (texture(texture0, fragTexCoord).a - 0.5) * 2.0;
    float d_dist_d_frag = length(vec2(dFdx(sdf), dFdy(sdf)));
    float alpha = smoothstep(d_dist_d_frag*.5, -d_dist_d_frag*.5, sdf);
    //alpha *= smoothstep(d_dist_d_frag*2.0, d_dist_d_frag*1.0, -sdf);
    finalColor = mix(background_color, fragColor, alpha);
    //finalColor.rgb = fragColor.rgb;
    //finalColor.a = alpha;
    gl_FragDepth = depth;

    //finalColor.a = 1.0;
    //finalColor.rgb = texture(texture0, fragTexCoord).rgb;
    //finalColor.rgb = vec3(1) * smoothstep(.5, .1, inside_distance); //texture(texture0, fragTexCoord).a;
    //finalColor.rgb = vec3(1) * smoothstep(d_dist_d_frag*20.0, d_dist_d_frag*1.0, inside_distance); //texture(texture0, fragTexCoord).a;
    //finalColor.rgb = vec3(1) * d_dist_d_frag;
}