/**
 * @brief Fragment shader for costmap-visual.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::costmap_visual
 * @class FragmentShader
 */

#version 440
// EXTENSION shading_language_420pack

uniform float resolution;
uniform sampler2D texture_unit;
in vec2 tex_coord;
out vec4 frag_color;

const float INFINITY = 1e8;
const float weight = 2.f;
const float inscribed_radius = 8.f;
const float cutoff = 25.f;


vec2 colorToCoord(const vec4 color) {
    uvec4 d = uvec4(color * 255.f + 0.5f);
    return vec2((d.x << 8) + d.y, (d.z << 8) + d.w) / 65279.f;
}

float hasSeed(vec4 cell) {
    return step(cell.z, 0.997f);
}

float computeDistance(const vec2 coord, const vec4 otherCell) {
    vec2 otherCoord = colorToCoord(otherCell);
    float dx = coord.x - otherCoord.x;
    float dy = coord.y - otherCoord.y;
    return sqrt(dx * dx + dy * dy);
}

vec4 costFunction(const float cellDistance) {
    float r = cellDistance * resolution;
    float inCollision = step(r, 0.5f);
    float inscribed = (1. - inCollision) * step(r, inscribed_radius);
    float decay = (1. - inCollision) * (1. - inscribed);
    float d = max((exp(-weight*(r-inscribed_radius)/(cutoff-inscribed_radius))-exp(-weight))/(1.-exp(-weight)), 0.0f);
    return vec4(0.f, 0.f, 0.f, 1.f)   * inCollision
         + vec4(0.f, 0.3f, 0.3f, 1.f) * inscribed
         + vec4(1.f, 1.f - d, 0.f, 1.f) * decay;
        // in collision -> black
        // safety margin -> cyan
        // exponential decay: red -> yellow                     
}

void main() {
    vec4 cell = texture(texture_unit, tex_coord);
    float cellHasSeed = hasSeed(cell);
    float cellDistance = computeDistance(tex_coord, cell);
    frag_color = cellHasSeed * costFunction(cellDistance)
               + (1. - cellHasSeed) * vec4(1., 0., 1., 1.);
}