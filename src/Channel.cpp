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

#include <gpu_coverage/Channel.h>
#include <gpu_coverage/Scene.h>
#include <gpu_coverage/Utilities.h>
#include <cstdio>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gpu_coverage {

static const double FPS = 30;

Channel::Channel(Scene * const scene, const aiNodeAnim * const channel, const size_t id)
        : id(id), node(scene->findNode(channel->mNodeName.C_Str())) {
    startFrame = std::numeric_limits<size_t>::max();
    endFrame = -std::numeric_limits<size_t>::max();
    for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i) {
        const aiVectorKey& key = channel->mPositionKeys[i];
        const size_t frame = timeToFrame(key.mTime);
        locations[frame] = glm::vec3(key.mValue[0], key.mValue[1], key.mValue[2]);
        if (frame < startFrame) {
            startFrame = frame;
        }
        if (frame > endFrame) {
            endFrame = frame;
        }
    }
    for (unsigned int i = 0; i < channel->mNumRotationKeys; ++i) {
        const aiQuatKey& key = channel->mRotationKeys[i];
        const size_t frame = timeToFrame(key.mTime);
        rotations[frame] = glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z);
        if (frame < startFrame) {
            startFrame = frame;
        }
        if (frame > endFrame) {
            endFrame = frame;
        }
    }
    for (unsigned int i = 0; i < channel->mNumScalingKeys; ++i) {
        const aiVectorKey& key = channel->mScalingKeys[i];
        const size_t frame = timeToFrame(key.mTime);
        scales[frame] = glm::vec3(key.mValue[0], key.mValue[1], key.mValue[2]);
        if (frame < startFrame) {
            startFrame = frame;
        }
        if (frame > endFrame) {
            endFrame = frame;
        }
    }
    if (node) {
        node->addAnimationChannel(this);
    }
    if (startFrame > endFrame) {
        startFrame = endFrame = 0;
    }
    numFrames = endFrame - startFrame;
    setFrame(startFrame);
}

Channel::~Channel() {
}

size_t Channel::timeToFrame(const double& time) const {
    return static_cast<unsigned int>(time * FPS + 0.5);
}

void Channel::setFrame(const size_t frame) {
    glm::vec3 loc;
    {
        const std::pair<Locations::const_iterator, Locations::const_iterator> locIt = findInterval(locations, frame);
        const Locations::const_iterator& locLow = locIt.first;
        const Locations::const_iterator& locHigh = locIt.second;
        if (locLow->first == locHigh->first) {
            loc = locLow->second;
        } else {
            const float locPos = static_cast<float>(frame - locLow->first)
                    / static_cast<float>(locHigh->first - locLow->first);
            loc = locLow->second * (1.f - locPos) + locHigh->second * locPos;
        }
    }
    glm::quat rot;
    {
        const std::pair<Rotations::const_iterator, Rotations::const_iterator> rotIt = findInterval(rotations, frame);
        const Rotations::const_iterator& rotLow = rotIt.first;
        const Rotations::const_iterator& rotHigh = rotIt.second;
        if (rotLow->first == rotHigh->first) {
            rot = rotLow->second;
        } else {
            const float rotPos = static_cast<float>(frame - rotLow->first)
                    / static_cast<float>(rotHigh->first - rotLow->first);
            rot = glm::slerp(rotLow->second, rotHigh->second, rotPos);
        }
    }
    glm::vec3 scl;
    {
        const std::pair<Scales::const_iterator, Scales::const_iterator> sclIt = findInterval(scales, frame);
        const Scales::const_iterator& sclLow = sclIt.first;
        const Scales::const_iterator& sclHigh = sclIt.second;
        if (sclLow->first == sclHigh->first) {
            scl = sclLow->second;
        } else {
            const float sclPos = static_cast<float>(frame - sclLow->first)
                    / static_cast<float>(sclHigh->first - sclLow->first);
            scl = sclLow->second * (1.f - sclPos) + sclHigh->second * sclPos;
        }
    }
    localTransform = glm::translate(glm::mat4(1.0f), loc) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), scl);
}

} /* namespace gpu_coverage */
