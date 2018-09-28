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

#ifndef INCLUDE_ARTICULATION_BONE_H_
#define INCLUDE_ARTICULATION_BONE_H_

#include <assimp/scene.h>
#include <glm/detail/type_mat4x4.hpp>

namespace gpu_coverage {

// Forward declarations
class Node;

/**
 * @brief Represents a bone of an animation skeleton.
 *
 * This class is our equivalent of Assimp's aiBone class.
 */
class Bone {
public:
    /**
     * @brief Constructor.
     * @param[in] bone The Assimp aiBone for creating this bone.
     * @param[in] meshId The ID of the mesh that this bone is attached to.
     * @param[in] id The unique ID of this bone.
     */
    Bone(const aiBone * const bone, const size_t meshId, const size_t id);

    /**
     * @brief Destructor.
     */
    virtual ~Bone();

    /**
     * @brief Write Graphviz %Dot node representing this bone to file for debugging.
     * @param[in] dot Output %Dot file.
     */
    void toDot(FILE *dot);

    /**
     * Returns the unique ID of this bone.
     * @return ID.
     */
    inline const size_t& getId() const {
        return id;
    }

    /**
     * Returns the ID of the mesh that this bone is attached to.
     * @return Mesh ID.
     */
    inline const size_t& getMeshId() const {
        return meshId;
    }

    /**
     * Returns the name of this bone for logging.
     * @return Name of this bone.
     *
     * If available, the name included in the input file read by Assimp is used,
     * otherwise a generic string is constructed.
     */
    inline const std::string& getName() const {
        return name;
    }

    /**
     * Returns the offset matrix.
     * @return Offset matrix.
     *
     * The offset matrix transforms from mesh space to bone space in bind pose
     * (see Assimp aiBone documentation).
     */
    inline const glm::mat4& getOffsetMatrix() {
        return offsetMatrix;
    }

    /**
     * Assigns this bone to a scene graph node.
     * @param[in] node Scene graph node.
     */
    inline void setNode(Node * node) {
        this->node = node;
    }

    /**
     * Returns the scene graph node that this bone is attached to.
     * @return The scene graph node.
     *
     * Initially the node is set to NULL, can be set with setNode(Node *node).
     */
    inline Node * getNode() const {
        return this->node;
    }

protected:
    const size_t id;                  ///< Unique ID of the bone, see getId().
    const size_t meshId;              ///< ID of the mesh that this bone is attached to, see getMeshId().
    const std::string name;           ///< Name of the bone for logging, see getName().
    const glm::mat4 offsetMatrix;     ///< Offset matrix, see getOffsetMatrix().
    Node * node;                      ///< Scene graph node assigned to this bone, see getNode() and setNode().
};

}  // namespace gpu_coverage

#endif /* INCLUDE_ARTICULATION_BONE_H_ */
