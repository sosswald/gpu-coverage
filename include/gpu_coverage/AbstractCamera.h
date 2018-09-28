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

#ifndef INCLUDE_ARTICULATION_ABSTRACTCAMERA_H_
#define INCLUDE_ARTICULATION_ABSTRACTCAMERA_H_

#include <gpu_coverage/Programs.h>
#include <gpu_coverage/Node.h>
#include <cstdio>
#include <glm/detail/type_mat4x4.hpp>
#include <vector>
#include <string>

namespace gpu_coverage {

/**
 * @brief Abstract superclass for all cameras.
 *
 * See CameraOrtho, CameraPerspective, and PanoramaCamera subclasses.
 */
class AbstractCamera {
public:
    /**
     * @brief Constructor.
     * @param[in] id The unique ID of this camera.
     * @param[in] name The name of this camera for logging.
     * @param[in] node The scene graph node to attach this camera to.
     */
    AbstractCamera(const size_t id, const std::string& name, Node * const node);
    /**
     * @brief Destructor.
     */
    virtual ~AbstractCamera();

    /**
     * @brief Forward the view and projection matrices to the given locations of a GLSL shader and additionally return the view matrices.
     * @param[in]  locationsMVP Struct with variable locations of a GLSL shader.
     * @param[out] view Will be set to the view matrices.
     *
     * The view output vector will be resized and filled by the method.
     *
     * This method assumes that there is only one view matrix.
     */
    virtual void setViewProjection(const LocationsMVP& locationsMVP, std::vector<glm::mat4>& view) const;

    /**
     * @brief Write Graphviz %Dot node representing this camera to file for debugging.
     * @param[in] dot Output %Dot file.
     */
    virtual void toDot(FILE *dot) const = 0;

    /**
     * @brief Returns the 4x4 projection matrix of the camera.
     * @return Projection matrix.
     */
    inline const glm::mat4x4& getProjectionMatrix() const {
        return projectionMatrix;
    }

    /**
     * @brief Returns the unique ID of this camera.
     * @return ID.
     */
    inline const size_t& getId() const {
        return id;
    }

    /**
     * @brief Returns the name of this camera for logging.
     * @return Name of this camera.
     *
     * If available, the name included in the input file read by Assimp is used,
     * otherwise a generic string is constructed.
     */
    inline const std::string& getName() const {
        return name;
    }

    /**
     * @brief Returns the scene graph node that this camera is attached to.
     * @return The scene graph node.
     *
     * The node has to be passed to the constructor.
     */
    inline Node * getNode() const {
        return node;
    }

protected:
    const size_t id;                 ///< Unique ID of the camera, see getId().
    const std::string name;          ///< Name of the camera for logging, see getName().
    Node * const node;               ///< Scene graph node assigned to this camera, see getNode().
    glm::mat4 projectionMatrix;
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_ABSTRACTCAMERA_H_ */
