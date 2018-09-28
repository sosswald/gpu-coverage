/**
 * @brief VertexAndroid shader for test-step.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::test_step
 * @class VertexAndroidShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

layout(location = 0) in vec3 vertex_position;
uniform float resolution;

void main() {
    gl_Position = vec4(vertex_position, 1.0f);
}
