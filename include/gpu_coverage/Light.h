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

#ifndef INCLUDE_ARTICULATION_LIGHT_H_
#define INCLUDE_ARTICULATION_LIGHT_H_

#include <assimp/scene.h>
#include <glm/detail/type_vec3.hpp>

namespace gpu_coverage {

// Forward declarations
class Node;

/**
 * @brief Represents a light source, corresponds to Assimp's aiLight structure.
 */
class Light {
public:
    /**
     * @brief Constructor.
     * @param[in] light Assimp light structure.
     * @param[in] id Unique ID of the light source-
     * @param[in] node Scene graph node where the light is attached.
     */
    Light(const aiLight * const light, const size_t id, Node * node);

    /**
     * @brief Destructor.
     */
    virtual ~Light();

    /**
     * @brief Write Graphviz %Dot node representing this light source to file for debugging.
     * @param[in] dot Output %Dot file.
     */
    void toDot(FILE *dot);

    /**
     * @brief Returns the unique ID of this light source.
     * @return ID.
     */
    size_t getId() const {
        return id;
    }

    /**
     * @brief Returns the name of this light source for logging.
     * @return Name of this light source.
     *
     * If available, the name included in the input file read by Assimp is used.
     */
    const std::string& getName() const {
        return name;
    }

    /**
     * @brief Returns the scene graph node that this bone is attached to.
     * @return The scene graph node.
     */
    Node* getNode() const {
        return node;
    }

    /**
     * @brief Returns the diffuse light color.
     * @return Diffuse light color.
     */
    const glm::vec3& getDiffuse() const {
        return diffuse;
    }

    /**
     * @brief Returns the ambient light color.
     * @return Ambient light color.
     */
    const glm::vec3& getAmbient() const {
        return ambient;
    }

    /**
     * @brief Returns the specular light color.
     * @return Specular light color.
     */
    const glm::vec3& getSpecular() const {
        return specular;
    }

    /**
     * @brief Returns the type of the light source.
     * @return Type of the light source.
     */
    aiLightSourceType getType() const {
        return type;
    }

protected:
    const size_t id;                ///< Unique ID, see getId().
    const std::string name;         ///< Name of this light source, see getName().
    Node * const node;              ///< Scene graph node where this light source is attached, see getNode().
    const aiLightSourceType type;   ///< Type of this light source, see getType()
    const glm::vec3 diffuse;        ///< Diffuse light color, see getDiffuse().
    const glm::vec3 ambient;        ///< Ambient light color, see getAmbient().
    const glm::vec3 specular;       ///< Specular light color, see getSpecular().
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_LIGHT_H_ */
