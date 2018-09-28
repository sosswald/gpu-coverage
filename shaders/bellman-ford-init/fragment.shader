/**
 * @brief Fragment shader for dijkstra-init.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::dijkstra_init
 * @class FragmentShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION shader_bit_encoding
// EXTENSION shading_language_packing

out int dist;
uniform ivec2 robot_pixel;

const int INIT_DIST = 100000;
const int ZERO_DIST = 1;

void main() {  
  ivec2 a = ivec2(step(vec2(robot_pixel) - vec2(0.5, 0.5), gl_FragCoord.xy));
  ivec2 b = ivec2(step(gl_FragCoord.xy, vec2(robot_pixel) + vec2(0.5, 0.5)));
  int set = a.x * a.y * b.x * b.y;
  dist = INIT_DIST * (1 - set) - ZERO_DIST * set;
}
