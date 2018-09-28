/**
 * @brief Vertex shader for flat_projection_2darray.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::flat_projection_2darray
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

layout(location = 0) in vec2 vertex_position;
out vec2 tex_coord;
flat out uint face;

// 2D array coordinates: (u,v) in range [0,1]
const vec2 TEX_COORDS[4] = vec2(
    vec2( 0.f, 0.f),
    vec2( 1.f, 0.f),
    vec2( 1.f, 1.f),
    vec2( 0.f, 1.f)
);

void main() {
    gl_Position = vec4(vertex_position, 0.0, 1.0);
    tex_coord = TEX_COORDS[gl_VertexID % 4];
    face = gl_VertexID / 4;
}