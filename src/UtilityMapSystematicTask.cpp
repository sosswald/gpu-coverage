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
#include <gpu_coverage/BellmanFordXfbRenderer.h>
#include <gpu_coverage/UtilityMapSystematicTask.h>
#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/VisibilityRenderer.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/Config.h>
#include <gpu_coverage/Channel.h>
#include <gpu_coverage/Utilities.h>

#include <sys/mman.h>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace gpu_coverage {

UtilityMapSystematicTask::UtilityMapSystematicTask(Scene * const scene, const size_t threadNr,
        SharedData * const sharedData, const size_t& startFrame, const size_t& endFrame)
: AbstractTask(sharedData, threadNr), scene(scene),
  costmapRenderer(NULL), bellmanFordRenderer(NULL), visibilityRenderer(NULL), renderer(NULL),
  startFrame(startFrame), endFrame(endFrame),
  outputVideo(NULL), debug(true)
{
    // Get scene nodes
    Node * const projectionPlane = scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));
    if (!projectionPlane) {
        logError("Could not find projection plane");
        return;
    }
    cameraNode = scene->findNode(Config::getInstance().getParam<std::string>("robotCamera"));

    // Create renderers
    costmapRenderer = new CostMapRenderer(scene, projectionPlane, false, false);
    if (!costmapRenderer->isReady()) {
        return;
    }
    bellmanFordRenderer = new BellmanFordXfbRenderer(scene, costmapRenderer, false, false);
    if (!bellmanFordRenderer->isReady()) {
        return;
    }
    visibilityRenderer = new VisibilityRenderer(scene, false, true);
    if (!visibilityRenderer->isReady()) {
        return;
    }
    if (debug) {
        renderer = new Renderer(scene, false, true);
        if (!renderer->isReady()) {
            return;
        }
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "/tmp/output_%zu.avi", threadNr);
    outputVideo = new cv::VideoWriter(filename, CV_FOURCC('D', 'I', 'V', 'X'), 30.,
            cv::Size(bellmanFordRenderer->getTextureWidth(), bellmanFordRenderer->getTextureHeight()), true);
    if (!outputVideo->isOpened()) {
        logError("Could not open output video file %s", filename);
        return;
    }

    ready = true;
}

UtilityMapSystematicTask::~UtilityMapSystematicTask()
{
    if (costmapRenderer) {
        delete costmapRenderer;
    }
    if (bellmanFordRenderer) {
        delete bellmanFordRenderer;
    }
    if (visibilityRenderer) {
        delete visibilityRenderer;
    }
    if (renderer) {
        delete renderer;
    }
    if (outputVideo) {
        outputVideo->release();
    }
}

void UtilityMapSystematicTask::run() {
    if (!ready) {
        return;
    }
    const int& height = bellmanFordRenderer->getTextureHeight();
    const int& width = bellmanFordRenderer->getTextureWidth();
    const GLint minUtility = -25600;
    const GLint maxUtility = 100;
    // BGR!
    const cv::Vec3b LOW(255, 0, 204);   // magenta
    const cv::Vec3b MID(255, 0, 0);     // blue
    const cv::Vec3b HIGH(255, 255, 0);  // cyan
    const cv::Vec3b INSCRIBED(0, 0, 0);  // black
    const cv::Vec3b OBSTACLE(128, 128, 128);  // gray

    const float minX = -1.883;
    const float maxX =  1.733;
    const float minY = -2.177;
    const float maxY =  1.440;
    const float z = 1.074f;

    const float dx = (maxX - minX) / width;
    const float dy = (maxY - minY) / width;

    for (size_t frame = startFrame; frame < endFrame; ++frame) {
        scene->getChannels()[0]->setFrame(frame);
        costmapRenderer->display();
        bellmanFordRenderer->display();

        GLuint costmap[width * height] ;
        glBindTexture(GL_TEXTURE_2D, bellmanFordRenderer->getTexture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, costmap);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                static const glm::vec3 worldUp(0.f, 0.f, -1.f);
                const glm::vec3 eye(minX + x * dx, minY + y * dy, z);
                const glm::vec3 look(glm::normalize(eye));
                const glm::vec3 right(glm::cross(look, worldUp));
                const glm::vec3 up(glm::cross(look, right));
                cameraNode->setLocalTransform(glm::inverse(glm::lookAt(eye, glm::vec3(0., 0., 0.), up)));

                visibilityRenderer->display();

                //TODO remove debug code
                /*if (debug) {
                    {
                        cv::Mat mat(visibilityRenderer->getTextureHeight(), visibilityRenderer->getTextureWidth(), CV_8UC3);
                        cv::Mat flip(visibilityRenderer->getTextureHeight(), visibilityRenderer->getTextureWidth(), CV_8UC3);
                        glBindTexture(GL_TEXTURE_2D, visibilityRenderer->getTexture());
                        glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
                        cv::flip(mat, flip, 0);
                        char filename[256];
                        snprintf(filename, sizeof(filename), "/tmp/utility/visibility-%d-%d.png", x, y);
                        cv::imwrite(filename, flip);
                    }

                    {
                        renderer->setFrame(frame);
                        renderer->display();
                        cv::Mat mat(renderer->getTextureHeight(), renderer->getTextureWidth(), CV_8UC3);
                        cv::Mat flip(renderer->getTextureHeight(), renderer->getTextureWidth(), CV_8UC3);
                        glBindTexture(GL_TEXTURE_2D, renderer->getTexture());
                        glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
                        cv::flip(mat, flip, 0);
                        char filename[256];
                        snprintf(filename, sizeof(filename), "/tmp/utility/renderer-%d-%d.png", x, y);
                        cv::imwrite(filename, flip);
                    }
                }*/
            }
        }
        std::vector<GLuint> visibility;
        visibilityRenderer->getPixelCounts(visibility);
        if (visibility.size() != static_cast<size_t>(width * height)) {
            pthread_mutex_lock(&sharedData->mutex);
            logError("Expected %d visibility counts, but got %zu", width * height, visibility.size());
            pthread_mutex_unlock(&sharedData->mutex);
            return;
        }
        cv::Mat mat(height, width, CV_8UC3);
        for (int y = 0, i = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x, ++i) {
                const GLuint gain = visibility[i];
                const GLuint cost = costmap[i];
                const GLint utility = cost > 5000000 ? -cost : (gain / 10 - cost);
                float value = (static_cast<float>(utility) - static_cast<float>(minUtility))
                        / static_cast<float>(maxUtility - minUtility);
                if (value < 0.) {
                    value = 0.;
                }
                if (value > 1.) {
                    value = 1.;
                }
                mat.at<cv::Vec3b>(y, x) = cost > 15000000 ? OBSTACLE : (
                                          cost > 5000000 ? INSCRIBED : (
                                          value < 0.5 ? LOW * (1. - 2. * value) + MID * (2. * value)
                                                      : MID * (1. - 2. * (value - 0.5)) + HIGH * (2. * (value - 0.5))));
                if (debug) {
                    static FILE *debugoutput = fopen("/tmp/utility-map-debug.txt", "w");
                    fprintf(debugoutput, "%zu\t%d\t%d\t%d\t%d\t%d\t%.5f\n", frame, x, y, gain, cost, utility, value);
                }
            }
        }
        cv::Mat flip(height, width, CV_8UC3);
        cv::flip(mat, flip, 0);
        outputVideo->write(flip);
        pthread_mutex_lock(&sharedData->mutex);
        printf("[%zu] %.0f%% done\n", threadNr, static_cast<double>(frame - startFrame) / static_cast<float>(endFrame - startFrame) * 100.f);
        pthread_mutex_unlock(&sharedData->mutex);

        char filename[256];
        snprintf(filename, sizeof(filename), "/tmp/utility-%03zu.png", frame);
        cv::imwrite(filename, flip);

        if (debug) {
            break;
        }
    }
}


} /* namespace gpu_coverage */
