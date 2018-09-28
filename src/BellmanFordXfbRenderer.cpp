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

#include <gpu_coverage/BellmanFordXfbRenderer.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Config.h>
#include <fstream>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/string_cast.hpp>

namespace gpu_coverage {

#if __ANDROID__
#define IMMEDIATE_QUERY_READ 1
#endif

BellmanFordXfbRenderer::BellmanFordXfbRenderer(const Scene * const scene, const CostMapRenderer * const costmapRenderer,
        const bool renderToWindow, const bool visual)
        : AbstractRenderer(scene, "bellmanfordxfb"), costmapRenderer(costmapRenderer), renderToWindow(renderToWindow),
          renderVisual(visual || renderToWindow),
          progShowTexture(NULL), progVisualizeIntTexture(NULL),
          width(256), height(256), maxIterations(1024),
          numQueries(sizeof(numPrimitivesQuery) / sizeof(numPrimitivesQuery[0]))
{
    if (!progBellmanFordXfbInit.isReady() || !progBellmanFordXfbStep.isReady()) {
        return;
    }

    progBellmanFordXfbInit.use();
    glUniform1f(progBellmanFordXfbInit.locations.resolution, static_cast<float>(width));

    progBellmanFordXfbStep.use();
    glUniform1f(progBellmanFordXfbStep.locations.resolution, static_cast<float>(width));
    glUniform1i(progBellmanFordXfbStep.locations.textureUnit, 0);

    if(renderVisual) {
        progVisualizeIntTexture = new ProgramVisualizeIntTexture();
        if (!progVisualizeIntTexture->isReady()) {
            return;
        }
        checkGLError();
        progVisualizeIntTexture->use();
        glUniform1i(progVisualizeIntTexture->locations.textureUnit, 8);
        glUniform1i(progVisualizeIntTexture->locations.minDist, 0);
        glUniform1i(progVisualizeIntTexture->locations.maxDist, width * 100 * M_SQRT2);
    }
    if (renderToWindow) {
        progShowTexture = new ProgramShowTexture();
        if (!progShowTexture->isReady()) {
            return;
        }
        progShowTexture->use();
        glUniform1i(progShowTexture->locations.textureUnit, 8);
    }

    const size_t numTextures = sizeof(textures) / sizeof(textures[0]);
    glGenTextures(numTextures, textures);
    for (size_t i = 0; i < numTextures; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        switch (i) {
        case QUEUED:
        case OUTPUT:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32I, width, height);
            break;
        case VISUAL:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
            break;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glGenFramebuffers(1, &framebuffer);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLfloat vertices[] = {
            0.f,  0.f, 0.f,
           -1.f, -1.f, 0.f,   // QUADS
            1.f, -1.f, 0.f,
            1.f,  1.f, 0.f,
           -1.f,  1.f, 0.f,
           -1.f, -1.f, 0.f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glGenTransformFeedbacks(2, tfi);
    glGenBuffers(2, tbo);
    for (size_t i = 0; i < 2; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, tbo[i]);
        glBufferData(GL_ARRAY_BUFFER, 100000, NULL, GL_STREAM_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glGenQueries(numQueries, numPrimitivesQuery);
    glDisable(GL_DITHER);

    ready = true;
}

BellmanFordXfbRenderer::~BellmanFordXfbRenderer() {
    glDeleteBuffers(2, tbo);
    glDeleteBuffers(1, &vbo);
    glDeleteTransformFeedbacks(2, tfi);
    glDeleteTextures(sizeof(textures) / sizeof(textures[0]), textures);
    glDeleteVertexArrays(1, &vao);
    glDeleteQueries(numQueries, numPrimitivesQuery);
}

void BellmanFordXfbRenderer::display() {
    if (!ready) {
        return;
    }
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "bellmanfordxfbrenderer");
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    GLboolean oldDepthTest = glIsEnabled(GL_DEPTH_TEST);

    // Setup framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);

    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);

    // Clear output texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[OUTPUT], 0);
    const GLint large[4] = { 10000000, 0, 0, 0 };
    glClearBufferiv(GL_COLOR, 0, large);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[QUEUED], 0);
    const GLint zero[4] = { 0, 0, 0, 0 };
    glClearBufferiv(GL_COLOR, 0, zero);

    checkGLError();

    // Bind output texture to image unit
    glBindImageTexture(4, costmapRenderer->getTexture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32I);
    glBindImageTexture(5, textures[OUTPUT], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32I);
    glBindImageTexture(6, textures[QUEUED], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32I);

    int input = 0;
    int output = 1;

    // =========== INIT ===========

    progBellmanFordXfbInit.use();
    const glm::mat4 mvp = costmapRenderer->getCamera()->getProjectionMatrix()
            * glm::inverse(costmapRenderer->getCamera()->getNode()->getWorldTransform());
    const glm::vec4 robotPosition = glm::column(scene->findNode(Config::getInstance().getParam<std::string>("robotCamera"))->getWorldTransform(), 3);
    const glm::vec4 position = mvp * robotPosition;
    glm::vec2 uv(position.x / position.w, position.y / position.w);
    if (uv.x < -1.f || uv.y < -1.f || uv.x > 1 || uv.y > 1) {
        logWarn("Robot position out of range: (%.2f, %.2f) not in range [-1..1, -1..1]", uv.x, uv.y);
    }
    glUniform2f(progBellmanFordXfbInit.locations.robotPosition, uv.x, uv.y);

    // source buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnable(GL_RASTERIZER_DISCARD);
    checkGLError();

    // target buffer
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfi[output]);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo[output]);
    checkGLError();

    // render
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, 1);
    glEndTransformFeedback();
    checkGLError();

    GLuint numPrimitives = 1;
    std::swap(input, output);

    // =========== STEP ===========
    progBellmanFordXfbStep.use();
    for (size_t i = 0; i < maxIterations && numPrimitives > 0; ++i) {
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        // source buffer
        glBindBuffer(GL_ARRAY_BUFFER, tbo[input]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        checkGLError();

        // target buffer
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfi[output]);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tbo[output]);
        checkGLError();

        // render
#ifndef IMMEDIATE_QUERY_READ
        if (i > numQueries) {
            // Deferred read back
            glGetQueryObjectuiv(numPrimitivesQuery[i % numQueries], GL_QUERY_RESULT, &numPrimitives);
        }
#endif
        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, numPrimitivesQuery[i % numQueries]);
        glBeginTransformFeedback(GL_POINTS);

#ifdef IMMEDIATE_QUERY_READ
        glDrawArrays(GL_POINTS, 0, numPrimitives);
#else
        glDrawTransformFeedback(GL_POINTS, tfi[input]);
#endif
        glEndTransformFeedback();
        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
#ifdef IMMEDIATE_QUERY_READ
        // Immediate read back
        glGetQueryObjectuiv(numPrimitivesQuery[i % numQueries], GL_QUERY_RESULT, &numPrimitives);
#endif

        std::swap(input, output);
    }

    // unbind buffers
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, GL_NONE);
    glDisable(GL_RASTERIZER_DISCARD);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    if (renderVisual) {
        progVisualizeIntTexture->use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[VISUAL], 0);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, textures[OUTPUT]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        checkGLError();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        checkGLError();
        glDrawArrays(GL_TRIANGLE_FAN, 1, 4);
        checkGLError();
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_NONE, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLError();
    if (renderToWindow) {
        glClear(GL_COLOR_BUFFER_BIT);
        const GLint s = std::min(oldViewport[2], oldViewport[3]);
        glViewport(oldViewport[0] + (oldViewport[2] - s) / 2, oldViewport[1] + (oldViewport[3] - s) / 2, s, s);
        progShowTexture->use();
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, textures[VISUAL]);
        glDrawArrays(GL_TRIANGLE_FAN, 1, 4);
        glBindTexture(GL_TEXTURE_2D, GL_NONE);
        checkGLError();

    }
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    glBindVertexArray(GL_NONE);
    if (oldDepthTest) {
        glEnable(GL_DEPTH_TEST);
    }
    checkGLError();
    glPopDebugGroup();

}

} /* namespace gpu_coverage */
