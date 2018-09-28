/**
 * @brief Vertex shader for dijkstra-step.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::dijkstra_step
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

uniform float resolution;
layout(location = 0) in vec3 vertex_position;
out vec2 tex_coord;
out vec2 neighbors[8];

const vec2 DELTA[8] = vec2[8](
    vec2(-1.f, -1.f),    
    vec2( 1.f, -1.f),
    vec2( 1.f,  1.f),
    vec2(-1.f,  1.f),
    vec2(-1.f,  0.f),
    vec2( 1.f,  0.f),
    vec2( 0.f, -1.f),
    vec2( 0.f,  1.f)
);

void main() {
    gl_Position = vec4(vertex_position, 1.0f);
    tex_coord = (vertex_position.xy + 1.f) / 2.f;
    for (int i = 0; i < 8; ++i) {
        neighbors[i] = tex_coord + DELTA[i].xy / resolution;
    }
        
}