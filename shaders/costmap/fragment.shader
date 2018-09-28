/**
 * @brief Fragment shader for costmap.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::costmap
 * @class FragmentShader
 */

#version 440
// EXTENSION shading_language_420pack

uniform float resolution;           ///< Size of a map cell
uniform sampler2D texture_unit; 
in vec2 tex_coord;
out int dist;

const float INSCRIBED = 100000.f;
//const float OBSTACLE = 200000;  // = INSCRIBED added twice
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

float distFunction(const float cellDistance) {
    float radius = cellDistance * resolution;
    float decaying = max((exp(-weight*(radius-inscribed_radius)/(cutoff-inscribed_radius))-exp(-weight))/(1.-exp(-weight)), 0.0f);
    float isCollision = step(radius, 0.5f); 
    float isInscribed = step(radius, inscribed_radius);
    return INSCRIBED * (isCollision + isInscribed) + decaying * (1.f - isInscribed);
}

void main() {
    vec4 cell = texture(texture_unit, tex_coord);
    int cellHasSeed = int(hasSeed(cell));
    float cellDistance = computeDistance(tex_coord, cell);
    dist = cellHasSeed * int(distFunction(cellDistance) * 100.f) + (1 - cellHasSeed) * int(INSCRIBED * 100.f);
}