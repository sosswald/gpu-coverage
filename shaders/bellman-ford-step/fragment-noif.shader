/**
 * @brief FragmentNoif shader for dijkstra-step.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::dijkstra_step
 * @class FragmentNoifShader
 */

#version 440
// EXTENSION shader_atomic_counters
// EXTENSION shader_bit_encoding
// EXTENSION shading_language_420pack

in vec2 tex_coord;
in vec2 neighbors[8];
out int idist;

uniform isampler2D costmap_texture_unit;
uniform isampler2D input_texture_unit;
layout (binding = 2, offset = 0) uniform atomic_uint pixel_counter;

const int COST_FACTOR = 100;

void main() {
    int cost = texture(costmap_texture_unit, tex_coord).r;
    int inputDist = texture(input_texture_unit, tex_coord).r;
    int isObstacle = int(step(101., float(cost)));
    
    // free space: check neighbors for change
    int oldDist = abs(inputDist);
    int dist = oldDist;
    int setDist = 0;
    // diagonal neighbors
    for (int i = 0; i < 4; ++i) {
        int otherDist = texture(input_texture_unit, neighbors[i]).r;
        // neighbor cell has changed
        int otherChanged = int(step(float(otherDist), -0.5f));
        int newDist = -otherDist + 141 + COST_FACTOR * cost;
        // path via changed neighbor is better
        int isBetter = otherChanged * int(step(float(newDist), float(dist)));
        dist = dist * (1 - isBetter) + newDist * isBetter;
        setDist |= isBetter;
    }
    // horizontal/vertical neihbors
    for (int i = 4; i < 8; ++i) {
        int otherDist = texture(input_texture_unit, neighbors[i]).r;
        int otherChanged = int(step(float(otherDist), -0.5f));
        int newDist = -otherDist + 100 + COST_FACTOR * cost;
        int isBetter = otherChanged * int(step(float(newDist), float(dist)));
        dist = dist * (1 - isBetter) + newDist * isBetter;
        setDist |= isBetter;
    } 
    int previousChanged = int(step(float(inputDist), -0.5));
    // distance changed: set new distance and mark as changed (= negative value)
        // value changed in previous iteration, but not in current iteration:
        // remove "changed" marker
        // neighbors did not change: pass through (whether or not is obstacle)
    idist = setDist * (-dist * (1 - isObstacle) + inputDist * isObstacle)
          + (1 - setDist) * (inputDist * (((isObstacle << 1) - 1) * previousChanged + (1 - previousChanged)));
    if (setDist == 1) {
        atomicCounterIncrement(pixel_counter);
    }
} 
