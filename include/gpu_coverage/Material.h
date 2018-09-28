/*
 * Copyright (c) 2018, Stefan Osswald
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef INCLUDE_ARTICULATION_MATERIAL_H_
#define INCLUDE_ARTICULATION_MATERIAL_H_

#include <gpu_coverage/Texture.h>
#include <assimp/scene.h>
#include <vector>

namespace gpu_coverage {

/**
 * @brief Represents a mesh material corresponding to Assimp's aiMaterial.
 */
class Material {
public:
    /**
     * @brief Constructor
     * @param[in] material Assimp material structure.
     * @param[in] id Unique ID of this material.
     * @param[in] dir Directory for loading texture images.
     */
    Material(const aiMaterial * const material, size_t id, const std::string& dir);
    /**
     * @brief Destructor.
     */
    virtual ~Material();

    /**
     * @brief Returns the ambient material color.
     * @return Ambient material color.
     */
    inline const float * getAmbient() const {
        return ambient;
    }

    /**
     * @brief Returns the diffuse material color.
     * @return Diffuse material color.
     */
    inline const float * getDiffuse() const {
        return diffuse;
    }

    /**
     * @brief Returns the specular material color.
     * @return Specular material color.
     */
    inline const float * getSpecular() const {
        return specular;
    }

    /**
     * @brief Returns the material's shininess coefficient.
     * @return Shininess coefficient.
     */
    inline float getShininess() const {
        return shininess;
    }

    /**
     * @brief Returns the name of this material for logging.
     * @return Name of this material.
     *
     * If available, the name included in the input file read by Assimp is used.
     */
    inline const std::string& getName() const {
        return name;
    }

    /**
     * @brief Returns true if a diffuse texture is assigned to the material.
     * @return True if diffuse texture is assigned.
     */
    inline bool hasTexture() const {
        return diffuseTexture != NULL;
    }

    /**
     * @brief Returns the diffuse texture if present, NULL otherwise.
     * @return Diffuse texture.
     */
    inline const Texture * getTexture() const {
        return diffuseTexture;
    }

    /**
     * @brief Sets the texture.
     * @param[in] texture New texture.
     */
    inline void setTexture(const Texture * const texture) {
        if (allocatedDiffuseTexture) {
            delete diffuseTexture;
            allocatedDiffuseTexture = false;
        }
        diffuseTexture = texture;
    }

    /**
     * @brief Loads a supplement file mapping material names to texture images.
     * @param[in] filename File name.
     */
    static void loadMaterialsFile(const char * const filename);

    /**
     * @brief Returns the unique ID of this material.
     * @return ID.
     */
    const size_t& getId() const {
        return id;
    }

protected:
    const size_t id;                 ///< Unique ID, see getId().
    const std::string name;          ///< Name of this animation, see getName().
    float ambient[4];                ///< Ambient color RGBA
    float diffuse[4];                ///< Diffuse color RGBA
    float specular[4];               ///< Specular color RGBA
    float shininess;                 ///< Shininess factor
    const Texture * diffuseTexture;  ///< Diffuse texture if available, NULL otherwise
    bool allocatedDiffuseTexture;    ///< True if this instance has allocated the texture and is responsible for deleting it later
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_MATERIAL_H_ */
