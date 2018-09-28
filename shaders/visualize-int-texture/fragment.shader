/**
 * @brief Fragment shader for visualize_int_texture.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::visualize_int_texture
 * @class FragmentShader
 */

#version 440
// EXTENSION shading_language_420pack

in vec2 tex_coord;
out vec4 frag_color;
uniform int min_dist;
uniform int max_dist;
uniform isampler2D input_texture_unit;

const vec4 OBSTACLE = vec4(0.5f, 0.5f, 0.5f, 1.f);
const vec4 INSCRIBED  = vec4(0.0f, 0.0f, 0.0f, 1.f);
// magenta -> blue -> cyan
//const vec4 LOW      = vec4(0.8f, 0.0f, 1.0f, 1.f);
//const vec4 MID      = vec4(0.0f, 0.0f, 1.0f, 1.f);
//const vec4 HIGH     = vec4(0.0f, 1.0f, 1.0f, 1.f);

// cyan -> blue -> magenta
const vec4 LOW      = vec4(0.0f, 1.0f, 1.0f, 1.f);
const vec4 MID      = vec4(0.0f, 0.0f, 1.0f, 1.f);
const vec4 HIGH     = vec4(0.8f, 0.0f, 1.0f, 1.f);


// green -> yellow -> red
//const vec4 LOW      = vec4(0.0f, 1.0f, 0.0f, 1.f);
//const vec4 MID      = vec4(1.0f, 1.0f, 0.0f, 1.f);
//const vec4 HIGH     = vec4(1.0f, 0.0f, 0.0f, 1.f);

void main() {
	// color scheme from former dijkstra map shader
    int dist = texture(input_texture_unit, tex_coord).r;
    float value = clamp(float(dist - min_dist) / float(max_dist - min_dist), 0.0f, 1.0f);
    float isObstacle = step(15000000.f, float(dist));
    float isInscribed = step(5000000.f, float(dist)) * (1.f - isObstacle);
    float isHigher = step(0.5f, value);
    frag_color = mix(mix(mix(
                         mix(LOW, MID, value * 2.), mix(MID, HIGH, (value - 0.5) * 2.), isHigher), 
                     OBSTACLE, isObstacle),
                     INSCRIBED, isInscribed);
} 
