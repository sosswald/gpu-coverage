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
#include <gpu_coverage/RandomSearchTask.h>
#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/VisibilityRenderer.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/Config.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Channel.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <sys/mman.h>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace gpu_coverage {

RandomSearchTask::TaskSharedData *RandomSearchTask::taskSharedData = NULL;

RandomSearchTask::RandomSearchTask(Scene * const scene, const size_t threadNr, SharedData * const sharedData,
        const size_t numIterations, const size_t numArticulationConfigs, const size_t numCameraPoses)
        : AbstractTask(sharedData, threadNr), scene(scene),
                numIterations(numIterations), numArticulationConfigs(numArticulationConfigs), numCameraPoses(
                        numCameraPoses), numArticulations(scene->getChannels().size()),
                costmapRenderer(NULL), bellmanFordRenderer(NULL), visibilityRenderer(NULL)
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
    renderer = new Renderer(scene, false, true);
    if (!renderer->isReady()) {
        return;
    }

    // Create links
    costmapTexture = new Texture(bellmanFordRenderer->getTexture());
    costmapMaterial->setTexture(costmapTexture);

    std::stringstream targets(Config::getInstance().getParam<std::string>("target"));
    size_t targetI = 0;
    while (targets.good()) {
        std::string targetName;
        targets >> targetName;
        if (!targetName.empty()) {
            const Node * const target = scene->findNode(targetName);
            targetTexture[targetI] = target->getMeshes().front()->getMaterial()->getTexture()->getTextureObject();
            ++targetI;
            targetPoints.push_back(glm::vec3(glm::column(target->getWorldTransform(), 3)));
        }
    }
    numTargetTextures = targetI;

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

RandomSearchTask::~RandomSearchTask() {
    delete costmapRenderer;
    delete bellmanFordRenderer;
    delete visibilityRenderer;
    delete costmapTexture;
}

void RandomSearchTask::run() {
    if (!ready) {
        return;
    }
    const glm::vec3 worldUp(0.f, 0.f, -1.f);

    for (size_t i = 0; i < numIterations; ++i) {
        bellmanFordRenderer->setRobotPosition(taskSharedData->currentConfiguration.getCameraLocalTransform());
        std::vector<RobotSceneConfiguration *> configurations;
        configurations.reserve(numArticulationConfigs * numCameraPoses);
        for (size_t a = 0; a < numArticulationConfigs; ++a) {
            RobotSceneConfiguration rsc;
            rsc.setRandomArticulation(seed);
            rsc.applyToScene(scene);
            costmapRenderer->display();
            bellmanFordRenderer->display();

            for (size_t i = 0; i < numCameraPoses; ++i) {
                RobotSceneConfiguration *c = new RobotSceneConfiguration();
                c->set(rsc);
                c->setRandomCameraHeight(seed);
                c->setRandomCameraPosition(seed, &targetPoints);
                // c->count will be set later
                configurations.push_back(c);
                cameraNode->setLocalTransform(c->getCameraLocalTransform());
                visibilityRenderer->display();
            }
        }

        std::vector<GLuint> visibilityResults;
        visibilityRenderer->getPixelCounts(visibilityResults);
        const size_t numResults = visibilityResults.size();
        if (numResults != configurations.size()) {
            logError("Visibility results count does not match configurations count");
            return;
        }

        // scope for bestConfiguration
        {
            RobotSceneConfiguration * bestConfiguration = NULL;
            float bestEval = -std::numeric_limits<float>::max();
            for (size_t nc = 0; nc < numResults; ++nc) {
                configurations[nc]->setCount(visibilityResults[nc]);
                const float eval = configurations[nc]->getEvaluation(taskSharedData->currentConfiguration);
                if (eval > bestEval) {
                    bestEval = eval;
                    bestConfiguration = configurations[nc];
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
        for (std::vector<RobotSceneConfiguration *>::iterator it = configurations.begin(); it != configurations.end();
                ++it) {
            delete *it;
        }
        configurations.clear();

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
                std::cout << "# coverage\tcost\tgain\tevaluation\t";
                std::cout << "x\ty\tz\troll\tpitch\tyaw";
                for (size_t a = 0; a < numArticulations; ++a) {
                    std::cout << scene->getChannels()[a]->getNode()->getName() << "\t";
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
            std::cout << taskSharedData->bestConfiguration.getCount() << "\t"
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
        renderer->display();

        std::vector<GLuint> pixelCounts;
        visibilityRenderer->getPixelCounts(pixelCounts);
        if (pixelCounts[0] != taskSharedData->bestConfiguration.getCount()) {
            logError("Error: pixel count of best configuration differs: %u != %u", pixelCounts[0],
                    taskSharedData->bestConfiguration.getCount());
        }

        // Copy to original texture
        for (size_t t = 0; t < numTargetTextures; ++t) {
            glCopyImageSubData(visibilityRenderer->getTexture(t), GL_TEXTURE_2D, 0, 0, 0, 0, targetTexture[t], GL_TEXTURE_2D, 0,
                    0, 0, 0, 1024, 1024, 1);
        }

        if (threadNr == 0) {
            // Write to file for debugging
            char filename[256];
            cv::Mat mat(visibilityRenderer->getTextureHeight(), visibilityRenderer->getTextureWidth(), CV_8UC3);
            cv::Mat flip(visibilityRenderer->getTextureHeight(), visibilityRenderer->getTextureWidth(), CV_8UC3);
            glActiveTexture(GL_TEXTURE10);
            for (size_t t = 0; t < numTargetTextures; ++t) {
                snprintf(filename, sizeof(filename), "random_search_coverage_%02zu_%02zu.png", i, t);
                glBindTexture(GL_TEXTURE_2D, visibilityRenderer->getTexture(t));
                glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
                cv::flip(mat, flip, 0);
                cv::imwrite(filename, flip);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            snprintf(filename, sizeof(filename), "random_search_view_%02zu.png", i);
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

        // Wait for other threads
        pthread_barrier_wait(&sharedData->barrier);
    }
}

void RandomSearchTask::allocateSharedData() {
    if (!taskSharedData) {
        taskSharedData = (TaskSharedData *) mmap(NULL,
                sizeof(TaskSharedData) + 2 * RobotSceneConfiguration::numArticulation * sizeof(float),
                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        taskSharedData->bestConfiguration.articulation = reinterpret_cast<float*>(taskSharedData + 1);
        taskSharedData->currentConfiguration.articulation = taskSharedData->bestConfiguration.articulation
                + RobotSceneConfiguration::numArticulation;
    }
}

void RandomSearchTask::freeSharedData() {
    if (taskSharedData) {
        munmap(taskSharedData, sizeof(TaskSharedData) + 2 * RobotSceneConfiguration::numArticulation * sizeof(float));
        taskSharedData = NULL;
    }
}

} /* namespace gpu_coverage */

