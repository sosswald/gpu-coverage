/**
 * @brief Vertex shader for visual-vertexcolor.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::visual_vertexcolor
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_color;

out vec3 vcolor;

void main() {
    mat4 mvp = projection_matrix * view_matrix * model_matrix;
    gl_Position = mvp * vec4(vertex_position, 1.0);
    vcolor = vertex_color; 
}
