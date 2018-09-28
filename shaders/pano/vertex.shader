/**
 * @brief Vertex shader for pano.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pano
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

layout(location = 0) in vec3 vertex_positionIn;
layout(location = 1) in vec3 vertex_colorIn;
layout(location = 2) in vec2 vertex_texIn;
layout(location = 3) in vec3 vertex_normalIn;

out vec3 vertex_color;
out vec3 vertex_normal;
out vec2 vertex_texcoord;

void main() {
    gl_Position = vec4(vertex_positionIn, 1.0);
    vertex_color = vertex_colorIn;
    vertex_normal = vertex_normalIn;
    vertex_texcoord = vertex_texIn;
}
