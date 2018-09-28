/**
 * @brief Fragment shader for dijkstra-step.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::dijkstra_step
 * @class FragmentShader
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

const int COST_FACTOR = 40;

void main() {
    int cost = texture(costmap_texture_unit, tex_coord).r;
    int inputDist = texture(input_texture_unit, tex_coord).r;
    if (cost > 100) {
        // current texel is obstacle or in inscribed radius: pass through
        idist = cost;
    } else {
        // free space: check neighbors for change
        int oldDist = abs(inputDist);
        int dist = oldDist;
        bool setDist = false;
        for (int i = 0; i < 8; ++i) {
            int otherDist = texture(input_texture_unit, neighbors[i]).r;
            if (otherDist < 0) {
                // neighbor cell has changed
                int newDist = -otherDist + (i < 4 ? 141 : 100) + COST_FACTOR * cost;
                if (newDist < dist) {
                    dist = newDist;
                    setDist = true;
                }
            }
        } 
        if (setDist) {
            // distance changed: set new distance and mark as changed (= negative value)
            idist = -dist;
            atomicCounterIncrement(pixel_counter);
        } else if (inputDist < 0) {
            // value changed in previous iteration, but not in current iteration:
            // remove "changed" marker
            idist = -inputDist;
        } else {
            // neighbors did not change: pass through
            idist = inputDist;
        }
    }
} 
