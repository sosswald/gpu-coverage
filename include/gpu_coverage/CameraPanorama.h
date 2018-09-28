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

#ifndef INCLUDE_ARTICULATION_CAMERAPANORAMA_H_
#define INCLUDE_ARTICULATION_CAMERAPANORAMA_H_

#include <gpu_coverage/CameraPerspective.h>

namespace gpu_coverage {

/**
 * @brief Omnidirectional panorama camera.
 *
 * This class combines six perspective cameras to an omnidirectional
 * camera in a cube map setup.
 */
class CameraPanorama: public CameraPerspective {
public:
    /**
     * @brief Cube map side.
     *
     * The order of the sides must be the order that glTexStorage2D() expects.
     */
    enum Direction {
        RIGHT = 0,         ///< GL_TEXTURE_CUBE_MAP_POSITIVE_X
        LEFT = 1,          ///< GL_TEXTURE_CUBE_MAP_NEGATIVE_X
        TOP = 2,           ///< GL_TEXTURE_CUBE_MAP_POSITIVE_Y
        BOTTOM = 3,        ///< GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
        BACK = 4,          ///< GL_TEXTURE_CUBE_MAP_POSITIVE_Z
        FRONT = 5          ///< GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    /**
     * @brief Constructor
     * @param[in] id Unique ID of the camera.
     * @param[in] node Scene graph node where to attach this camera.
     */
    CameraPanorama(const size_t& id, Node * const node);

    /**
     * @brief Destructor.
     */
    virtual ~CameraPanorama();

    /**
     * @brief Forward the view and projection matrices to the given locations of a GLSL shader and additionally return the view matrices.
     * @param[in]  locationsMVP Struct with variable locations of a GLSL shader.
     * @param[out] view Will be set to the view matrices.
     *
     * The view output vector will be resized and filled by the method.
     *
     * This method uses 6 view matrices, one for each side of a cube map.
     */
    virtual void setViewProjection(const LocationsMVP& locationsMVP, std::vector<glm::mat4>& view) const;

};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_CAMERAPANORAMA_H_ */
