#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D depthMap;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

const vec3 fade_color = vec3(0,1,.5);

vec3 visualize_depth_map(float depthmap_value) {
    float z_n = 2.0 * depthmap_value - 1.0;
    float near = 0.01;
    float far = 1000.0;
    float z_e = 2.0 * near * far / (far + near - z_n * (far - near));
    return vec3(1. - z_e / 10.0);
}

float sdf_box( vec2 p, vec2 b ) {
  vec2 q = abs(p) - b;
  return length(max(q, 0.0)) + min(max(q.x, q.y),0.0);
}

void main() {
    // Texel color fetching from texture sampler
    //vec4 texel_color = texture(texture0, fragTexCoord);

    vec2 uv = fragTexCoord*2. -1.0;

    float edge_dist = 1. + sdf_box(uv, vec2(1.));
    float fade_factor_color = smoothstep(0.8, 1.0, edge_dist) * 0.02;
    float fade_factor_distortion = edge_dist * edge_dist * 0.01;

    uv.y *= 1. + fade_factor_distortion;
    uv.x *= 1. + fade_factor_distortion;

    vec4 tex_color = texture(texture0, uv*.5+.5);
    vec2 screen_mask_v2 = step(vec2(-1), uv) * step(uv, vec2(1));
    float screen_mask = screen_mask_v2.x * screen_mask_v2.y;
    // Alpha scissor prevents unfortunate fading
    if (tex_color.a > 0.01) finalColor.a = 1.0;
    else discard;
    vec3 col = tex_color.rgb;
    col = mix(fade_color * 0.1, col, screen_mask);

    //float fade_factor = smoothstep(0.6, 1.2, 1. - fragTexCoord.y) * 0.1;
    col = mix(col, fade_color, fade_factor_color);
    //col = vec3(1) * edge_dist;

    col = visualize_depth_map(texture(depthMap, fragTexCoord).r);
    
    finalColor.rgb = col;
}