/**
 * @brief GeometryAndroid shader for test-step.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::test_step
 * @class GeometryAndroidShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION shader_image_load_store

layout(points) in;
layout(points, max_vertices=8) out;
uniform layout(binding=4, r32i) readonly iimage2D costmap;
uniform layout(binding=5, r32i) coherent iimage2D map;
uniform layout(binding=6, r32i) coherent iimage2D queued;

uniform float resolution;

const int COST_FACTOR = 100;

out vec3 position;

const ivec3 DELTA[8] = ivec3[8](
  ivec3(-1, -1, 141),
  ivec3(-1,  0, 100),
  ivec3(-1,  1, 141),
  ivec3( 0, -1, 100),
  ivec3( 0,  1, 100),
  ivec3( 1, -1, 141),
  ivec3( 1,  0, 100),
  ivec3( 1,  1, 141)
);

bool insideBox(ivec2 point) {
    ivec2 BL = ivec2(0, 0);
    ivec2 TR = ivec2(int(resolution), int(resolution));
    bvec4 b = bvec4(greaterThan(point, BL), lessThan(point, TR));
    return all(b);
}

void main() {
    ivec2 tex_coord[1] = ivec2[1](ivec2((gl_in[0].gl_Position.xy + 1.f) / 2.f * resolution));
    imageStore(queued, tex_coord[0], ivec4(0, 0, 0, 0));
    int newCost = imageLoad(map, tex_coord[0]).r;
    for (int d = 0; d < 8; ++d) {
        ivec2 neighbor = tex_coord[0] + DELTA[d].xy;
        if (insideBox(neighbor)) {
            int cost = imageLoad(costmap, neighbor).r;
            if (cost < 100000) {
                int neighborNewCost = newCost + DELTA[d].z + cost * COST_FACTOR;
                int neighborOldCost = imageLoad(map, neighbor).r;
                
                // neighbor changed -> queue            
                if (neighborNewCost <= neighborOldCost) {
                    imageStore(map, neighbor, ivec4(neighborNewCost, 0, 0, 0));
                    int alreadyQueued = imageLoad(queued, neighbor).r;
                    if (alreadyQueued == 0) {
                        imageStore(queued, neighbor, ivec4(1, 0, 0, 0));
                        position = vec3(gl_in[0].gl_Position.xy + vec2(DELTA[d].xy) * (2. / resolution), 0.);
                        gl_Position = vec4(position, 1.);
                        EmitVertex();
                        EndPrimitive();
                    }
                }
            }
        }
    }
}
