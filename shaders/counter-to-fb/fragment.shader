/**
 * @brief Fragment shader for counter_to_fb.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::counter_to_fb
 * @class FragmentShader
 */

#version 440
// EXTENSION shader_atomic_counters
// EXTENSION shading_language_420pack

layout (binding = 2, offset = 0) uniform atomic_uint pixel_counter;
out uint count;

void main() {
  count = atomicCounter(pixel_counter);
}
