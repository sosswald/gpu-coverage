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

#include <gpu_coverage/Bone.h>
#include <gpu_coverage/Utilities.h>
#include <cstdio>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/io.hpp>

namespace gpu_coverage {

Bone::Bone(const aiBone * const bone, const size_t meshId, const size_t id)
        : id(id), meshId(meshId), name(bone->mName.C_Str()), offsetMatrix(
                glm::transpose(glm::make_mat4x4(bone->mOffsetMatrix[0]))), node(NULL)
{
}

Bone::~Bone() {
}

void Bone::toDot(FILE *dot) {
    fprintf(dot, "  b%zu_%zu [label=\"{%s}\", shape=\"record\"];\n", meshId, id, name.c_str());
    const glm::vec3 position(glm::column(offsetMatrix, 3));
    const glm::vec3 scaling = glm::vec3(glm::length(glm::column(offsetMatrix, 0)),
            glm::length(glm::column(offsetMatrix, 1)), glm::length(glm::column(offsetMatrix, 2)));
    const glm::mat3 unscale = glm::mat3(
            glm::vec3(glm::column(offsetMatrix, 0)) / scaling[0],
            glm::vec3(glm::column(offsetMatrix, 1)) / scaling[1],
            glm::vec3(glm::column(offsetMatrix, 2)) / scaling[2]);
    const glm::quat rotation(unscale);
    fprintf(dot,
            "  b%zu_%zu [label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"1\"><tr><td colspan=\"2\">%s</td></tr><tr><td>offset position</td><td>%.2f, %.2f, %.2f</td></tr><tr><td>offset rotation</td><td>%.1f&#176;, %.1f&#176;, %.1f&#176;</td></tr><tr><td>offset scale</td><td>%.1f, %.1f, %.1f</td></tr></table>>, shape=\"plaintext\"];\n",
            meshId, id, name.c_str(),
            position.x, position.y, position.z,
            glm::degrees(glm::roll(rotation)), glm::degrees(glm::pitch(rotation)), glm::degrees(glm::yaw(rotation)),
            scaling.x, scaling.y, scaling.z
            );
    fprintf(dot, "  b%zu_%zu -> m%zu [style=\"dashed\"];\n", meshId, id, meshId);
}

}  // namespace gpu_coverage
