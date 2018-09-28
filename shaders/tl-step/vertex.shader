/**
 * @brief Vertex shader for tl-step.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::tl_step
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

uniform float width;
uniform float height;
layout(location = 0) in vec3 vertex_position;
out vec2 tex_coord;
out vec2 neighbors[3];

void main() {
    gl_Position = vec4(vertex_position, 1.0f);
    tex_coord = (vertex_position.xy + 1.f) / 2.f;    
    neighbors[0] = vec2(tex_coord.x - 1.5f / width, tex_coord.y);
    neighbors[1] = vec2(tex_coord.x, tex_coord.y - 1.5f / height);
    neighbors[2] = vec2(tex_coord.x, tex_coord.y + 1.5f / height);
}