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

#include <gpu_coverage/RobotSceneConfiguration.h>
#include <gpu_coverage/Config.h>
#include <gpu_coverage/Channel.h>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace gpu_coverage {

float RobotSceneConfiguration::costCameraHeightChange;
float RobotSceneConfiguration::costDistance;
float RobotSceneConfiguration::gainFactor;
float RobotSceneConfiguration::minCameraHeight;
float RobotSceneConfiguration::maxCameraHeight;
size_t RobotSceneConfiguration::numArticulation = 0;
RobotSceneConfiguration::ArticulationCost *RobotSceneConfiguration::costArticulation = NULL;

RobotSceneConfiguration::RobotSceneConfiguration()
        : count(0) {
    articulation = (float *) calloc(numArticulation, sizeof(float));
    for (size_t a = 0; a < numArticulation; ++a) {
        articulation[a] = 0.f;
    }
}

RobotSceneConfiguration::RobotSceneConfiguration(const RobotSceneConfiguration& other) {
    articulation = (float *) calloc(numArticulation, sizeof(float));
    set(other);
}

RobotSceneConfiguration::~RobotSceneConfiguration() {
}

float RobotSceneConfiguration::getCost(RobotSceneConfiguration& previousConfig) const {
    float costs = 0.f;
    // distance
    const float distance = hypot(cameraPosition.x - previousConfig.cameraPosition.x,
            cameraPosition.y - previousConfig.cameraPosition.y);
    costs += costDistance * distance;
    // articulation
    for (size_t a = 0; a < numArticulation; ++a) {
        const float delta = fabs(articulation[a] - previousConfig.articulation[a]);
        if (delta > 1e-4) {
            costs += costArticulation[a].constant + costArticulation[a].linear * delta;
        }
    }
    // height change
    if (fabs(previousConfig.cameraPosition.z - cameraPosition.z) > 1e-4) {
        costs += costCameraHeightChange;
    }
    return costs;
}

float RobotSceneConfiguration::getGain(RobotSceneConfiguration& previousConfig) const {
    return gainFactor * (static_cast<int>(count) - static_cast<int>(previousConfig.count));
}

void RobotSceneConfiguration::setRandomArticulation(unsigned int& seed) {
    size_t which = rand_r(&seed) * numArticulation / RAND_MAX;
    for (size_t a = 0; a < numArticulation; ++a) {
        setArticulation(a, a == which ? static_cast<float>(rand_r(&seed)) / RAND_MAX : 0);
    }
}

void RobotSceneConfiguration::setRandomCameraHeight(unsigned int& seed) {
    if (maxCameraHeight - minCameraHeight > 1e-4) {
        setCameraHeight(
                minCameraHeight + static_cast<float>(rand_r(&seed)) / RAND_MAX * (maxCameraHeight - minCameraHeight));
    } else {
        setCameraHeight(minCameraHeight);
    }
}

void RobotSceneConfiguration::setRandomCameraPosition(unsigned int& seed, const std::vector<glm::vec3> * const targetPoints) {
    const float x = (static_cast<float>(rand_r(&seed)) / RAND_MAX - 0.5f) * 10.8f; // TODO get sampling range from projection plane
    const float y = (static_cast<float>(rand_r(&seed)) / RAND_MAX - 0.5f) * 10.8f;
    static const glm::vec3 worldUp(0.f, 0.f, -1.f);
    const glm::vec3 eye(x, y, cameraPosition.z);
    const glm::vec3 look(glm::normalize(eye - targetPoints->at(rand_r(&seed) * targetPoints->size() / RAND_MAX)));
    const glm::vec3 right(glm::cross(look, worldUp));
    const glm::vec3 up(glm::cross(look, right));
    setCameraLocalTransform(glm::inverse(glm::lookAt(eye, glm::vec3(0., 0., 0.), up)));
}

void RobotSceneConfiguration::setCameraHeight(const float& value) {
    cameraLocalTransform[3][2] = value;
    setCameraLocalTransform(cameraLocalTransform);
}

void RobotSceneConfiguration::setCameraLocalTransform(const glm::mat4x4& transform) {
    cameraLocalTransform = transform;
    cameraPosition = glm::vec3(glm::column(cameraLocalTransform, 3));
}

void RobotSceneConfiguration::applyToScene(Scene * const scene) const {
    const Scene::Channels& channels = scene->getChannels();
    for (size_t i = 0; i < channels.size(); ++i) {
        channels[i]->setFrame(channels[i]->getStartFrame() + articulation[i] * channels[i]->getNumFrames());
    }
}

void RobotSceneConfiguration::loadCosts() {
    costCameraHeightChange = Config::getInstance().getParam<float>("costCameraHeightChange");
    costDistance = Config::getInstance().getParam<float>("costDistance");
    numArticulation = 20;
    costArticulation = (ArticulationCost *) calloc(numArticulation, sizeof(ArticulationCost));
    for (size_t i = 0; i < numArticulation; ++i) {
        costArticulation[i] = ArticulationCost(0.5f, 1.0f); // todo: move to config file
    }
    minCameraHeight = Config::getInstance().getParam<float>("minCameraHeight");
    maxCameraHeight = Config::getInstance().getParam<float>("maxCameraHeight");
    gainFactor = Config::getInstance().getParam<float>("gainFactor");
}

} /* namespace gpu_coverage */
