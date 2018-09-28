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

#include <gpu_coverage/Animation.h>
#include <gpu_coverage/Channel.h>
#include <gpu_coverage/Node.h>
#include <cstdio>
#include <sstream>
#include <limits>

namespace gpu_coverage {

inline static std::string getAnimationName(const aiAnimation * const animation, const size_t id) {
    if (animation->mName.length > 0) {
        return std::string(animation->mName.C_Str());
    } else {
        std::stringstream ss;
        ss << "Animation " << id;
        return ss.str();
    }
}

Animation::Animation(Scene * const scene, const aiAnimation * const animation, const size_t id)
        : id(id), name(getAnimationName(animation, id)), duration(animation->mDuration)
{
    channels.reserve(animation->mNumChannels);
    startFrame = std::numeric_limits<size_t>::max();
    endFrame = -std::numeric_limits<size_t>::max();
    for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
        Channel * const channel = new Channel(scene, animation->mChannels[i], i);
        channels.push_back(channel);
        if (channel->getStartFrame() < startFrame) {
            startFrame = channel->getStartFrame();
        }
        if (channel->getEndFrame() > endFrame) {
            endFrame = channel->getEndFrame();
        }
        if (startFrame > endFrame) {
            startFrame = endFrame = 0;
        }
        numFrames = endFrame - startFrame;
    }
}

Animation::~Animation() {
    for (size_t i = 0; i < channels.size(); ++i) {
        delete channels[i];
    }
    channels.clear();
}

void Animation::toDot(FILE *dot) {
    fprintf(dot, "  a%zu [label=\"{%s|{Duration|%.1f s}}\", shape=\"record\"];\n", id, name.c_str(), duration);
    for (Channels::const_iterator channelIt = channels.begin(); channelIt != channels.end(); ++channelIt) {
        fprintf(dot, "  a%zu -> n%zu [style=\"dashed\"];", id, (*channelIt)->getNode()->getId());
    }
}

}  // namespace gpu_coverage
