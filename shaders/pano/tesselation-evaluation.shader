/**
 * @brief TesselationEvaluation shader for pano.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pano
 * @class TesselationEvaluationShader
 */

#version 440
// EXTENSION shading_language_420pack
 
layout(triangles, equal_spacing, ccw) in;
in vec3 tvertex_color[];
in vec3 tvertex_normal[];
in vec2 tvertex_texcoord[];
out vec3 vertex_color;
out vec3 vertex_normal;
out vec2 vertex_texcoord;
 
vec3 interpolate3(in vec3 v0, in vec3 v1, in vec3 v2)
{
  return (gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2); 
}
vec2 interpolate2(in vec2 v0, in vec2 v1, in vec2 v2)
{
  return (gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2); 
}
 
void main()
{ 
 gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position +
                gl_TessCoord.y * gl_in[1].gl_Position +
                gl_TessCoord.z * gl_in[2].gl_Position); 
 vertex_color = interpolate3(tvertex_color[0], tvertex_color[1], tvertex_color[2]);
 vertex_normal = interpolate3(tvertex_normal[0], tvertex_normal[1], tvertex_normal[2]);
 vertex_texcoord = interpolate2(tvertex_texcoord[0], tvertex_texcoord[1], tvertex_texcoord[2]);
}
