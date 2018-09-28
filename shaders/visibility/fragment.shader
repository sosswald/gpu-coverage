/**
 * @brief Fragment shader for visibility.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::visibility
 * @class FragmentShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION shader_image_load_store

layout(early_fragment_tests) in;
uniform sampler2D texture_unit;
uniform layout(binding=7, rgba8ui) writeonly uimage2D visibility_map;
uniform float resolution;

const uvec4 COLOR = uvec4(0U, 0xFFU, 0U, 0xFFU);

in vec2 tex_coord;
out vec4 frag_color;

void main() {
  if (gl_FrontFacing) {
    frag_color = vec4(texture(texture_unit, tex_coord).rgb, 1.);
    ivec2 center = ivec2(tex_coord.xy * resolution);
    for (int x = center.x - 3; x <= center.x + 3; ++x) {
      for (int y = center.y - 3; y <= center.y + 3; ++y) {
        if (abs(x) != 3 || abs(y) != 3) {
          imageStore(visibility_map, ivec2(x, y), COLOR);
        }
      }
    }
  } else {
    //discard;
    frag_color = vec4(0.f, 0.f, 1.f, 1.f);
  }
}
