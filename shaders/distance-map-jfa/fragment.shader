/**
 * @brief Fragment shader for distance_map_jfa.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::distance_map_jfa
 * @class FragmentShader
 */

#version 440
// EXTENSION shading_language_420pack

out vec4 frag_color;
uniform int step_size;
uniform float resolution;
uniform sampler2D texture_unit;

vec2 colorToCoord(const vec4 color) {
  uvec4 d = uvec4(color * 255.f + 0.5f);
  return vec2((d.x << 8) + d.y, (d.z << 8) + d.w) / 65279.f;
}

bool hasSeed(vec4 cell) {
  return cell.z < 0.997f;
}

float computeDistance(const vec2 coord, const vec4 otherCell) {
  vec2 otherCoord = colorToCoord(otherCell);
  float dx = coord.x - otherCoord.x;
  float dy = coord.y - otherCoord.y;
  return sqrt(dx * dx + dy * dy);
}

void main() {
  vec2 coord = gl_FragCoord.xy / resolution;
  float delta = float(step_size) / resolution;
  
  vec4 cell = texture(texture_unit, coord);
  bool cellHasSeed = hasSeed(cell);
  float cellDistance = 0.0;
  if (cellHasSeed) {
    cellDistance = computeDistance(coord, cell);
  }
     
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      if (x == 0 && y == 0) {
        continue;
      }
      vec2 offsetCoord = coord + vec2(x, y) * delta;
      if (offsetCoord.x < -1. || offsetCoord.x > 1. || offsetCoord.y < -1. || offsetCoord.y > 1.) {
        continue;
      }
      // coord and offsetCoord are in [0, 1]
       
      vec4 otherCell = texture(texture_unit, offsetCoord);
      bool otherHasSeed = hasSeed(otherCell);
      if (!otherHasSeed) {
        // other cell is blank, does not contribute
        continue;
      }
      float otherDistance = computeDistance(coord, otherCell);
      if (!cellHasSeed || cellDistance > otherDistance) {
        // copy other cell
        cell = otherCell;
        cellHasSeed = true;
        cellDistance = otherDistance;
      }      
    }
  }   
  frag_color = cell;
}
