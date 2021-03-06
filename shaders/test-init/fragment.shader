/**
 * @brief Fragment shader for test-init.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::test_init
 * @class FragmentShader
 */

#version 440
// EXTENSION shader_atomic_counters
// EXTENSION shading_language_420pack

in vec3 position;
out uint dist;

void main() {
    discard;
}