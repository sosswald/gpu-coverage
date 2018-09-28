/**
 * @brief Fragment shader for show-texture.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::show_texture
 * @class FragmentShader
 */

#version 440
uniform sampler2D texture_unit;
in vec2 tex_coord;
out vec4 frag_color;

void main() {
    frag_color = texture(texture_unit, tex_coord);
}
