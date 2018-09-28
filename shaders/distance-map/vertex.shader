/**
 * @brief Vertex shader for distance-map.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::distance_map
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

layout(location = 0) in vec3 vertex_position;
out vec2 tex_coord;

void main() {
    gl_Position = vec4(vertex_position, 1.0f);
    tex_coord = (vertex_position.xy + 1.f) / 2.f;    
}