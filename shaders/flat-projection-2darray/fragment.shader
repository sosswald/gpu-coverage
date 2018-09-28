/**
 * @brief Fragment shader for flat_projection_2darray.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::flat_projection_2darray
 * @class FragmentShader
 */

#version 440
// EXTENSION texture_array

in vec2 tex_coord;
flat in uint face;
uniform sampler2DArray texture_unit;

out vec4 color;

void main() {
  color = texture2DArray(texture_unit, vec3(tex_coord, face));
}