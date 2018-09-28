/**
 * @brief Geometry shader for pano-semantic.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pano_semantic
 * @class GeometryShader
 */

#version 440

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 model_matrix;
uniform mat4 view_matrix[6];
uniform mat4 projection_matrix;

in vec2 vertex_texcoord[];
out vec2 tex_coord;

void main() {
    for (int layer = 0; layer < 6; ++layer) {        
        for (int i = 0; i < gl_in.length(); ++i) {
            gl_Layer = layer;
            mat4 mvp = projection_matrix * view_matrix[layer] * model_matrix;
            gl_Position = mvp * gl_in[i].gl_Position;
            tex_coord = vertex_texcoord[i];
            EmitVertex();
        }
        EndPrimitive();
    }
}
