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

#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Config.h>
#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/gtc/matrix_transform.hpp>

namespace gpu_coverage {

CostMapRenderer::CostMapRenderer(const Scene * const scene, const Node * const projectionPlane, const bool renderToWindow, const bool visual)
        : AbstractRenderer(scene, "CostMapRenderer"), renderToWindow(renderToWindow),
          renderVisual(renderToWindow | visual),
          camera(NULL), cameraNode(NULL), floorNode(NULL), planeNode(NULL),
          progCostMapVisual(NULL), progShowTexture(NULL),
          width(256), height(256) {
    if (!progJFA.isReady() || !progSeed.isReady() || !progCostMap.isReady()) {
        return;
    }
    if (renderVisual) {
        progCostMapVisual = new ProgramCostMapVisual();
        if (!progCostMapVisual->isReady()) {
            return;
        }
        checkGLError();
        progCostMapVisual->use();
        glUniform1i(progCostMapVisual->locations.textureUnit, 7);
        glUniform1f(progCostMapVisual->locations.resolution, width);
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

    cameraNode = new Node("distance cam node", scene->getRoot());
    const float scale = projectionPlane->getLocalTransform()[0][0];
    glm::vec3 center = glm::vec3(projectionPlane->getLocalTransform()[3]);

    cameraNode->setLocalTransform(glm::inverse(glm::lookAt(
            center,
            center + glm::vec3(0.f, 0.f, -1.f),
            glm::vec3(0.f, 1.f, 0.f)
                    )));
    camera = new CameraOrtho(scene->cameras.size(), "distance map cam", cameraNode,
            -scale, scale, scale, -scale, 0.1f, 100.0f);
    cameraNode->addCamera(camera);

    floorNode = scene->findNode(Config::getInstance().getParam<std::string>("floor"));
    if (!floorNode) {
        logError("Could not find floor node %s", Config::getInstance().getParam<std::string>("floor").c_str());
    }
    floorProjectionNode = scene->findNode(Config::getInstance().getParam<std::string>("floorProjection"));
    if (!floorProjectionNode) {
        logError("Could not find projection floor node %s", Config::getInstance().getParam<std::string>("floorProjection").c_str());
    }
    planeNode = scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));
    if (!planeNode) {
        logError("Could not projection plane node %s", Config::getInstance().getParam<std::string>("projectionPlane").c_str());
    }

    progSeed.use();
    glUniform1f(progSeed.locations.resolution, static_cast<float>(width));
    progJFA.use();
    glUniform1f(progJFA.locations.resolution, static_cast<float>(width));
    progCostMap.use();
    glUniform1f(progCostMap.locations.resolution, static_cast<float>(width));
    checkGLError();

    // Generate frame buffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    checkGLError();

    glGenTextures(4, textures);
    checkGLError();
    for (size_t i = 0; i < 4; ++i) {
        glActiveTexture(GL_TEXTURE2 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        switch(i) {
        case SWAP1:
        case SWAP2:
        case VISUAL:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
            break;
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
    GLfloat vertices[12] = {
            -1.f, -1.f, 0.f,
            1.f, -1.f, 0.f,
            1.f, 1.f, 0.f,
            -1.f, 1.f, 0.f
    };
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLError();

    ready = true;

}

CostMapRenderer::~CostMapRenderer() {
    delete cameraNode;
    delete camera;
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(4, textures);
    glDeleteFramebuffers(1, &framebuffer);
    if (progShowTexture) {
        delete progShowTexture;
        progShowTexture = NULL;
    }
    if (progCostMapVisual) {
        delete progCostMapVisual;
        progCostMapVisual = NULL;
    }

    checkGLError();
}

void CostMapRenderer::display() {
    if (!ready) {
        return;
    }
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "costmap");
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    GLboolean oldDepthTest = glIsEnabled(GL_DEPTH_TEST);
    progSeed.use();
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[SWAP1], 0);
    GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    checkGLError();
    glViewport(0, 0, width, height);
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    checkGLError();
    const bool floorVisible = floorNode->isVisible();
    floorNode->setVisible(false);
    const bool floorProjectionVisible = floorProjectionNode->isVisible();
    floorProjectionNode->setVisible(false);
    const bool planeVisible = planeNode->isVisible();
    planeNode->setVisible(false);
    scene->render(camera, &progSeed.locationsMVP);
    if (floorVisible) {
        floorNode->setVisible(true);
    }
    if (floorProjectionVisible) {
        floorProjectionNode->setVisible(true);
    }
    if (planeVisible) {
        planeNode->setVisible(true);
    }
    checkGLError();

    progJFA.use();
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);
    glBindVertexArray(vao);

    // Bind textures
    static const GLuint textureUnit = 2;
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, textures[SWAP1]);
    glActiveTexture(GL_TEXTURE0 + textureUnit + 1);
    glBindTexture(GL_TEXTURE_2D, textures[SWAP2]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[SWAP1], 0);

    int inputTexture = SWAP1;
    int outputTexture = SWAP2;

    for (int stepSize = width / 2; stepSize > 0; stepSize /= 2) {
        glUniform1i(progJFA.locations.textureUnit, textureUnit + inputTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[outputTexture], 0);
        checkGLError();
        glUniform1i(progJFA.locations.stepSize, stepSize);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        // Swap textures
        std::swap(outputTexture, inputTexture);
    }

    progCostMap.use();
    checkGLError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[OUTPUT], 0);
    glDrawBuffers(1, &drawBuffer);
    checkGLError();
    glUniform1i(progCostMap.locations.textureUnit, textureUnit + inputTexture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGLError();

    if (renderVisual) {
        progCostMapVisual->use();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[VISUAL], 0);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, textures[outputTexture]);
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
    glBindVertexArray(0);
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    if (oldDepthTest) {
        glEnable(GL_DEPTH_TEST);
    }
    checkGLError();
    glPopDebugGroup();
}

} /* namespace gpu_coverage */
