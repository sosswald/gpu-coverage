/**
 * @brief FragmentFastforward shader for dijkstra-init.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::dijkstra_init
 * @class FragmentFastforwardShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION shader_bit_encoding
// EXTENSION shading_language_packing

out int dist;
uniform float resolution;
uniform isampler2D costmap_texture_unit;
uniform ivec2 robot_pixel;

const int INIT_DIST = 100000;
const int ZERO_DIST = 1;
const int COST_FACTOR = 100;

void main() {  
  ivec2 xy = ivec2(gl_FragCoord.xy);    
  bool diagonal = abs(xy.x - robot_pixel.x) == abs(xy.y - robot_pixel.y);
  if (xy == robot_pixel) {
    dist = -ZERO_DIST;
  } else if (diagonal || xy.x == robot_pixel.x || xy.y == robot_pixel.y) {
    // Same row / column / diagonal line -> check if line of sight is free
    ivec2 delta = ivec2(sign(robot_pixel.x - xy.x), sign(robot_pixel.y - xy.y));
    int stepCost = diagonal ? 141 : 100;
    bool free = true;
    int costSum = ZERO_DIST;
    int i = 0;
    while (xy != robot_pixel && i < 2 * int(resolution)) {
      int cost = texture(costmap_texture_unit, vec2(xy) / resolution).r;
      if (cost > 1000) {
        // obstacle
        free = false;
        break;
      } else {
        costSum += stepCost + COST_FACTOR * cost;
      }
      xy += delta;
      ++i;
    }
    if (free) {
      dist = -costSum;
    } else {
      dist = INIT_DIST;
    }
  } else {
      dist = INIT_DIST;
  }
}
