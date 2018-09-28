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

#include <gpu_coverage/CoordinateAxes.h>
#include <gpu_coverage/Mesh.h>

#include GLEXT_INCLUDE
#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#include <glm/gtc/type_ptr.hpp>

namespace gpu_coverage {

CoordinateAxes::CoordinateAxes()
        : modelMatrix(glm::mat4()) {
    const float points[] = {
            -1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, 1.0f
    };
    const float colors[] = {
            1.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f
    };

    const unsigned int indices[] = { 0, 1, 2, 3, 4, 5 };

    glGenBuffers(3, vbo);
    enum {
        POINTS = 0, COLORS = 1, INDICES = 2
    };

    vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[POINTS]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    glVertexAttribPointer(Mesh::VERTEX_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(Mesh::VERTEX_BUFFER);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[COLORS]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(Mesh::COLOR_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(Mesh::COLOR_BUFFER);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDICES]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vbo[INDICES]), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(Mesh::INDEX_BUFFER, 1, GL_UNSIGNED_INT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(Mesh::INDEX_BUFFER);
}

CoordinateAxes::~CoordinateAxes() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(3, vbo);
}

void CoordinateAxes::display() const {
    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, 6);
}

} /* namespace gpu_coverage */
