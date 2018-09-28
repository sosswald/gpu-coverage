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
#include <gpu_coverage/HillclimbingTask.h>
#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/VisibilityRenderer.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/PanoRenderer.h>
#include <gpu_coverage/PanoEvalRenderer.h>
#include <gpu_coverage/Config.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Channel.h>

#include <vector>
#include <algorithm>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <fstream>
#include <sys/time.h>
#include <sys/mman.h>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace gpu_coverage {

HillclimbingTask::TaskSharedData *HillclimbingTask::taskSharedData = NULL;

HillclimbingTask::HillclimbingTask(Scene * const scene, const size_t threadNr, SharedData * const sharedData)
        : AbstractTask(sharedData, threadNr), scene(scene),
          numIterations(100),
          numArticulations(scene->getChannels().size())
{
    // Get scene nodes
    Node * const projectionPlane = scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));
    Material * const costmapMaterial = projectionPlane->getMeshes().front()->getMaterial();
    if (!costmapMaterial) {
        logError("Could not find costmap material");
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
#ifdef WRITE_VISUALIZATION_DATA
    renderer = new Renderer(scene, false, true);
    if (!renderer->isReady()) {
        return;
    }
    renderer->setCamera(scene->findCamera(Config::getInstance().getParam<std::string>("robotCamera")));
#endif
    panoRenderer = new PanoRenderer(scene, false, bellmanFordRenderer);
    if (!panoRenderer->isReady()) {
        return;
    }
    panoEvalRenderer = new PanoEvalRenderer(scene, false, WRITE_VISUALIZATION_DATA, panoRenderer, bellmanFordRenderer);

    const std::string panoCameraNames = Config::getInstance().getParam<std::string>("panoCamera");
    std::istringstream iss(panoCameraNames);
    std::string panoCameraName;
    bool isPair = false;
    CameraPanorama * first = NULL;
    CameraPanorama * second = NULL;
    while (!iss.eof()) {
        std::getline(iss, panoCameraName, ' ');
        if (panoCameraName.compare("(") == 0) {
            isPair = true;
        } else if (panoCameraName.compare(")") == 0) {
            if (first && second) {
                panoEvalRenderer->addCameraPair(first, second);
            } else {
                logError("Panorama camera pair with less than two cameras");
                return;
            }
            isPair = false;
            first = NULL;
            second = NULL;
        } else if (!panoCameraName.empty()) {
            gpu_coverage::Node * const cameraNode = scene->findNode(panoCameraName);
            if (!cameraNode) {
                logError("Could not find panorama camera %s", panoCameraName.c_str());
                return;
            }
            CameraPanorama * const panoCam = scene->makePanoramaCamera(cameraNode);
            scene->makePanoramaCamera(scene->findNode(panoCameraName));
            if (isPair) {
                if (first == NULL) {
                    first = panoCam;
                } else if (second == NULL) {
                    second = panoCam;
                } else {
                    logError("More than two cameras in panorama camera pair");
                }
            } else {
                panoEvalRenderer->addCamera(panoCam);
            }
        }
    }

    // Create links
    costmapTexture = new Texture(bellmanFordRenderer->getTexture());
    costmapMaterial->setTexture(costmapTexture);

    std::stringstream targets(Config::getInstance().getParam<std::string>("target"));
    while (targets.good()) {
        std::string targetName;
        targets >> targetName;
        if (!targetName.empty()) {
            const Node * const target = scene->findNode(targetName);
            targetTextures.push_back(target->getMeshes().front()->getMaterial()->getTexture()->getTextureObject());
            targetNames.push_back(target->getName());
        }
    }
    if (threadNr == 0) {
        // First thread initializes current robot pose and articulation
        taskSharedData->bestEval = -std::numeric_limits<float>::max();
        taskSharedData->currentConfiguration.setArticulation(0, 0.f);
        taskSharedData->currentConfiguration.setCameraLocalTransform(cameraNode->getLocalTransform());
        taskSharedData->currentConfiguration.setCount(0);
        taskSharedData->finished = false;
    }

    ready = true;
}

HillclimbingTask::~HillclimbingTask() {
    delete costmapRenderer;
    delete bellmanFordRenderer;
    delete visibilityRenderer;
    delete costmapTexture;
}

struct Utilities {
     int a, x, y, utility;
     Utilities(const int a, const int x, const int y, const int utility) : a(a), x(x), y(y), utility(utility) {}
     bool operator<(const Utilities& other) const {
         return utility < other.utility;
     }
 };

void HillclimbingTask::run() {
    if (!ready) {
        return;
    }
    const int width = bellmanFordRenderer->getTextureWidth();
    const int height = bellmanFordRenderer->getTextureHeight();
    GLint utilitymap[width * height];
    cv::Mat utilitymapCV(height, width, CV_8UC3);
    cv::Mat flipCV(height, width, CV_8UC3);
    struct timespec curTime;
    clock_gettime(CLOCK_MONOTONIC, &curTime);
    double lastTime = static_cast<double>(curTime.tv_sec) + static_cast<double>(curTime.tv_nsec) * 1e-9;;

    for (size_t i = 0; i < numIterations; ++i) {
        bellmanFordRenderer->setRobotPosition(taskSharedData->currentConfiguration.getCameraLocalTransform());
        std::vector<RobotSceneConfiguration *> configurations1;
        configurations1.reserve(numArticulations);
        GLint highestUtility = 100000;
        std::vector<Utilities> allUtilities;
        for (size_t a = 0; a < numArticulations; ++a) {
            configurations1.push_back(new RobotSceneConfiguration());
            for (size_t b = 0; b < numArticulations; ++b) {
                configurations1[a]->setArticulation(b, a == b ? 1. : 0.);
            }
            configurations1[a]->applyToScene(scene);
            costmapRenderer->display();
            bellmanFordRenderer->display();
            panoEvalRenderer->display();

            glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
            glBindTexture(GL_TEXTURE_2D, panoEvalRenderer->getUtilityMap());
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, utilitymap);
            {
                for (int y = 0, i = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x, ++i) {
                        if (utilitymap[i] < 100000) {
                            allUtilities.push_back(Utilities(a, x, y, utilitymap[i]));
                        }
                    }
                }
            }
#ifdef WRITE_VISUALIZATION_DATA
            char filename[256];
            snprintf(filename, sizeof(filename), "hillclimbing_utility_%02zu_%02zu.png", i, a);
            glBindTexture(GL_TEXTURE_2D, panoEvalRenderer->getUtilityMapVisual());
            //glBindTexture(GL_TEXTURE_2D, costmapRenderer->getVisualTexture());
            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, utilitymapCV.data);
            cv::flip(utilitymapCV, flipCV, 0);
            cv::imwrite(filename, flipCV);
#endif
        }

        sort(allUtilities.begin(), allUtilities.end());
        std::vector<RobotSceneConfiguration *> configurations2;
        std::vector<GLuint> visibilityResults;
        for (size_t u = 0; u < 10; ++u) {
            for (float pitch = glm::radians(20.); pitch <= glm::radians(160.); pitch += glm::radians(20.) ) {
                for (float yaw = 0; yaw < glm::radians(360.); yaw += glm::radians(20.)) {
                    RobotSceneConfiguration *c = new RobotSceneConfiguration();
                    const float x = -5.36 + static_cast<float>(std::min(width - 1, std::max(1, allUtilities[u].x))) / static_cast<float>(width)  * 10.98;  // TODO
                    const float y = -6.49 + static_cast<float>(std::min(height - 1, std::max(1, allUtilities[u].y))) / static_cast<float>(height) * 10.98;  // TODO
                    static const glm::vec3 worldUp(0.f, 0.f, -1.f);
                    const glm::vec3 eye(x, y, 1.4);  // TODO
                    c->set(*configurations1[allUtilities[u].a]);
                    const glm::vec3 look = glm::vec3(sin(pitch) * cos(yaw), sin(pitch) * sin(yaw), cos(pitch));
                    const glm::vec3 right(glm::cross(look, worldUp));
                    const glm::vec3 up(glm::cross(look, right));
                    c->setCameraLocalTransform(glm::inverse(glm::lookAt(eye, eye + look, up)));
                    c->applyToScene(scene);
                    configurations2.push_back(c);
                    cameraNode->setLocalTransform(c->getCameraLocalTransform());
                    visibilityRenderer->display();
#ifdef WRITE_VISUALIZATION_DATA
                    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
                    std::vector<GLuint> v;
                    visibilityRenderer->getPixelCounts(v);
                    if (i == 0 && u == 0 && int(glm::degrees(pitch) + 0.5) == 100 && int(glm::degrees(yaw) + 0.5) == 20) {
                        v[0] = 0;
                    }

                    if (v.size() != 1) {
                        logError("Unexpected pixel count");
                    }
                    visibilityResults.push_back(v[0]);
                    /*if (i == 0 && u == 0 && int(glm::degrees(pitch) + 0.5) == 100 && int(glm::degrees(yaw) + 0.5) == 20) {
                        renderer->display();
                        char filename[256];
                        snprintf(filename, sizeof(filename), "hillclimbing_yaw_%02zu_%zu_%.0f_%.0f_%d.png", i, u, glm::degrees(pitch), glm::degrees(yaw), v[0]);
                        {
                            cv::Mat mat(renderer->getTextureHeight(), renderer->getTextureWidth(), CV_8UC3);
                            glActiveTexture(GL_TEXTURE10);
                            glBindTexture(GL_TEXTURE_2D, renderer->getTexture());
                            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
                            glBindTexture(GL_TEXTURE_2D, 0);
                            cv::Mat flip(renderer->getTextureHeight(), renderer->getTextureWidth(), CV_8UC3);
                            cv::flip(mat, flip, 0);
                            cv::imwrite(filename, flip);
                        }
                        for (size_t vis = 0; vis < targetNames.size(); ++vis) {
                            snprintf(filename, sizeof(filename), "hillclimbing_yaw_%02zu_%zu_%.0f_%.0f_%d_%s.png", i, u, glm::degrees(pitch), glm::degrees(yaw), v[0], targetNames[vis].c_str());
                            {
                                cv::Mat mat(visibilityRenderer->getTextureHeight(), visibilityRenderer->getTextureWidth(), CV_8UC3);
                                glActiveTexture(GL_TEXTURE10);
                                glBindTexture(GL_TEXTURE_2D, visibilityRenderer->getTexture(vis));
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
                                glBindTexture(GL_TEXTURE_2D, 0);
                                cv::Mat flip(mat.rows, mat.cols, CV_8UC3);
                                cv::flip(mat, flip, 0);
                                cv::imwrite(filename, flip);
                            }
                            snprintf(filename, sizeof(filename), "hillclimbing_yaw_%02zu_%zu_%.0f_%.0f_%d_%s_render.png", i, u, glm::degrees(pitch), glm::degrees(yaw), v[0], targetNames[vis].c_str());
                            {
                                cv::Mat mat(960, 1280, CV_8UC3);
                                glActiveTexture(GL_TEXTURE10);
                                glBindTexture(GL_TEXTURE_2D, visibilityRenderer->textures[VisibilityRenderer::COLOR]);
                                glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
                                glBindTexture(GL_TEXTURE_2D, 0);
                                cv::Mat flip(mat.rows, mat.cols, CV_8UC4);
                                cv::flip(mat, flip, 0);
                                cv::imwrite(filename, flip);
                            }
                        }
                        checkGLError();
                        exit(240);
                    }*/
#endif
                }
            }
        }

#ifndef WRITE_VISUALIZATION_DATA
        visibilityRenderer->getPixelCounts(visibilityResults);
#endif
        const size_t numResults = visibilityResults.size();
        if (numResults != configurations2.size()) {
            logError("Visibility results count does not match configurations count");
            return;
        }

        // scope for bestConfiguration
        {
            RobotSceneConfiguration * bestConfiguration = NULL;
            float bestEval = -std::numeric_limits<float>::max();
            for (size_t nc = 0; nc < numResults; ++nc) {
                configurations2[nc]->setCount(visibilityResults[nc]);
                const float eval = configurations2[nc]->getEvaluation(taskSharedData->currentConfiguration);
                if (eval > bestEval) {
                    bestEval = eval;
                    bestConfiguration = configurations2[nc];
                }
            }

            // Publish best result
            pthread_mutex_lock(&sharedData->mutex);
            if (bestEval > taskSharedData->bestEval) {
                taskSharedData->bestConfiguration.set(*bestConfiguration);
                taskSharedData->bestEval = bestEval;
            }
            pthread_mutex_unlock(&sharedData->mutex);
        }

        // Delete all configurations
        for (std::vector<RobotSceneConfiguration *>::iterator it = configurations1.begin(); it != configurations1.end();
                ++it) {
            delete *it;
        }
        configurations1.clear();
        for (std::vector<RobotSceneConfiguration *>::iterator it = configurations2.begin(); it != configurations2.end();
                ++it) {
            delete *it;
        }
        configurations2.clear();

        // Wait for other threads
        pthread_barrier_wait(&sharedData->barrier);

        // Set best configuration of all threads to the new current configuration
        if (threadNr == 0) {
            // Debug output
            static bool isFirst = true;
            if (isFirst) {
                // print header and initial pose
                cameraNode->setLocalTransform(taskSharedData->currentConfiguration.getCameraLocalTransform());
                const glm::mat4 worldTransform(cameraNode->getWorldTransform());
                const glm::vec3 position(glm::column(worldTransform, 3));
                const glm::vec3 scaling = glm::vec3(glm::length(glm::column(worldTransform, 0)),
                        glm::length(glm::column(worldTransform, 1)), glm::length(glm::column(worldTransform, 2)));
                const glm::mat3 unscale = glm::mat3(
                        glm::vec3(glm::column(worldTransform, 0)) / scaling[0],
                        glm::vec3(glm::column(worldTransform, 1)) / scaling[1],
                        glm::vec3(glm::column(worldTransform, 2)) / scaling[2]);
                const glm::quat rotation(unscale);
                float roll = glm::roll(rotation);
                float pitch = glm::pitch(rotation);
                float yaw = glm::yaw(rotation);
                if (M_PI_2 - fabs(roll) < 1e-3) {
                    roll = 0.f;
                    pitch = M_PI_2 - pitch;
                }
                std::cout << "#id\tshow\tcoverage\tcost\tgain\tevaluation\t";
                std::cout << "x\ty\tz\troll\tpitch\tyaw";
                for (size_t a = 0; a < numArticulations; ++a) {
                    std::cout << "\t" << scene->getChannels()[a]->getNode()->getName();
                }
                std::cout << std::endl;
                std::cout << "0\t0\t0\t0\t"
                        << position.x << "\t" << position.y << "\t" << position.z << "\t"
                        << roll << "\t" << pitch << "\t" << yaw;
                for (size_t a = 0; a < numArticulations; ++a) {
                   std::cout << "\t" << taskSharedData->currentConfiguration.getArticulation(a);
                }
                std::cout << std::endl;

                isFirst = false;
            }
            cameraNode->setLocalTransform(taskSharedData->bestConfiguration.getCameraLocalTransform());
            const glm::mat4 worldTransform(cameraNode->getWorldTransform());
            const glm::vec3 position(glm::column(worldTransform, 3));
            const glm::vec3 scaling = glm::vec3(glm::length(glm::column(worldTransform, 0)),
                    glm::length(glm::column(worldTransform, 1)), glm::length(glm::column(worldTransform, 2)));
            const glm::mat3 unscale = glm::mat3(
                    glm::vec3(glm::column(worldTransform, 0)) / scaling[0],
                    glm::vec3(glm::column(worldTransform, 1)) / scaling[1],
                    glm::vec3(glm::column(worldTransform, 2)) / scaling[2]);
            const glm::quat rotation(unscale);
            float roll = glm::roll(rotation);
            float pitch = glm::pitch(rotation);
            float yaw = glm::yaw(rotation);
            if (M_PI_2 - fabs(roll) < 1e-3) {
                roll = 0.f;
                pitch = M_PI_2 - pitch;
            }
            const float cost = taskSharedData->bestConfiguration.getCost(taskSharedData->currentConfiguration);
            const float gain = taskSharedData->bestConfiguration.getGain(taskSharedData->currentConfiguration);
            const float eval = taskSharedData->bestConfiguration.getEvaluation(taskSharedData->currentConfiguration);
            std::cout << i << "\ttrue\t" << taskSharedData->bestConfiguration.getCount() << "\t"
                    << cost << "\t" << gain << "\t" << eval << "\t";
            std::cout << position.x << "\t" << position.y << "\t" << position.z << "\t"
                    << roll << "\t" << pitch << "\t" << yaw;
            for (size_t t = 0; t < numArticulations; ++t) {
                std::cout << "\t" << taskSharedData->bestConfiguration.getArticulation(t);
            }
            std::cout << std::endl;

            taskSharedData->currentConfiguration.set(taskSharedData->bestConfiguration);
            if (taskSharedData->bestEval < 0.f) {
                taskSharedData->finished = true;
            }
            taskSharedData->bestEval = -std::numeric_limits<float>::max();
        }

        // Wait for other threads
        pthread_barrier_wait(&sharedData->barrier);

        if (taskSharedData->finished) {
            break;
        }

        // Re-render best visibility texture
        cameraNode->setLocalTransform(taskSharedData->currentConfiguration.getCameraLocalTransform());
        taskSharedData->currentConfiguration.applyToScene(scene);
        costmapRenderer->display();
        bellmanFordRenderer->display();
        visibilityRenderer->display();

        std::vector<GLuint> pixelCounts;
        visibilityRenderer->getPixelCounts(pixelCounts);
        if (pixelCounts[0] != taskSharedData->bestConfiguration.getCount()) {
            logError("Error: pixel count of best configuration differs: %u != %u", pixelCounts[0],
                    taskSharedData->bestConfiguration.getCount());
        }

        // Copy to original texture
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        for (size_t t = 0; t < targetTextures.size(); ++t) {
            glCopyImageSubData(visibilityRenderer->getTexture(t), GL_TEXTURE_2D, 0, 0, 0, 0, targetTextures[t], GL_TEXTURE_2D, 0,
                    0, 0, 0, 1024, 1024, 1);
        }

#ifdef WRITE_VISUALIZATION_DATA
        if (threadNr == 0) {
            // Write to file for debugging
            char filename[256];
            cv::Mat mat(visibilityRenderer->getTextureHeight(), visibilityRenderer->getTextureWidth(), CV_8UC3);
            cv::Mat flip(visibilityRenderer->getTextureHeight(), visibilityRenderer->getTextureWidth(), CV_8UC3);
            glActiveTexture(GL_TEXTURE10);
            for (size_t t = 0; t < targetTextures.size(); ++t) {
                snprintf(filename, sizeof(filename), "hillclimbing_coverage_%02zu_%s.png", i, targetNames[t].c_str());
                glBindTexture(GL_TEXTURE_2D, visibilityRenderer->getTexture(t));
                glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
                cv::flip(mat, flip, 0);
                cv::imwrite(filename, flip);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            renderer->display();

            snprintf(filename, sizeof(filename), "hillclimbing_view_%02zu.png", i);
            {
                cv::Mat mat(renderer->getTextureHeight(), renderer->getTextureWidth(), CV_8UC3);
                glActiveTexture(GL_TEXTURE10);
                glBindTexture(GL_TEXTURE_2D, renderer->getTexture());
                glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
                glBindTexture(GL_TEXTURE_2D, 0);
                cv::Mat flip(renderer->getTextureHeight(), renderer->getTextureWidth(), CV_8UC3);
                cv::flip(mat, flip, 0);
                cv::imwrite(filename, flip);
            }
        }
#endif

        // Wait for other threads
        pthread_barrier_wait(&sharedData->barrier);

        clock_gettime(CLOCK_MONOTONIC, &curTime);
        double thisTime = static_cast<double>(curTime.tv_sec) + static_cast<double>(curTime.tv_nsec) * 1e-9;
        std::cout << (thisTime - lastTime) << std::endl;
        lastTime = thisTime;
    }

}

void HillclimbingTask::allocateSharedData() {
    if (!taskSharedData) {
        taskSharedData = (TaskSharedData *) mmap(NULL,
                sizeof(TaskSharedData) + 2 * RobotSceneConfiguration::numArticulation * sizeof(float),
                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        taskSharedData->bestConfiguration.articulation = reinterpret_cast<float*>(taskSharedData + 1);
        taskSharedData->currentConfiguration.articulation = taskSharedData->bestConfiguration.articulation
                + RobotSceneConfiguration::numArticulation;
    }
}

void HillclimbingTask::freeSharedData() {
    if (taskSharedData) {
        munmap(taskSharedData, sizeof(TaskSharedData) + 2 * RobotSceneConfiguration::numArticulation * sizeof(float));
        taskSharedData = NULL;
    }
}

} /* namespace gpu_coverage */
