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
#include <gpu_coverage/PanoRenderer.h>
#include <gpu_coverage/Utilities.h>
#include <sstream>
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

PanoRenderer::PanoRenderer(const Scene * const scene, const bool renderToWindow, AbstractRenderer * const bellmanFordRenderer)
        : AbstractRenderer(scene, "PanoRenderer"),
                renderToWindow(renderToWindow), renderSemantic(Config::getInstance().getParam<bool>("panoSemantic")),
                camera(NULL),
                progPano(NULL), progPanoSemantic(NULL), progCostmapIndex(NULL),
                cubemapWidth(1024), cubemapHeight(1024), progMapProjection(NULL), mapProjectionVao(0),
                mapProjectionVbo(0), debug(false), bellmanFordRenderer(bellmanFordRenderer)
{
    panoOutputFormat = Config::getInstance().getParam<Config::PanoOutputValue>("panoOutputFormat");
    renderToCubemap = Config::getInstance().getParam<bool>("renderToCubemap");

    projectionPlaneNode = scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));
    if (!projectionPlaneNode) {
        logError("Projection plane node not found");
        return;
    }
    floorNode = scene->findNode(Config::getInstance().getParam<std::string>("floor"));
    if (!floorNode) {
        logError("Floor plane node not found");
        return;
    }


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
            if (!targetNode) {
                logError("Target node %s not found", targetName.c_str());
                return;
            }
            targetNodes.push_back(targetNode);
        }
    }

    switch (panoOutputFormat) {
    case Config::EQUIRECTANGULAR:
        panoWidth = 1024;
        panoHeight = 512;
        break;
    case Config::CYLINDRICAL:
        panoWidth = 1024;
        panoHeight = 512;
        break;
    case Config::CUBE:
        panoWidth = 1024;
        panoHeight = 768;
        break;
    case Config::IMAGE_STRIP_HORIZONTAL:
        panoWidth = 1020;
        panoHeight = 170;
        break;
    case Config::IMAGE_STRIP_VERTICAL:
        panoWidth = 170;
        panoHeight = 1020;
        break;
    default:
        logError("Unknown panorama output format");
        return;
    }

    progShowTexture.use();
    glUniform1i(progShowTexture.locations.textureUnit, 9);


    if (renderSemantic) {
        progPanoSemantic = new ProgramPanoSemantic();
        if (!progPanoSemantic->isReady()) {
            return;
        }
        progCostmapIndex = new ProgramCostmapIndex();
        if (!progCostmapIndex->isReady()) {
            return;
        }
        progCostmapIndex->use();
        glUniform1i(progCostmapIndex->locations.textureUnit, 3);
        glUniform1i(progCostmapIndex->locations.width, bellmanFordRenderer->getTextureWidth());
        glUniform1i(progCostmapIndex->locations.height, bellmanFordRenderer->getTextureHeight());

    } else {
        progPano = new ProgramPano();
        if (!progPano->isReady()) {
            return;
        }
    }

    if (panoOutputFormat == Config::EQUIRECTANGULAR || panoOutputFormat == Config::CYLINDRICAL) {
        // force rendering to cubemap
        renderToCubemap = true;
        if (panoOutputFormat == Config::EQUIRECTANGULAR) {
            progMapProjection = new ProgramEquirectangular();
        } else {
            progMapProjection = new ProgramCylindrical();
        }
        if (!progMapProjection->isReady())
            return;
        glGenVertexArrays(1, &mapProjectionVao);
        glBindVertexArray(mapProjectionVao);
        glGenBuffers(1, &mapProjectionVbo);
        glBindBuffer(GL_ARRAY_BUFFER, mapProjectionVbo);
        GLfloat vertices[] = {
                -1.f, -1.f, 0.f,
                 1.f, -1.f, 0.f,
                 1.f,  1.f, 0.f,
                -1.f, -1.f, 0.f,
                 1.f,  1.f, 0.f,
                -1.f,  1.f, 0.f
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);
        mapProjectionCount = 6;
        checkGLError();
    } else {
        if (renderToCubemap) {
            progMapProjection = new ProgramFlatProjectionCube();
        } else {
            progMapProjection = new ProgramFlatProjection2dArray();
        }
        if (!progMapProjection->isReady())
            return;
        glGenVertexArrays(1, &mapProjectionVao);
        glBindVertexArray(mapProjectionVao);
        glGenBuffers(1, &mapProjectionVbo);
        glBindBuffer(GL_ARRAY_BUFFER, mapProjectionVbo);
        GLfloat vertices[72];
        std::pair<GLfloat, GLfloat> base[6];
        float patchWidth, patchHeight;
        switch (panoOutputFormat) {
        case Config::IMAGE_STRIP_HORIZONTAL:
            for (size_t i = 0; i < 6; ++i) {
                base[i] = std::make_pair<GLfloat, GLfloat>(static_cast<float>(i) / 3.f - 1.f, -1.f);
            }
            patchWidth = 1. / 3.f;
            patchHeight = 2.f;
            break;
        case Config::IMAGE_STRIP_VERTICAL:
            for (size_t i = 0; i < 6; ++i) {
                base[i] = std::make_pair<GLfloat, GLfloat>(-1.f, static_cast<float>(5 - i) / 3.f - 1.f);
            }
            patchWidth = 2.f;
            patchHeight = 1. / 3.f;
            break;
        case Config::CUBE:
            base[0] = std::make_pair<GLfloat, GLfloat>(0.0f, -1.f / 3.f);
            base[1] = std::make_pair<GLfloat, GLfloat>(-1.0f, -1.f / 3.f);
            base[2] = std::make_pair<GLfloat, GLfloat>(-0.5f, -1.f);
            base[3] = std::make_pair<GLfloat, GLfloat>(-0.5f, 1.f / 3.f);
            base[4] = std::make_pair<GLfloat, GLfloat>(-0.5f, -1.f / 3.f);
            base[5] = std::make_pair<GLfloat, GLfloat>(0.5f, -1.f / 3.f);
            patchWidth = 0.5f;
            patchHeight = 2. / 3.f;
            break;
        default:
            throw std::runtime_error("Invalid output format for flat projection shader");
        }
        for (size_t i = 0; i < 6; ++i) {
            vertices[12 * i + 0] = base[i].first;
            vertices[12 * i + 1] = base[i].second;
            vertices[12 * i + 2] = base[i].first + patchWidth;
            vertices[12 * i + 3] = base[i].second;
            vertices[12 * i + 4] = base[i].first + patchWidth;
            vertices[12 * i + 5] = base[i].second + patchHeight;
            vertices[12 * i + 6] = base[i].first;
            vertices[12 * i + 7] = base[i].second;
            vertices[12 * i + 8] = base[i].first + patchWidth;
            vertices[12 * i + 9] = base[i].second + patchHeight;
            vertices[12 * i + 10] = base[i].first;
            vertices[12 * i + 11] = base[i].second + patchHeight;
        }
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);
        mapProjectionCount = 36;
    }

    // Generate frame buffer
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    checkGLError();

    // Generate textures
    glGenTextures(1, &depthCubeMap);
    glGenTextures(1, &colorCubeMap);
    if (renderToCubemap) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
#if __ANDROID__
        for (size_t i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24, cubemapWidth, cubemapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
        }
#else
        glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_DEPTH_COMPONENT24, cubemapWidth, cubemapHeight);
#endif

        glBindTexture(GL_TEXTURE_CUBE_MAP, colorCubeMap);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
        glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, cubemapWidth, cubemapHeight);
#if HAS_OPENCV
        for (size_t i = 0; i < 6; ++i) {
            if (debug) {
                GLvoid * pixels;
                cv::Mat mat, flip;
                std::stringstream ss;
                ss << DATADIR "/textures/debug" << i << ".png";
                mat = cv::Mat(cv::imread(ss.str().c_str(), cv::IMREAD_COLOR));
                flip = cv::Mat(mat.rows, mat.cols, mat.type());
                cv::flip(mat, flip, 0);
                pixels = flip.data;
                glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, cubemapWidth, cubemapHeight, GL_BGR,
                        GL_UNSIGNED_BYTE, pixels);
            }
        }
        #endif
        checkGLError();
        progMapProjection->use();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, colorCubeMap);
        glUniform1i(progMapProjection->locations.textureUnit, 1);
        glActiveTexture(GL_TEXTURE0);
        checkGLError();
    } else {
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthCubeMap);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_DEPTH_COMPONENT24, cubemapWidth, cubemapHeight, 6);
        glBindTexture(GL_TEXTURE_2D_ARRAY, colorCubeMap);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, renderSemantic ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, renderSemantic ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
#if HAS_OPENCV
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, cubemapWidth, cubemapHeight, 6);
        if (debug) {
            unsigned char * pixels = (unsigned char *) malloc(6 * cubemapWidth * cubemapHeight * 3);
            cv::Mat mat;
            cv::Mat flip;
            for (size_t i = 0; i < 6; ++i) {
                std::stringstream ss;
                ss << DATADIR "/textures/debug" << i << ".png";
                mat = cv::Mat(cv::imread(ss.str().c_str(), cv::IMREAD_COLOR));
                flip = cv::Mat(mat.rows, mat.cols, mat.type());
                cv::flip(mat, flip, 0);
                memcpy(&pixels[i * cubemapWidth * cubemapHeight * 3], flip.data, cubemapWidth * cubemapHeight * 3);
            }
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, cubemapWidth, cubemapHeight, 6, GL_RGB, GL_UNSIGNED_BYTE,
                    pixels);
            free(pixels);
        }
#endif
        progMapProjection->use();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D_ARRAY, colorCubeMap);
        glUniform1i(progMapProjection->locations.textureUnit, 1);
        glActiveTexture(GL_TEXTURE0);
        checkGLError();
    }

    // Bind texture to framebuffer
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorCubeMap, 0);
    GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    checkGLError();
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        logError("Could not create framebuffer and textures for panorama rendering");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_NONE, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_NONE, 0);

    checkGLError();

    glGenTextures(sizeof(textures)/sizeof(textures[0]), textures);
    for (size_t i = 0; i < sizeof(textures)/sizeof(textures[0]); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        switch (i) {
        case PANO:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, renderSemantic ? GL_NEAREST : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, renderSemantic ? GL_NEAREST : GL_LINEAR);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, panoWidth, panoHeight);
            break;
        case COSTMAP_INDEX:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, bellmanFordRenderer->getTextureWidth(), bellmanFordRenderer->getTextureHeight());
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
             1.f,  1.f, 0.f,
            -1.f,  1.f, 0.f,

            -1.f - o, -1.f - o, 0.f,   // CopyToFB quad
            -1.f + o, -1.f - o, 0.f,
            -1.f + o, -1.f + o, 0.f,
            -1.f - o, -1.f + o, 0.f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    checkGLError();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    checkGLError();
    ready = true;
}

PanoRenderer::~PanoRenderer() {
    if (progPano) {
        delete progPano;
        progPano = NULL;
    }
    if (progPanoSemantic) {
        delete progPanoSemantic;
        progPanoSemantic = NULL;
    }
    if (progMapProjection) {
        delete progMapProjection;
        progMapProjection = NULL;
    }
    if (progCostmapIndex) {
        delete progCostmapIndex;
        progCostmapIndex = NULL;
    }
    if (mapProjectionVbo) {
        glDeleteBuffers(1, &mapProjectionVbo);
    }
    if (mapProjectionVao) {
        glDeleteVertexArrays(1, &mapProjectionVao);
    }
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(sizeof(textures)/sizeof(textures[0]), textures);
}

void PanoRenderer::display() {
    if (!ready) {
        return;
    }
    if (!camera) {
        logError("Camera not set in PanoRenderer::display()");
    }
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "pano");
    GLint oldViewport[4];
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    GLboolean oldDepthTest = glIsEnabled(GL_DEPTH_TEST);
    checkGLError();

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    if (renderSemantic) {
        // prepare costmap index texture
        progCostmapIndex->use();
        glViewport(0, 0, bellmanFordRenderer->getTextureWidth(), bellmanFordRenderer->getTextureHeight());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[COSTMAP_INDEX], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_NONE, 0);
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, bellmanFordRenderer->getTexture());
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        checkGLError();
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }

    glEnable(GL_DEPTH_TEST);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorCubeMap, 0);
    GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    glDepthMask(GL_TRUE);
    glViewport(0, 0, cubemapWidth, cubemapHeight);
    checkGLError();

    // Render
    // Clear view
    if (!debug) {
        if (renderSemantic) {
            // render target node, costmap, and rest separately with different colors
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            progPanoSemantic->use();

            std::vector<glm::mat4> view;
            camera->setViewProjection(progPanoSemantic->locationsMVP, view);

            // 1. render obstacles without texture (= black)
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "test-obstacle");
            for (TargetNodes::const_iterator targetIt = targetNodes.begin(); targetIt != targetNodes.end(); ++targetIt) {
                (*targetIt)->setVisible(false);
            }
            projectionPlaneNode->setVisible(false);
            glUniform1i(progPanoSemantic->locationsMaterial.hasTexture, GL_FALSE);
            scene->render(camera, &progPanoSemantic->locationsMVP, NULL, NULL, false);
            projectionPlaneNode->setVisible(true);
            glPopDebugGroup();

            // 2. render target node
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "test-target");
            for (TargetNodes::const_iterator targetIt = targetNodes.begin(); targetIt != targetNodes.end(); ++targetIt) {
                (*targetIt)->setVisible(true);
                (*targetIt)->render(view, &progPanoSemantic->locationsMVP, &progPanoSemantic->locationsMaterial, false);
            }
            glPopDebugGroup();

            // 3. render costmap index texture
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "test-costmapindex");
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, textures[COSTMAP_INDEX]);
            glUniform1i(progPanoSemantic->locationsMaterial.textureUnit, 3);
            glUniform1i(progPanoSemantic->locationsMaterial.hasTexture, true);
            projectionPlaneNode->render(view, &progPanoSemantic->locationsMVP, NULL, false);
            glPopDebugGroup();
        } else {
            // render everything
            glClearColor(0.4f, 0.4f, 0.6f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            progPano->use();
            std::vector<glm::mat4> view;
            camera->setViewProjection(progPano->locationsMVP, view);
#ifdef __ANDROID__
            //TODO hack for depth problem: render floor first
            floorNode->render(view, &progPano->locationsMVP, &progPano->locationsMaterial, progPano->hasTesselationShader);
            floorNode->setVisible(false);
#endif
            scene->render(camera, &progPano->locationsMVP, &progPano->locationsLight,
                    &progPano->locationsMaterial, progPano->hasTesselationShader);
            floorNode->setVisible(true);
        }
        checkGLError();
    }
    checkGLError();

    // Project to map
    glDisable(GL_DEPTH_TEST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_NONE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[PANO], 0);
    glViewport(0, 0, panoWidth, panoHeight);
    checkGLError();
    progMapProjection->use();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(mapProjectionVao);
    glDrawArrays(GL_TRIANGLES, 0, mapProjectionCount);
    glBindVertexArray(0);
    checkGLError();

    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (renderToWindow) {
        progShowTexture.use();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, textures[PANO]);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        checkGLError();
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
    }
    if (oldDepthTest) {
        glEnable(GL_DEPTH_TEST);
    }
    glPopDebugGroup();

}

} /* namespace gpu_coverage */
