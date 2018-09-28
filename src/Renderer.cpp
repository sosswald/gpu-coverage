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

#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/Node.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Config.h>
#include <assimp/postprocess.h>
#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <iostream>
#include <sstream>

#ifndef DATADIR
#define DATADIR "."
#endif

//#define USE_CUBEMAP

namespace gpu_coverage {

Renderer::Renderer(const Scene * const scene, const bool renderToWindow, const bool renderToTexture)
        : AbstractRenderer(scene, "Renderer"), renderToWindow(renderToWindow), renderToTexture(renderToTexture), width(
                640), height(480),
                progShowTexture(NULL) {
    camera = scene->findCamera(Config::getInstance().getParam<std::string>("externalCamera"));
    if (camera == NULL) {
        logError("Could not find camera");
        return;
    }
    if (!progVisualTexture.isReady() || !progVisualVertexcolor.isReady())
        return;

    if (renderToTexture) {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glGenTextures(2, textures);
        for (size_t i = 0; i < 2; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            switch (i) {
            case COLOR:
                glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
                break;
            case DEPTH:
                glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, width, height);
                break;
            }
        }

        checkGLError();
        if (renderToWindow) {
            progShowTexture = new ProgramShowTexture();
            progShowTexture->use();
            glUniform1i(progShowTexture->locations.textureUnit, 8);

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
            checkGLError();
        }
    }

    ready = true;
}

Renderer::~Renderer() {
    if (progShowTexture) {
        delete progShowTexture;
    }
    if (renderToTexture) {
        if (renderToWindow) {
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
        }
        glDeleteTextures(2, textures);
        glDeleteFramebuffers(1, &framebuffer);
    }
    checkGLError();
}

void Renderer::display() {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "renderer");
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    GLboolean oldDepthTest = glIsEnabled(GL_DEPTH_TEST);
    if (renderToTexture) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COLOR], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textures[DEPTH], 0);
        GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &drawBuffer);
        checkGLError();
        glViewport(0, 0, width, height);
    }
    progVisualTexture.use();
    // Clear view
#if !HAS_GLES
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.4f, 0.4f, 0.6f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene->render(camera, &progVisualTexture.locationsMVP, &progVisualTexture.locationsLight,
            &progVisualTexture.locationsMaterial, false);

    // render coordinate system(s) for pano cameras
    progVisualVertexcolor.use();
    for (Scene::PanoramaCameras::const_iterator panoIt = scene->panoramaCameras.begin(); panoIt != scene->panoramaCameras.end(); ++panoIt) {
        std::vector<glm::mat4> view;
        camera->setViewProjection(progVisualVertexcolor.locationsMVP, view);
        const glm::mat4 modelMatrix = (*panoIt)->getNode()->getWorldTransform();
        glUniformMatrix4fv(progVisualVertexcolor.locationsMVP.modelMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        coordinateAxes.display();
    }

    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    if (!oldDepthTest) {
        glDisable(GL_DEPTH_TEST);
    }
    if (renderToTexture && progShowTexture) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (renderToWindow) {
            progShowTexture->use();
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, textures[COLOR]);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            checkGLError();
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindVertexArray(0);
        }
    }
    glPopDebugGroup();
}

} /* namespace gpu_coverage */
