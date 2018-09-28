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

#ifndef INCLUDE_ARTICULATION_ANIMATION_H_
#define INCLUDE_ARTICULATION_ANIMATION_H_

#include <assimp/scene.h>
#include <vector>

namespace gpu_coverage {

// Forward declarations
class Scene;
class Node;
class Channel;

/**
 * @brief Represents the animation of a scene object.
 *
 * This class is our equivalent of Assimp's aiAnimation class.
 */
class Animation {
public:
    /**
     * @brief Constructor.
     * @param[in] scene Scene that contains this animation.
     * @param[in] animation The Assimp aiAnimation for creating this animation.
     * @param[in] id Unique ID of this animation.
     */
    Animation(Scene * const scene, const aiAnimation * const animation, const size_t id);

    /**
     * @brief Destructor.
     */
    virtual ~Animation();

    typedef std::vector<Channel *> Channels;

    /**
     * @brief Write Graphviz %Dot node representing this animation to file for debugging.
     * @param[in] dot Output %Dot file.
     */
    void toDot(FILE *dot);

    /**
     * @brief Returns the unique ID of this animation.
     * @return ID.
     */
    const size_t& getId() const {
        return id;
    }

    /**
     * @brief Returns the name of this animation for logging.
     * @return Name of this animation.
     *
     * If available, the name included in the input file read by Assimp is used,
     * otherwise a generic string is constructed.
     */
    const std::string& getName() const {
        return name;
    }

    /**
     * @brief Frame number where this animation starts.
     * @return Start frame.
     */
    inline const size_t& getStartFrame() const {
        return startFrame;
    }

    /**
     * @brief Frame number where this animation ends.
     * @return End frame.
     */
    inline const size_t& getEndFrame() const {
        return endFrame;
    }

    /**
     * @brief Duration of this animation in frames.
     * @return Number of frames.
     */
    inline const size_t& getNumFrames() const {
        return numFrames;
    }

    /**
     * @brief Vector of Channel pointers that this animation influences.
     * @return Channels.
     */
    inline const Channels& getChannels() const {
        return channels;
    }

    /**
     * @brief Duration of this animation in seconds.
     * @return Duration in seconds.
     */
    inline const double& getDuration() const {
        return duration;
    }

protected:
    const size_t id;             ///< Unique ID, see getId().
    const std::string name;      ///< Name of this animation, see getName().
    const double duration;       ///< Duration of the animation in seconds.
    size_t startFrame;           ///< Start frame of the animation.
    size_t endFrame;             ///< End frame of the animation.
    size_t numFrames;            ///< Number of frames of the animation.
    Channels channels;           ///< Channels manipulated by this animation.
};

}

#endif /* INCLUDE_ARTICULATION_ANIMATION_H_ */
