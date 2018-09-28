/**
 * @brief Fragment shader for visual-flat.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::visual_flat
 * @class FragmentShader
 */

#version 440

uniform vec4 color;
out vec4 frag_color;


void main() {
  frag_color = color;
}
