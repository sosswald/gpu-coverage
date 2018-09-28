/**
 * @brief Fragment shader for cylindrical.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::cylindrical
 * @class FragmentShader
 */

#version 440
in vec2 latY;
out vec4 color;
uniform samplerCube texture_unit;

void main() {
    float cosPhi = cos(latY.x);
    float sinPhi = sin(latY.x);
    float y = latY.y;
    color = texture(texture_unit, vec3(-sinPhi, -y, -cosPhi));
}
