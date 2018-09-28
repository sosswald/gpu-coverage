/**
 * @brief Geometry shader for pano.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pano
 * @class GeometryShader
 */

#version 440
// EXTENSION shading_language_420pack

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 model_matrix;
uniform mat4 view_matrix[6];
uniform mat4 projection_matrix;
uniform mat3 normal_matrix[6];

struct LightInfo {
  vec4 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};


struct MaterialInfo {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};

uniform LightInfo light;
uniform MaterialInfo material;

in vec3 vertex_normal[];
in vec3 vertex_color[];
in vec2 vertex_texcoord[];
out vec2 tex_coord;
out float diffuse_factor;
out float specular_factor;

void main() {
    for (int layer = 0; layer < 6; ++layer) {        
        for (int i = 0; i < gl_in.length(); ++i) {
            gl_Layer = layer;
            mat4 mv = view_matrix[layer] * model_matrix;
            mat4 mvp = projection_matrix * mv;            
            vec3 tnorm = normalize(normal_matrix[layer] * vertex_normal[i]);
            vec4 eye_coords = mv * gl_in[i].gl_Position;
            vec3 s = normalize(vec3(view_matrix[layer] * light.position - eye_coords));
            diffuse_factor = max(dot(s, tnorm), 0.0);
            bool isFacing = diffuse_factor > 0.0;
            if (isFacing) {
              vec3 v = normalize(-eye_coords.xyz);
              vec3 r = reflect(-s, tnorm);
              specular_factor = pow(max(dot(r,v), 0.0), material.shininess);
            } else {
              specular_factor = 0.;
            } 
            tex_coord = vertex_texcoord[i];
            gl_Position = mvp * gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
