/**
 * @brief Fragment shader for pano-eval.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pano_eval
 * @class FragmentShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION shader_image_load_store

uniform sampler2D texture_unit;
uniform isampler2D integral;
uniform layout(binding=7, r32i) coherent iimage2D utility_map;
uniform float resolution;

in vec2 tex_coord;
out vec4 frag_color;

int isReachable(const vec4 color) {
    return int(step(color.a, 0.6f))   // MSB of alpha is 0 for costmap and 1 for target texture
         * (1 - int(step(dot(step(color, vec4(0.)), vec4(1.)), 0.5)));    // any component > 0 --> not obstacle
}

float isTarget(const vec4 color) {
    return step(0.999f, color.a) * step(0.999f, color.r);
}

ivec2 colorToCoordinate(const vec4 color) {
    // Unpack format defined in IndexImage
    ivec4 i = ivec4(color * 255.f);    
    return ivec2(i.x << 8 | i.y, i.a << 8 | i.z);                 
}

void main() {
    // For debugging integral image:
    //ivec2 it = texture(integral, tex_coord).xy;
    //frag_color = vec4(float(it.x) / 500., float(it.y) / 500., 0, 1.);
    //return;
    
    vec4 my_color = texture(texture_unit, tex_coord);    
    vec2 opposite_coord = vec2(tex_coord.x + 0.5 * (step(tex_coord.x, 0.5) * 2. - 1.), 1. - tex_coord.y);
    vec4 opposite_color = texture(texture_unit, opposite_coord);
    int set = isReachable(my_color); // * isTarget(opposite_color);
    // reachable robot pose and something to see     
    ivec2 center = colorToCoordinate(my_color);
    ivec2 i = texture(integral, opposite_coord).xy;
    int gain = i.x * set;
    //int gain = i.x * int(set);
    if (center.x >= 0 && center.x < 256 && center.y >= 0 && center.y < 256) {
   		imageAtomicMax(utility_map, center, gain);
   	}
    //frag_color = vec4(float(gain) / 400., 0, set, 1);
    frag_color = my_color;
}
