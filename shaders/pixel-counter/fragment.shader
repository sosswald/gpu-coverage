/**
 * @brief Fragment shader for pixel-counter.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pixel_counter
 * @class FragmentShader
 */

#version 440
// EXTENSION shader_atomic_counters
// EXTENSION shading_language_420pack

uniform sampler2D input_texture_unit;
layout (binding = 2, offset = 0) uniform atomic_uint pixel_counter;
in vec2 tex_coord;
out vec4 frag_color;
const vec4 COLOR = vec4(0.f, 0.f, 0.f, 1.f);

void main() {
  vec4 inputColor = texture(input_texture_unit, tex_coord);
  // is green?
  if (inputColor.y > 0.5) {
    atomicCounterIncrement(pixel_counter);
  }
  // have to set a color, otherwise this shader might get optimized out
  frag_color = COLOR;
}
