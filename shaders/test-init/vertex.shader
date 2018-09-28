/**
 * @brief Vertex shader for test-init.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::test_init
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

uniform float resolution;
uniform vec2 robot_position;

out ivec2 tex_coord;

void main() {
    gl_Position = vec4(robot_position, 0.f, 1.f);
    tex_coord = ivec2((robot_position + 1.f) / 2.f * resolution);
}