/**
 * @brief Fragment shader for occupancy-map.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::occupancy_map
 * @class FragmentShader
 */

#version 440

out vec4 frag_color;

void main() {
  frag_color = vec4(0.0, 0.0, 0.0, 1.0);
}