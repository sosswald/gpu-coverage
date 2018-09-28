/**
 * @brief Fragment shader for visual-vertexcolor.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::visual_vertexcolor
 * @class FragmentShader
 */

#version 440
in vec3 vcolor;
out vec4 frag_color;

void main() {
  frag_color = vec4(vcolor, 1.0);
}