/**
 * @brief Fragment shader for equirectangular.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::equirectangular
 * @class FragmentShader
 */

#version 440
in vec2 latLon;
out vec4 color;
uniform samplerCube texture_unit;

void main() {
    vec2 c = cos(latLon), s = sin(latLon);
    color = texture(texture_unit, vec3(-s.y * s.x, c.y, -c.x * s.y));
}