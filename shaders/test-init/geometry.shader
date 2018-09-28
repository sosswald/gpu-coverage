/**
 * @brief Geometry shader for test-init.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::test_init
 * @class GeometryShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION shader_image_load_store

layout(points) in;
layout(points, max_vertices=1) out;
uniform layout(binding=5, r32i) writeonly iimage2D map;
uniform float resolution;
//const float resolution = 256.;

//in ivec2[] tex_coord;
out vec3 position;

void main() {
	ivec2 tex_coord[1] = ivec2[1](ivec2((gl_in[0].gl_Position.xy + 1.f) / 2.f * resolution));
	imageStore(map, tex_coord[0], ivec4(0, 0, 0, 0));
	position = gl_in[0].gl_Position.xyz;
	gl_Position = gl_in[0].gl_Position;
	EmitVertex();
	EndPrimitive();
}
