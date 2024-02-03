#version 330
/*
// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 fragColor;

// NOTE: Add here your custom variables
uniform mat4 mvp;

vec3 rotate_axis_angle(vec3 v, vec3 axis, float angle) {
    // Using Euler-Rodrigues Formula
    // Ref.: https://en.wikipedia.org/w/index.php?title=Euler%E2%80%93Rodrigues_formula

    axis = normalize(axis);

    vec3 w = axis * sin(angle * 0.5);
    float a = cos(angle * 0.5);

    vec3 wv = cross(w, v);
    return v + 2.0*a*wv + 2.0*cross(w, wv);
}

void main() {
    // Send vertex attributes to fragment shader

    vec3 ecc_vector = vertexPosition;
    ecc_vector.z = vertexColor.w;

    float ecc = length(ecc_vector);
    float p = length(vertexColor.xyz);
    vec3 periapsis_dir = ecc_vector / ecc;
    vec3 normal = vertexColor.xyz / p;

    float focal = vertexPosition.z;
    float r = p / (1 + ecc * cos(focal));
    vec3 dir = rotate_axis_angle(periapsis_dir, normal, focal);
    vec3 pos = r * dir;

    fragColor = vec4(1, 1, 1, 1);

    // Calculate final vertex position
    gl_Position = mvp*vec4(pos, 1.0);
}*/


//layout(location = 0) in float anomaly;

in vec3 vertexPosition;
out float path_offset;

uniform mat4 orbit_transform;
uniform float semi_latus_rectum;
uniform float eccentricity;
uniform mat4 mvp;

/*vec4 get_point(float x){
    float r = semi_latus_rectum / (1 + eccentricity * cos(x));
	return orbit_transform * vec4(
		r * cos(x), 0, r * sin(x), 1
	);
}*/

void main() {
    //float anomaly = vertexPosition.x;
	//gl_Position = mvp * get_point(anomaly);
    gl_Position = mvp * vec4(vertexPosition, 1);
	//path_offset = anomaly / 6.283;
}