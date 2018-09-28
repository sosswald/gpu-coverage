/**
 * @brief Vertex shader for rendering a textured mesh.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::visual_texture
 * @class VertexShader
 */

#version 440
// EXTENSION shading_language_420pack
// EXTENSION explicit_attrib_location

uniform mat4 model_matrix;                    ///< Model matrix.
uniform mat4 view_matrix;                     ///< View matrix.
uniform mat4 projection_matrix;               ///< Projection matrix.
uniform mat3 normal_matrix;                   ///< Normal matrix = transposed inverse of model view matrix

/**
 * @brief Properties of a point light source
 */
struct LightInfo {
  vec4 position;        ///< Position of the light source.
  vec3 ambient;         ///< Ambient color of the light source.
  vec3 diffuse;         ///< Diffuse color of the light source.
  vec3 specular;        ///< Specular color of the light source.
};

/**
 * @brief Properties of a material
 */
struct MaterialInfo {
  vec3 ambient;         ///< Ambient color of the material.
  vec3 diffuse;         ///< Diffuse color of the material.
  vec3 specular;        ///< Specular color of the material.
  float shininess;      ///< Shininess factor of the material.
};

uniform LightInfo light;            ///< Point light source.
uniform MaterialInfo material;      ///< %Material of the object.

layout(location = 0) in vec3 vertex_position;     ///< Vertex buffer with per vertex 3D positions
layout(location = 2) in vec2 vertex_texcoord;     ///< Vertex buffer with per vertex 2D texture coordinates
layout(location = 3) in vec3 vertex_normal;       ///< Vertex buffer with per vertex 3D normals

out vec2 tex_coord;                   ///< Output texture coordinate of the vertex
out float diffuse_factor;             ///< Output diffuse factor of the vertex
out float specular_factor;            ///< Output specular factor of the vertex

void main() {
    mat4 mv = view_matrix * model_matrix;
    mat4 mvp = projection_matrix * mv;
    vec3 tnorm = normalize(normal_matrix * vertex_normal);
    vec4 eye_coords = mv * vec4(vertex_position, 1.0);
    vec3 s = normalize(vec3(view_matrix * light.position - eye_coords));
    diffuse_factor = max(dot(s, tnorm), 0.0);
    bool isFacing = diffuse_factor > 0.0;
    if (isFacing) {
      vec3 v = normalize(-eye_coords.xyz);
      vec3 r = reflect(-s, tnorm);
      specular_factor = pow(max(dot(r,v), 0.0), material.shininess);
    } else {
      specular_factor = 0.;
    } 
    gl_Position = mvp * vec4(vertex_position, 1.0);
    tex_coord = vertex_texcoord;
}