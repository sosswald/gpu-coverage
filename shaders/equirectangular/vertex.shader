/**
 * @brief Vertex shader for equirectangular.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::equirectangular
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

layout(location = 0) in vec3 vertex_position;
out vec2 latLon;

#define PI      3.14159265358979323846
#define HALF_PI 1.57079632679489661923

void main() {
    // input:    x = [-1, 1],      y = [-1, 1]
    // output: phi = [0, 2pi], theta = [0, pi]
    latLon = vec2((vertex_position.x + 1.) * PI, (vertex_position.y + 1.) * HALF_PI);
    gl_Position = vec4(vertex_position, 1.0);
}