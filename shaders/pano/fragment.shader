/**
 * @brief Fragment shader for pano.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pano
 * @class FragmentShader
 */

#version 440
uniform bool has_texture;
uniform sampler2D texture_unit;
in vec2 tex_coord;
in float diffuse_factor;
in float specular_factor;
out vec4 frag_color;

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

void main() {
  vec3 diffuse_color;
  vec3 ambient_color;
  if (has_texture) {
    diffuse_color = vec3(texture(texture_unit, tex_coord));
    ambient_color = 0.3 * diffuse_color;
  } else {
      diffuse_color = material.diffuse;
      ambient_color = material.ambient;
  }
  frag_color = vec4(light.ambient * ambient_color + light.diffuse * diffuse_color * diffuse_factor + light.specular * material.specular * specular_factor, 1.0);
}
