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

#include <gpu_coverage/CameraPerspective.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/Node.h>
#include <gpu_coverage/Utilities.h>
#include <cstdio>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/gtc/matrix_transform.hpp>

namespace gpu_coverage {

CameraPerspective::CameraPerspective(const aiCamera * const camera, size_t id, Node * const node)
        : AbstractCamera(id, camera->mName.C_Str(), node),
                horFOV(camera->mHorizontalFOV), aspect(camera->mAspect), clipNear(camera->mClipPlaneNear),
                clipFar(camera->mClipPlaneFar)
{
    projectionMatrix = glm::perspective(horFOV, aspect, clipNear, clipFar);
}

CameraPerspective::CameraPerspective(size_t id, const std::string& name, Node * const node, const float horFOV,
        const float aspect, const float clipNear, const float clipFar)
        : AbstractCamera(id, name, node),
                horFOV(horFOV), aspect(aspect), clipNear(clipNear), clipFar(clipFar)
{
    projectionMatrix = glm::perspective(horFOV, aspect, clipNear, clipFar);
}

CameraPerspective::~CameraPerspective() {
}

void CameraPerspective::toDot(FILE *dot) const {
    fprintf(dot,
            "  c%zu [label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\"><tr><td colspan=\"2\">%s</td></tr><tr><td>type</td><td>perspective</td></tr><tr><td>hor FOV</td><td>%.1f&#176;</td></tr><tr><td>aspect</td><td>%.2f</td></tr><tr><td>clip near</td><td>%.1f</td></tr><tr><td>clip far</td><td>%.1f</td></tr></table>>, shape=\"plaintext\"];\n",
            id, name.c_str(),
            glm::degrees(horFOV), aspect, clipNear, clipFar);
    if (node) {
        fprintf(dot, "  n%zu -> c%zu;\n", node->getId(), id);
    }
}

} /* namespace gpu_coverage */
