/**
 * @brief Fragment shader for utility1.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::utility1
 * @class FragmentShader
 */

#version 440
uniform isampler2D utility_unit;
uniform isampler2D gain1_unit;

in vec2 tex_coords[9];
out int utility;
const int GAIN_WEIGHT = 500;

int getMax(isampler2D texture_unit) {
    int vals[9] = int[9](
      texture(texture_unit, tex_coords[0]).r,
      texture(texture_unit, tex_coords[1]).r,
      texture(texture_unit, tex_coords[2]).r,
      texture(texture_unit, tex_coords[3]).r,
      texture(texture_unit, tex_coords[4]).r,
      texture(texture_unit, tex_coords[5]).r,
      texture(texture_unit, tex_coords[6]).r,
      texture(texture_unit, tex_coords[7]).r,
      texture(texture_unit, tex_coords[8]).r
    );
    int c = 0;
    for (int i = 0; i < 9; ++i) {
      c += int(step(1, vals[i]));
    }
    return int(step(3, c)) * max(
      max(
        max(
          max(vals[0], vals[1]),
          max(vals[2], vals[3])
        ),
        max(
          max(vals[4], vals[5]),
          max(vals[6], vals[7])
        )
      ),
      vals[8]
    );
}


void main() {
    utility = texture(utility_unit, tex_coords[4]).r - getMax(gain1_unit) * GAIN_WEIGHT;
    //utility = 12800 - getMax(gain1_unit) * GAIN_WEIGHT;
}
