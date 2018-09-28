/**
 * @brief Vertex shader for counter_to_fb.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::counter_to_fb
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

layout(location = 0) in vec3 vertex_position;

void main() {
  gl_Position = vec4(vertex_position, 1.0);
}
