/**
 * @brief Fragment shader for costmap-index.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::costmap_index
 * @class FragmentShader
 */

#version 440
uniform isampler2D texture_unit;
uniform int width;
uniform int height;
in vec2 tex_coord;

out vec4 color;

void main() {
    ivec2 i = ivec2(int(tex_coord.x * float(width)), int(tex_coord.y * float(height)));
    ivec4 c = ivec4(
         (i.x & 0x7F00) >> 8,     // red:   x MSB
         i.x & 0xFF,              // green: x LSB
         i.y & 0xFF,              // blue:  y LSB
         (i.y & 0x7F00) >> 8);    // alpha: y MSB
    float cost = float(texture(texture_unit, tex_coord).r);
    color = vec4(c) / 255. * step(cost, 99900.);
}