/**
 * @brief Fragment shader for distance-map.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::distance_map
 * @class FragmentShader
 */

#version 440

in vec2 tex_coord;
out vec4 frag_color;
uniform float resolution;
uniform sampler2D texture_unit;

vec2 colorToCoord(const vec4 color) {
  uvec4 d = uvec4(color * 255.f + 0.5f);
  return vec2((d.x << 8) + d.y, (d.z << 8) + d.w) / 65279.f;
}

float hasSeed(const vec4 cell) {
  return step(cell.z, 0.997f);
}

float computeDistance(const vec2 coord, const vec4 otherCell) {
  vec2 otherCoord = colorToCoord(otherCell);
  float dx = coord.x - otherCoord.x;
  float dy = coord.y - otherCoord.y;
  return sqrt(dx * dx + dy * dy);
}

void main() {
  vec4 cell = texture(texture_unit, tex_coord);
  bool cellHasSeed = hasSeed(cell);
  float cellDistance = computeDistance(tex_coord, cell);
  float isRobot = step(cellDistance, 1./resolution);
  return cellHasSeed * isRobot * vec4(1., 0., 0., 1.)
       + cellHasSeed * (1. - isRobot) * vec4(0., 0., min(0.2 + cellDistance, 1.), 1.);
       + (1. - cellHasSeed) * vec4(1., 0., 1., 1.);
}
