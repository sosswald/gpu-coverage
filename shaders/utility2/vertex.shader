/**
 * @brief Vertex shader for utility2.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::utility2
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

layout(location = 0) in vec3 vertex_position;
out vec2 tex_coords[9];

void main() {
    gl_Position = vec4(vertex_position, 1.0);    
    vec2 center = (vertex_position.xy + 1.f) / 2.f;
    float d = 1. / 256. ;   // TODO
    tex_coords[0] = center + vec2(-d, -d);
    tex_coords[1] = center + vec2(-d,  0);
    tex_coords[2] = center + vec2(-d,  d);
    tex_coords[3] = center + vec2( 0, -d);
    tex_coords[4] = center;                
    tex_coords[5] = center + vec2( 0,  d);
    tex_coords[6] = center + vec2( d, -d);
    tex_coords[7] = center + vec2( d,  0);
    tex_coords[8] = center + vec2( d, -d); 
}

