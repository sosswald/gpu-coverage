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

#include <gpu_coverage/VisibilityRenderer.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Config.h>
#include <sstream>
#include <utility>   // for std::pair
#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#if __ANDROID__
#define _isnan(x) isnan(x)
#define _isinf(x) isinf(x)
#endif
#include <glm/gtc/type_ptr.hpp>
#include <glm/packing.hpp>

namespace gpu_coverage {

VisibilityRenderer::VisibilityRenderer(const Scene * const scene, const bool renderToWindow, const bool countPixels)
        : AbstractRenderer(scene, "VisibilityRenderer"), renderToWindow(renderToWindow), countPixels(countPixels),
                progShowTexture(NULL), progPixelCounter(NULL),
                width(1280), height(960), textureWidth(1024), textureHeight(1024)
                        #ifndef GET_BUFFER_DIRECT
                        , progCounterToFB(NULL), numPbo(sizeof(pbo) / sizeof(pbo[0])), curPbo(0), run(0)
#endif
{
    std::string targetNames = Config::getInstance().getParam<std::string>("target");
    if (targetNames.empty()) {
        logError("No targets defined");
        return;
    }
    std::istringstream iss(targetNames);
    std::string targetName;
    while (iss.good()) {
        iss >> targetName;
        if (!targetName.empty()) {
            Node * targetNode = scene->findNode(targetName);
            const Texture * texture = targetNode->getMeshes().front()->getMaterial()->getTexture();
            if (!texture) {
                logError("Target %s does not have texture image", targetName.c_str());
                return;
            }
            GLuint textureId = texture->getTextureObject();
            targets.push_back(std::pair<Node *, GLuint>(targetNode, textureId));
        }
    }

    projectionPlaneNode = scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));
    if (!projectionPlaneNode) {
        return;
        logError("Could not find projection plane");
    }
    camera = scene->findNode(Config::getInstance().getParam<std::string>("robotCamera"))->getCameras()[0];
    if (!camera) {
        logError("Could not find camera");
        return;
    }

    if (!progFlat.isReady() || !progVisibility.isReady()) {
        return;
    }
    if (renderToWindow) {
        progShowTexture = new ProgramShowTexture();
        if (!progShowTexture->isReady()) {
            return;
        }
        progShowTexture->use();
        glUniform1i(progShowTexture->locations.textureUnit, 8);
    }
    if (countPixels) {
        progPixelCounter = new ProgramPixelCounter();
        if (!progPixelCounter->isReady()) {
            return;
        }
        progPixelCounter->use();
        glUniform1i(progPixelCounter->locations.textureUnit, 11);

#ifndef GET_BUFFER_DIRECT
        progCounterToFB = new ProgramCounterToFB();
        if (!progCounterToFB->isReady()) {
            return;
        }
#endif
    }

    progVisibility.use();
    glUniform1f(progVisibility.locations.resolution, static_cast<float>(textureWidth));
    checkGLError();

    numTextures = 4 + targets.size();
    glGenTextures(numTextures, textures);
    for (size_t i = 0; i < numTextures; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        switch (i) {
        case DEPTH:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, width, height);
            break;
        case COLOR:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
            break;
        case COUNTER:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, 1, 1);
            break;
        case VISIBILITY:
        case COLORTEXTURE:
        default:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, textureWidth, textureHeight);
            break;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glGenFramebuffers(2, framebuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    checkGLError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COLOR], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textures[DEPTH], 0);
    GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    checkGLError();
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        logError("Could not create framebuffer and textures for visibility rendering");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[1]);
    checkGLError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COLORTEXTURE], 0);
    checkGLError();
    drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        logError("Could not create framebuffer and textures for visibility rendering");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLError();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLfloat vertices[] = {
            -1.f, -1.f, 0.f,   // QUADS
            1.f, -1.f, 0.f,
            1.f, 1.f, 0.f,
            -1.f, 1.f, 0.f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    checkGLError();

    if (countPixels) {
        glGenBuffers(1, &pixelCountBuffer);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, pixelCountBuffer);
        glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, pixelCountBuffer);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        checkGLError();

#ifndef GET_BUFFER_DIRECT
        glGenBuffers(numPbo, pbo);
        for (size_t i = 0; i < numPbo; ++i) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[i]);
            glBufferData(GL_PIXEL_PACK_BUFFER, 4, NULL, GL_STREAM_READ);
        }
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        glGenVertexArrays(1, &vaoPoint);
        glBindVertexArray(vaoPoint);
        glGenBuffers(1, &vboPoint);
        glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
        const float o = -1.f + 1. / static_cast<float>(width);
        GLfloat verticesPoint[] = {
                -1.f - o, -1.f - o, 0.f,   // CopyToFB quad
                -1.f + o, -1.f - o, 0.f,
                -1.f + o, -1.f + o, 0.f,
                -1.f - o, -1.f + o, 0.f
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(verticesPoint), verticesPoint, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);
        checkGLError();
#endif
    }

    ready = true;

}

VisibilityRenderer::~VisibilityRenderer() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(numTextures, textures);
    glDeleteFramebuffers(2, framebuffers);
    if (countPixels) {
        glDeleteBuffers(1, &pixelCountBuffer);
#ifndef GET_BUFFER_DIRECT
        glDeleteBuffers(1, &vboPoint);
        glDeleteVertexArrays(1, &vaoPoint);
        glDeleteBuffers(numPbo, pbo);
        if (progCounterToFB) {
            delete progCounterToFB;
            progCounterToFB = NULL;
        }
#endif
    }
    if (progShowTexture) {
        delete progShowTexture;
        progShowTexture = NULL;
    }
    if (progPixelCounter) {
        delete progPixelCounter;
        progPixelCounter = NULL;
    }
    checkGLError();
}

void VisibilityRenderer::display() {
    display(countPixels);
}

void VisibilityRenderer::display(bool countPixels) {
    if (!ready) {
        return;
    }
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "visibility");
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    GLboolean oldDepthTest = glIsEnabled(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    checkGLError();

    glViewport(0, 0, width, height);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glEnable(GL_DEPTH_TEST);
    checkGLError();
    //GLfloat one = 1.f;
    //glClearBufferfv(GL_DEPTH, 0, &one);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    checkGLError();

    size_t targetI = 0;
    for (Targets::const_iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt, ++targetI) {
        // Clear visibility maps by copying texture
        glCopyImageSubData(targetIt->second, GL_TEXTURE_2D, 0, 0, 0, 0, textures[VISIBILITY + targetI], GL_TEXTURE_2D, 0, 0, 0, 0,
                textureWidth, textureHeight, 1);
    }

    // Render obstacles to depth map
    progFlat.use();
    glUniform4f(progFlat.locations.color, 0.f, 0.f, 0.f, 1.f);
    std::vector<bool> targetsVisible;
    for (Targets::const_iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt) {
        targetsVisible.push_back(targetIt->first->isVisible());
        targetIt->first->setVisible(false);
    }
    const bool projectionPlaneVisible = projectionPlaneNode->isVisible();
    projectionPlaneNode->setVisible(false);
    scene->render(camera, &progFlat.locationsMVP);
    std::vector<bool>::const_iterator tvIt = targetsVisible.begin();
    for (Targets::const_iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt, ++tvIt) {
        targetIt->first->setVisible(*tvIt);
    }
    projectionPlaneNode->setVisible(projectionPlaneVisible);

    if (countPixels && progPixelCounter) {
        // Reset counter
        GLuint pixelCount = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, pixelCountBuffer);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, pixelCountBuffer);
        glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &pixelCount);
    }

    targetI = 0;
    for (Targets::const_iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt, ++targetI) {
        // Render target and mark corresponding target hits in visibility map
        progVisibility.use();
        glBindImageTexture(7, textures[VISIBILITY + targetI], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8UI);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        std::vector<glm::mat4> view;
        camera->setViewProjection(progVisibility.locationsMVP, view);
        checkGLError();

        checkGLError();

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        targetIt->first->render(view, &progVisibility.locationsMVP, &progVisibility.locationsMaterial, false);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        glBindImageTexture(7, GL_NONE, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8UI);

        if (countPixels && progPixelCounter) {
            // Switch to framebuffer with target texture
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[1]);
            glViewport(0, 0, textureWidth, textureHeight);

            progPixelCounter->use();
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, textures[VISIBILITY + targetI]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COLORTEXTURE], 0);

            // Draw shader for full texture
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glBindVertexArray(0);
        }
    }

    if (countPixels && progPixelCounter) {
        // Wait for counter to be ready
        glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);

#ifdef GET_BUFFER_DIRECT
        glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT);
        GLuint * buffer = (GLuint *) glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
        pixelCount = buffer[0];
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
        pixelCounts.push_back(pixelCount);
#else
        progCounterToFB->use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COUNTER], 0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindVertexArray(vaoPoint);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[curPbo]);
        if (run >= numPbo) {
            // deferred read back
            readBack();
        }
        checkGLError();
        glReadPixels(0, 0, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL); // queues write to PBO
        checkGLError();
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        curPbo = (curPbo + 1) % numPbo;
        ++run;
#endif
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLError();
    if (!oldDepthTest) {
        glDisable(GL_DEPTH_TEST);
    }
    if (renderToWindow && progShowTexture) {
        glClear(GL_COLOR_BUFFER_BIT);
        const GLint s = std::min(oldViewport[2], oldViewport[3]);
        glViewport(oldViewport[0] + (oldViewport[2] - s) / 2, oldViewport[1] + (oldViewport[3] - s) / 2, s, s);
        progShowTexture->use();
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, textures[VISIBILITY]);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        checkGLError();
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    glPopDebugGroup();
    checkGLError();
}

void VisibilityRenderer::getPixelCounts(std::vector<GLuint>& counts) {
    if (!countPixels) {
        throw std::invalid_argument(std::string("Called getPixelsCount() although counting pixels is disabled"));
    }
#ifndef GET_BUFFER_DIRECT
    // if less runs than PBOs (= ring buffer not full yet): start with PBO 0
    // else: continue reading PBO ring buffer
    if (run < numPbo) {
        curPbo = 0;
    }
    while (pixelCounts.size() < run) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[curPbo]);
        readBack();
        curPbo = (curPbo + 1) & numPbo;
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    run = 0;
    curPbo = 0;
#endif
    counts.clear();
    counts.reserve(pixelCounts.size());
    counts.insert(counts.begin(), pixelCounts.begin(), pixelCounts.end());
    pixelCounts.clear();
}

#ifndef GET_BUFFER_DIRECT
void VisibilityRenderer::readBack() {
    unsigned int *ptr = (unsigned int *) glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, 4, GL_MAP_READ_BIT);
    checkGLError();
    if (ptr != NULL) {
        //const GLuint pixelCount = glm::packUnorm4x8(glm::vec4(ptr[0], ptr[1], ptr[2], ptr[3]) / 255.f);
        const GLuint pixelCount = *ptr;
        pixelCounts.push_back(pixelCount);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    } else {
        logError("Error: Could not map buffer");
    }
}
#endif

}  // namespace gpu_coverage
