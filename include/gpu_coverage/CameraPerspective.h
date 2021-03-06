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

#ifndef INCLUDE_ARTICULATION_CAMERAPERSPECTIVE_H_
#define INCLUDE_ARTICULATION_CAMERAPERSPECTIVE_H_

#include <gpu_coverage/AbstractCamera.h>
#include <assimp/scene.h>

namespace gpu_coverage {

class Node;
struct LocationsMVP;

/**
 * @brief Perspective projection camera.
 */
class CameraPerspective: public AbstractCamera {
public:
    /**
     * @brief Constructor from Assimp aiCamera.
     * @param[in] camera The Assimp aiCamera for creating this camera.
     * @param[in] id Unique ID of this camera.
     * @param[in] node Scene graph node where to attach this camera.
     */
    CameraPerspective(const aiCamera * const camera, size_t id, Node * const node);
    /**
     * @brief Constructor for manually creating a camera.
     * @param[in] id Unique ID of this camera.
     * @param[in] name Name of this camera for logging.
     * @param[in] node Scene graph node where to attach this camera.
     * @param[in] horFOV Horizontal field of view in radians.
     * @param[in] aspect Aspect ratio of the camera image.
     * @param[in] clipNear Distance of the near clipping plane.
     * @param[in] clipFar Distance of the far clipping plane.
     */
    CameraPerspective(size_t id, const std::string& name, Node * const node, const float horFOV, const float aspect,
            const float clipNear, const float clipFar);

    /**
     * @brief Destructor.
     */
    virtual ~CameraPerspective();

    /**
     * @brief Write Graphviz %Dot node representing this camera to file for debugging.
     * @param[in] dot Output %Dot file.
     */
    void toDot(FILE *dot) const;

protected:
    const float horFOV;        ///< Horizontal field of view in radians.
    const float aspect;        ///< Aspect ratio of the camera image.
    const float clipNear;      ///< Distance of the near clipping plane.
    const float clipFar;       ///< Distance of the far clipping plane.

};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_CAMERAPERSPECTIVE_H_ */
