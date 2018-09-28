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

#include <gpu_coverage/BellmanFordRenderer.h>
#include <gpu_coverage/Utilities.h>
#include <fstream>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/string_cast.hpp>

namespace gpu_coverage {

BellmanFordRenderer::BellmanFordRenderer(const Scene * const scene, const CostMapRenderer * const costmapRenderer,
        const bool renderToWindow, const bool visual)
        : AbstractRenderer(scene, "BellmanFordRenderer"), costmapRenderer(costmapRenderer), renderToWindow(renderToWindow), renderVisual(
                visual || renderToWindow),
                progShowTexture(NULL), progVisualizeIntTexture(NULL), width(256), height(256), maxIterations(
                        1024),
                numPbo(sizeof(pbo) / sizeof(pbo[0]))
{
    if (!progInit.isReady() || !progStep.isReady() || !progCounterToFB.isReady()) {
        return;
    }
    if (renderVisual) {
        progVisualizeIntTexture = new ProgramVisualizeIntTexture();
        if (!progVisualizeIntTexture->isReady()) {
            return;
        }
        checkGLError();
        progVisualizeIntTexture->use();
        checkGLError();
        glUniform1i(progVisualizeIntTexture->locations.textureUnit, 7);
        checkGLError();
        glUniform1i(progVisualizeIntTexture->locations.minDist, 0);
        glUniform1i(progVisualizeIntTexture->locations.maxDist, width * (M_SQRT2 * 100));
        checkGLError();
    }
    if (renderToWindow) {
        progShowTexture = new ProgramShowTexture();
        if (!progShowTexture->isReady()) {
            return;
        }
        progShowTexture->use();
        glUniform1i(progShowTexture->locations.textureUnit, 8);
    }
    checkGLError();
    progInit.use();
    glUniform1f(progInit.locations.resolution, width);
    glUniform1i(progInit.locations.costmapTextureUnit, 5);
    progStep.use();
    glUniform1f(progStep.locations.resolution, width);
    glUniform1i(progStep.locations.costmapTextureUnit, 5);
    glUniform1i(progStep.locations.inputTextureUnit, 6);
    checkGLError();

    // Generate frame buffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    checkGLError();

    // Generate textures
    glGenTextures(5, textures);
    glActiveTexture(GL_TEXTURE6);
    for (size_t i = 0; i < 5; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        switch (i) {
        case VISUAL:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
            break;
        case COUNTER:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 1, 1);
            break;
        case SWAP1:
        case SWAP2:
        case OUTPUT:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32I, width, height);
            break;
        }

    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[SWAP1], 0);
    checkGLError();

    GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        logError("Could not create framebuffer and textures for distance map rendering");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    checkGLError();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const float o = -1.f + 1. / static_cast<float>(width);
    GLfloat vertices[] = {
            -1.f, -1.f, 0.f,   // QUADS
            1.f, -1.f, 0.f,
            1.f, 1.f, 0.f,
            -1.f, 1.f, 0.f,

            -1.f - o, -1.f - o, 0.f,   // CopyToFB quad
            -1.f + o, -1.f - o, 0.f,
            -1.f + o, -1.f + o, 0.f,
            -1.f - o, -1.f + o, 0.f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &counterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, counterBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    checkGLError();

    glGenBuffers(numPbo, pbo);
    for (size_t i = 0; i < numPbo; ++i) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, 4, NULL, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    checkGLError();

    ready = true;

}

BellmanFordRenderer::~BellmanFordRenderer() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(5, textures);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteBuffers(1, &counterBuffer);
    glDeleteBuffers(numPbo, pbo);
    if (progShowTexture) {
        delete progShowTexture;
        progShowTexture = NULL;
    }
    if (progVisualizeIntTexture) {
        delete progVisualizeIntTexture;
        progVisualizeIntTexture = NULL;
    }
    checkGLError();
}

void BellmanFordRenderer::display() {
    if (!ready) {
        return;
    }
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "bellmanford");
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    GLboolean oldDepthTest = glIsEnabled(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    checkGLError();

    // Bind textures
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, costmapRenderer->getTexture());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[SWAP1], 0);
    GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    glViewport(0, 0, width, height);
    //Clearing is not necessary as first shader renders the full FB
    //glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);

    progInit.use();
    glBindVertexArray(vao);
    const glm::mat4 mvp = costmapRenderer->getCamera()->getProjectionMatrix()
            * glm::inverse(costmapRenderer->getCamera()->getNode()->getWorldTransform());
    const glm::vec4 position = mvp * glm::vec4(0.f, 0.f, 0.f, 1.f);
    const glm::ivec2 uv(position.x / position.w * width, position.y / position.w * height);
    glUniform2i(progInit.locations.robotPixel, uv.x, uv.y);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGLError();

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COUNTER], 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    int inputTexture = SWAP1;
    int outputTexture = SWAP2;

    glActiveTexture(GL_TEXTURE6);

    GLuint pixelCount = 1;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, counterBuffer);
    size_t iteration;
    for (iteration = 0; iteration < maxIterations && pixelCount > 0; ++iteration) {
        // Expand frontier wave by one step
        progStep.use();

        // clear counter
        pixelCount = 0;
        glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &pixelCount);

        // set input and output texture
        glBindTexture(GL_TEXTURE_2D, textures[inputTexture]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[outputTexture], 0);
        checkGLError();

        // Draw
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // Unbind framebuffer texture
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_NONE, 0);

        // Wait for counter to be ready
        glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);

        // Count pixels
#ifdef GET_BUFFER_DIRECT
        GLuint * buffer = (GLuint *) glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
        pixelCount = buffer[0];
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
#else
        // Deferred readback via pixel buffer object
        progCounterToFB.use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COUNTER], 0);
        glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

        checkGLError();
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[iteration % numPbo]);
        if (iteration >= numPbo) {
            /*struct timespec startTime, endTime;
            if (Config::getInstance().TMP_DEBUG) {
                clock_gettime(CLOCK_MONOTONIC, &startTime);
            }*/
            unsigned int *ptr = (unsigned int *) glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, 4, GL_MAP_READ_BIT);
            /*if (Config::getInstance().TMP_DEBUG) {
                clock_gettime(CLOCK_MONOTONIC, &endTime);
                const double startT = static_cast<double>(startTime.tv_sec) + static_cast<double>(startTime.tv_nsec) * 1e-9;
                const double endT = static_cast<double>(endTime.tv_sec) + static_cast<double>(endTime.tv_nsec) * 1e-9;
                printf("BELLMANFORD %zu: %f\n", iteration, endT - startT);
            }*/
            checkGLError();
            if (ptr != NULL) {
                pixelCount = *ptr;
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            } else {
                logError("Could not map buffer");
            }
        } else {
            pixelCount = 1; // dummy value for first 4 iterations
        }
        checkGLError();
        glReadPixels(0, 0, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
        checkGLError();
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        checkGLError();
#endif
        std::swap(inputTexture, outputTexture);
    }
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    //std::cout << std::endl;
    //std::cout << "Bellman-Ford: " << iteration << " iterations" << std::endl;

    glCopyImageSubData(textures[inputTexture], GL_TEXTURE_2D, 0, 0, 0, 0, textures[OUTPUT], GL_TEXTURE_2D, 0, 0, 0, 0, width, height, 1);

    if (renderVisual) {
        progVisualizeIntTexture->use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[VISUAL], 0);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, textures[OUTPUT]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
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
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
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

}  // namespace gpu_coverage
