/**
 * @brief Fragment shader for rendering a textured mesh.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::visual_texture
 * @class FragmentShader
 */

#version 440
uniform bool has_texture;          ///< true if object has texture, false if material should be used instead.
uniform sampler2D texture_unit;    ///< %Texture unit for sampling the object texture.
in vec2 tex_coord;                 ///< %Texture coordinate passed in from vertex shader.
in float diffuse_factor;           ///< Diffuse factor passed in from vertex shader.
in float specular_factor;          ///< Specular factor passed in from vertex shader.
out vec4 frag_color;               ///< Output fragment color.

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

void main() {
  // Get diffuse and ambient color either from texture or material
  vec3 diffuse_color;
  vec3 ambient_color;
  if (has_texture) {
    diffuse_color = vec3(texture(texture_unit, tex_coord));
    ambient_color = 0.3 * diffuse_color;  // Ambient color: darken diffuse color
  } else {
    diffuse_color = material.diffuse;
    ambient_color = material.ambient;
  }
  // Compute the fragment color as the dot product of the light source and material properties with corresponding coefficients
  // calculated in the vertex shader
  frag_color = vec4(light.ambient * ambient_color + light.diffuse * diffuse_color * diffuse_factor + light.specular * material.specular * specular_factor, 1.0);
}
