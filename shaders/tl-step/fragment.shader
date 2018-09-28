/**
 * @brief Fragment shader for tl-step.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::tl_step
 * @class FragmentShader
 */

#version 440
// EXTENSION shader_atomic_counters
// EXTENSION shading_language_420pack

uniform isampler2D texture_unit;
in vec2 tex_coord;
in vec2 neighbors[3];
out ivec2 value;
layout (binding = 2, offset = 0) uniform atomic_uint pixel_counter;

int isReachable(const vec4 color) {
    return int(step(color.a, 0.6f))   // MSB of alpha is 0 for costmap and 1 for target texture
         * (1 - int(step(dot(step(color, vec4(0.)), vec4(1.)), 0.5)));    // any component > 0 --> not obstacle
}

void main() {
    value          = texture(texture_unit, tex_coord).xy;    
    ivec2 left     = texture(texture_unit, neighbors[0]).xy;
    ivec2 top      = texture(texture_unit, neighbors[1]).xy;
    ivec2 bottom   = texture(texture_unit, neighbors[2]).xy;
         
    ivec2 val_zero = ivec2(step(vec2(value), vec2(0.f, 0.f)));             // value.x == 0, value.y == 0
    int setl = val_zero.x * int(step(1.f, float(left.x)));                 // value.x == 0 && left.x >= 1
    int sett = val_zero.y * int(step(1.f, float(top.y)));                  // value.y == 0 && top.y > = 1
    int setb = val_zero.y * int(step(1.f, float(bottom.y))) * (1 - sett);  // value.y == 0 && bottom.y >= 1 && !sett
    value += ivec2((left.x   - (value.x << 1)) * setl,
                   (top.y    - (value.y << 1)) * sett + 
                   (bottom.y - (value.y << 1)) * setb);
    if ((setl | sett | setb) > 0) {
      atomicCounterIncrement(pixel_counter);
    }
}
