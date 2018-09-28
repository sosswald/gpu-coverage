/**
 * @brief Fragment shader for flat_projection_cubemap.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::flat_projection_cubemap
 * @class FragmentShader
 */

#version 440

in vec2 tex_coord;
flat in uint face;
uniform samplerCube texture_unit;

out vec4 color;

void main() {
  vec3 t;
  // see OpenGL standard 4.5 core table 8.19
  switch(face) {
  case 0: t = vec3(         1.f, -tex_coord.y, -tex_coord.x); break;
  case 1: t = vec3(        -1.f, -tex_coord.y,  tex_coord.x); break;
  case 2: t = vec3( tex_coord.x,          1.f,  tex_coord.y); break;
  case 3: t = vec3( tex_coord.x,         -1.f, -tex_coord.y); break;
  case 4: t = vec3( tex_coord.x, -tex_coord.y,  1.f);         break;
  case 5: t = vec3(_tex_coord.x, -tex_coord.y, -1.f);         break;
  }
  color = texture(texture_unit, t);
}
