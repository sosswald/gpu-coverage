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

#include <gpu_coverage/CameraPanorama.h>
#include <gpu_coverage/Node.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/Utilities.h>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/detail/func_trigonometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/io.hpp>

namespace gpu_coverage {

static const glm::vec3 TARGET[6] = {
        glm::vec3(1.f, 0.f, 0.f),  // +X = right
        glm::vec3(-1.f, 0.f, 0.f),  // -X = left
        glm::vec3(0.f, -1.f, 0.f),  // -Y = bottom
        glm::vec3(0.f, 1.f, 0.f),  // +Y = top
        glm::vec3(0.f, 0.f, -1.f),  // -Z = front
        glm::vec3(0.f, 0.f, 1.f)   // +Z = back
        };

static const glm::vec3 UP[6] = {
        glm::vec3(0.f, 1.f, 0.f),  // +X = right
        glm::vec3(0.f, 1.f, 0.f),  // -X = left
        glm::vec3(0.f, 0.f, -1.f),  // -Y = bottom
        glm::vec3(0.f, 0.f, 1.f),  // +Y = top
        glm::vec3(0.f, 1.f, 0.f),  // -Z = front
        glm::vec3(0.f, 1.f, 0.f)   // +Z = back
        };

CameraPanorama::CameraPanorama(const size_t& id, Node * const node)
        : CameraPerspective(id, std::string("Pano") + node->getName(), node, glm::radians(90.f), 1.f, 0.001f, 10.0f)
{
}

CameraPanorama::~CameraPanorama() {
}

void CameraPanorama::setViewProjection(const LocationsMVP& locationsMVP, std::vector<glm::mat4>& view) const {
    view.resize(6);
    // Set camera and projection
    // view: inverse because we fix the camera in the origin and move the world in the opposite direction
    for (size_t i = 0; i < 6; ++i) {
        //view[i] = glm::inverse(node->getWorldTransform() * ROTATIONS[i]);
        const glm::vec3 pos = glm::vec3(glm::column(node->getWorldTransform(), 3));
        view[i] = glm::lookAt(pos, pos + TARGET[i], UP[i]);
        glUniformMatrix4fv(locationsMVP.viewMatrix[i], 1, GL_FALSE, glm::value_ptr(view[i]));
        glUniformMatrix4fv(locationsMVP.projectionMatrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    }
    checkGLError();
}

} /* namespace gpu_coverage */
