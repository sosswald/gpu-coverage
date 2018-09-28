/**
 * @brief Fragment shader for distance_map_seed.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::distance_map_seed
 * @class FragmentShader
 */

#version 440
// EXTENSION shading_language_420pack

out vec4 frag_color;
uniform float resolution;

vec4 coordToColor(const vec2 coord) {
  uvec2 c = uvec2(coord * 65279.f + 0.5f);
  return vec4(c.x >> 8, c.x & 0xFFU, c.y >> 8, c.y & 0xFFU) / 255.f;
}

void main() {  
  frag_color = coordToColor(vec2(gl_FragCoord.xy) / resolution);
}