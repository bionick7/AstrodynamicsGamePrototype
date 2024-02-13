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

// NOTE: Add here your custom variables

void main() {
    // Texel color fetching from texture sampler
    //vec4 texel_color = texture(texture0, fragTexCoord);

    finalColor = texture(texture0, fragTexCoord);
    // Alpha scissor prevents unfortunate fading
    if (finalColor.a > 0.01) finalColor.a = 1.0;
    else discard;

    float depth_map = texture(depthMap, fragTexCoord).r;
    float z_n = 2.0 * depth_map - 1.0;
    float near = 0.01;
    float far = 1000.0;
    float z_e = 2.0 * near * far / (far + near - z_n * (far - near));
    //finalColor.rgb = 1. - vec3(z_e / 10.0);
    //finalColor.rgb = vec3(1) * depth_map * 20.0;
}