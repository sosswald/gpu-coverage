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
#include <gpu_coverage/PanoEvalRenderer.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Config.h>

#if HAS_OPENCV
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif
#include GL_INCLUDE
#include GLEXT_INCLUDE
#include <cstdio>
#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/detail/type_mat.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gpu_coverage {

PanoEvalRenderer::PanoEvalRenderer(const Scene * const scene, const bool renderToWindow, const bool renderToTexture,
        PanoRenderer * const panoRenderer, AbstractRenderer * const bellmanFordRenderer)
        : AbstractRenderer(scene, "PanoEvalRenderer"),
                renderToWindow(renderToWindow), renderToTexture(renderToTexture || renderToWindow),
                benchmark(false),
                panoRenderer(panoRenderer), progVisualizeIntTexture(NULL),
                progShowTexture(NULL),
                numPbo(sizeof(pbo) / sizeof(pbo[0])), maxIterations(2000),
                textureToVisualize(UTILITY_MAP_1), curUtilityMap(UTILITY_MAP_1)
{
    if (!progPanoEval.isReady() || !progTLEdge.isReady() || !progTLStep.isReady()
            || !progCounterToFB.isReady() || !progUtility1.isReady() || !progUtility2.isReady()) {
        return;
    }
    
    targetNode = scene->findNode(Config::getInstance().getParam<std::string>("target"));
    projectionPlaneNode = scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));

    panoTexture = panoRenderer->getTexture();
    panoWidth = panoRenderer->getTextureWidth();
    panoHeight = panoRenderer->getTextureHeight();

    bellmanFordTexture = bellmanFordRenderer->getTexture();
    bellmanFordWidth = bellmanFordRenderer->getTextureWidth();
    bellmanFordHeight = bellmanFordRenderer->getTextureHeight();

    if (renderToTexture) {
        progVisualizeIntTexture = new ProgramVisualizeIntTexture();
        if (!progVisualizeIntTexture->isReady()) {
            return;
        }

        progVisualizeIntTexture->use();
        glUniform1i(progVisualizeIntTexture->locations.textureUnit, 9);
        int maxDist, minDist;
        switch (textureToVisualize) {
            case GAIN1:
            case GAIN2:
                minDist = 0;
                maxDist = 100;
                break;
            case UTILITY_MAP_1:
                minDist = -100;
                maxDist = bellmanFordWidth * 100;
                break;
            default:
                minDist = 0;
                maxDist = 1;
        }
        glUniform1i(progVisualizeIntTexture->locations.minDist, minDist);
        glUniform1i(progVisualizeIntTexture->locations.maxDist, maxDist);
    }

    if (renderToWindow) {
        progShowTexture = new ProgramShowTexture();
        progShowTexture->use();
        glUniform1i(progShowTexture->locations.textureUnit, 9);
    }

    progUtility1.use();
    glUniform1i(progUtility1.locations.utilityUnit, 9);
    glUniform1i(progUtility1.locations.gain1Unit, 10);

    progUtility2.use();
    glUniform1i(progUtility2.locations.utilityUnit, 9);
    glUniform1i(progUtility2.locations.gain1Unit, 10);
    glUniform1i(progUtility2.locations.gain2Unit, 11);

    progPanoEval.use();
    glUniform1i(progPanoEval.locations.textureUnit, 10);
    glUniform1i(progPanoEval.locations.integral, 11);
    glUniform1f(progPanoEval.locations.resolution, bellmanFordWidth);

    if (!progTLEdge.isReady()) {
        return;
    }
    progTLEdge.use();
    glUniform1i(progTLEdge.locations.textureUnit, 10);

    progTLStep.use();
    glUniform1i(progTLStep.locations.textureUnit, 10);

    // Generate frame buffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    checkGLError();

    // Generate textures
    glGenTextures(sizeof(textures) / sizeof(textures[0]), textures);
    for (size_t i = 0; i < sizeof(textures) / sizeof(textures[0]); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        switch (i) {
        case EVAL:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, panoWidth, panoHeight);
            break;
        case SWAP1:
        case SWAP2:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32I, panoWidth, panoHeight);
            break;
        case COUNTER:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 1, 1);
            break;
        case UTILITY_MAP_1:
        case UTILITY_MAP_2:
        case GAIN1:
        case GAIN2:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32I, bellmanFordWidth, bellmanFordHeight);
            break;
        case UTILITY_MAP_VISUAL:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, bellmanFordWidth, bellmanFordHeight);
            break;
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const float o = -1.f + 1. / static_cast<float>(panoWidth);
    GLfloat vertices[] = {
            -1.f, -1.f, 0.f,
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
    checkGLError();

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

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    checkGLError();
    ready = true;
}

PanoEvalRenderer::~PanoEvalRenderer() {
    if (progVisualizeIntTexture) {
        delete progVisualizeIntTexture;
        progVisualizeIntTexture = NULL;
    }
    if (progShowTexture) {
        delete progShowTexture;
        progShowTexture = NULL;
    }
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(sizeof(textures) / sizeof(textures[0]), textures);
}

void PanoEvalRenderer::display() {
    if (!ready) {
        return;
    }
    if (panoEdgePairs.empty()) {
        logError("PanoEvalRenderer::display() called without panorama camera");
        return;
    }

    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    GLboolean oldDepthTest = glIsEnabled(GL_DEPTH_TEST);

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "pano-eval");
    // Copy cost map to utility map
    glCopyImageSubData(bellmanFordTexture, GL_TEXTURE_2D, 0, 0, 0, 0, textures[curUtilityMap], GL_TEXTURE_2D, 0, 0, 0, 0,
            bellmanFordWidth, bellmanFordHeight, 1);
    checkGLError();

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    for (PanoEdgePairs::const_iterator pairIt = panoEdgePairs.begin(); pairIt != panoEdgePairs.end(); ++pairIt) {
        // Clear gain maps
        glViewport(0, 0, bellmanFordWidth, bellmanFordHeight);
        GLint clear[4] = {0, 0, 0, 0};
        for (size_t iCam = 0; iCam < (pairIt->second.camera == NULL ? 1 : 2); ++iCam) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[GAIN1+iCam], 0);
            glClearBufferiv(GL_COLOR, 0, clear);
        }

        glViewport(0, 0, panoWidth, panoHeight);
        for (size_t iCam = 0; iCam < (pairIt->second.camera == NULL ? 1 : 2); ++iCam) {
            // Render panorama with semantic information
            const bool clockwise = iCam == 0 ? pairIt->first.clockwise : pairIt->second.clockwise;
            if (!benchmark) {
                CameraPanorama * const camera = iCam == 0 ? pairIt->first.camera : pairIt->second.camera;
                panoRenderer->setCamera(camera);
                checkGLError();
                panoRenderer->display();
                checkGLError();
            }

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
            glBindVertexArray(vao);

            // Calculate integral image
            progTLEdge.use();
            glUniform1f(progTLEdge.locations.width, (clockwise ? 1 : -1) * panoWidth);
            glUniform1f(progTLEdge.locations.height, (clockwise ? 1 : -1) * panoHeight);
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, panoTexture);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[SWAP1], 0);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            checkGLError();

            GLuint pixelCount = 1;
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counterBuffer);
            glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, counterBuffer);
            size_t iteration;
            int inputTexture = SWAP1;
            int outputTexture = SWAP2;
            checkGLError();
            for (iteration = 0; iteration < maxIterations && pixelCount > 0; ++iteration) {
                // clear counter
                pixelCount = 0;
                glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &pixelCount);
                checkGLError();

                // set input and output texture
                progTLStep.use();
                glUniform1f(progTLStep.locations.width, (clockwise ? 1 : -1) * panoWidth);
                glUniform1f(progTLStep.locations.height, (clockwise ? 1 : -1) * panoHeight);
                glActiveTexture(GL_TEXTURE10);
                glBindTexture(GL_TEXTURE_2D, textures[inputTexture]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[outputTexture], 0);
                checkGLError();

                // Draw
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

                // Unbind framebuffer texture
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_NONE, 0);

                // Wait for counter to be ready
                glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);

                // Deferred readback via pixel buffer object
                progCounterToFB.use();
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COUNTER], 0);
                glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

                checkGLError();
                glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[iteration % numPbo]);
                if (iteration >= numPbo) {
                    unsigned int *ptr = (unsigned int *) glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, 4, GL_MAP_READ_BIT);
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
                std::swap(inputTexture, outputTexture);
            }

            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            // Render evaluation shader (writes to gain map)
            glViewport(0, 0, panoWidth, panoHeight);
            glBindImageTexture(7, textures[GAIN1 + iCam], 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
            checkGLError();
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[EVAL], 0);
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, panoTexture);
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, textures[inputTexture]);
            progPanoEval.use();
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            checkGLError();

            // Unbind utility map
            glBindImageTexture(7, GL_NONE, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
            glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
            checkGLError();
        }

        // Render eval (writes to gain map)
        // TODO: extend for more than 1 panorama edge pair
        glViewport(0, 0, bellmanFordWidth, bellmanFordHeight);
        //glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "test");
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, textures[curUtilityMap]);
        curUtilityMap = (curUtilityMap == UTILITY_MAP_1 ? UTILITY_MAP_2 : UTILITY_MAP_1);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[curUtilityMap], 0);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, textures[GAIN1]);
        if (pairIt->second.camera == NULL) {
            progUtility1.use();
        } else {
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, textures[GAIN2]);
            progUtility2.use();
        }
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        checkGLError();
        //glPopDebugGroup();
    }

    if (renderToTexture) {
        // Visualize utility map
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               textures[UTILITY_MAP_VISUAL], 0);
        glActiveTexture(GL_TEXTURE9);
        switch(textureToVisualize) {
        case EVAL:
        case UTILITY_MAP_1:
            glBindTexture(GL_TEXTURE_2D, textures[curUtilityMap]);
            break;
        default:
            glBindTexture(GL_TEXTURE_2D, textures[textureToVisualize]);
            break;
        }
        progVisualizeIntTexture->use();
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
        checkGLError();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (renderToWindow) {
        checkGLError();
        if (textureToVisualize == EVAL) {
            glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
        } else {
            glClear(GL_COLOR_BUFFER_BIT);
            const GLint s = std::min(oldViewport[2], oldViewport[3]);
            glViewport(oldViewport[0] + (oldViewport[2] - s) / 2, oldViewport[1] + (oldViewport[3] - s) / 2, s, s);
        }
        progShowTexture->use();
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, textureToVisualize == EVAL ? textures[EVAL] : textures[UTILITY_MAP_VISUAL]);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        checkGLError();
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    if (oldDepthTest) {
        glEnable(GL_DEPTH_TEST);
    }
    glPopDebugGroup();

}

void PanoEvalRenderer::addCamera(CameraPanorama * const cam) {
    panoEdgePairs.push_back(PanoEdgePair(PanoEdge(cam, false), PanoEdge(NULL, true)));
}

void PanoEvalRenderer::addCameraPair(CameraPanorama * const first, CameraPanorama * const second) {
    panoEdgePairs.push_back(PanoEdgePair(PanoEdge(first, false), PanoEdge(second, true)));

}

} /* namespace gpu_coverage */
