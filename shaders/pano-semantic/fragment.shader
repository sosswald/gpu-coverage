/**
 * @brief Fragment shader for pano-semantic.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pano_semantic
 * @class FragmentShader
 */

#version 440
uniform bool has_texture;
uniform sampler2D texture_unit;

in vec2 tex_coord;
out vec4 frag_color;

const vec4 OBSTACLE = vec4(0.f, 0.f, 0.f, 1.f);

void main() {
  vec3 output_color;
  if (has_texture) {
    frag_color = texture(texture_unit, tex_coord);
  } else {
    frag_color = OBSTACLE;
  }
}
