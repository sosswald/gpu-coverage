/**
 * @brief Fragment shader for tl-edge.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::tl_edge
 * @class FragmentShader
 */

#version 440
uniform sampler2D texture_unit;
uniform float width;
uniform float height;
in vec2 tex_coord;
in vec2 neighbors[3];
out ivec2 value;

void main() {
    int isTarget = int(step(0.5f, texture(texture_unit, tex_coord).r));
    int neighbor0 = int(step(texture(texture_unit, neighbors[0]).r, 0.01f));
    int neighbor1 = int(step(0.5f, step(texture(texture_unit, neighbors[1]).r, 0.01f) + step(texture(texture_unit, neighbors[2]).r, 0.01f)));
    value = ivec2(neighbor0 * 2 - 1, neighbor1 * 2 - 1) * isTarget;
}
