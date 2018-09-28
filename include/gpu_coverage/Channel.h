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

#ifndef INCLUDE_ARTICULATION_CHANNEL_H_
#define INCLUDE_ARTICULATION_CHANNEL_H_

#include <assimp/scene.h>
#include <glm/detail/type_vec3.hpp>
#include <glm/detail/type_mat4x4.hpp>
#include <map>
#if HAS_GTEST
#include <gtest/gtest_prod.h>
#endif

namespace gpu_coverage {

// Forward declarations
class Scene;
class Node;

/**
 * @brief %Animation channel.
 *
 * Corresponds to Assimp's aiNodeAnim.
 */
class Channel {
public:
    /**
     * @brief Constructor.
     * @param[in] scene Scene that contains this channel.
     * @param[in] channel The Assimp aiNodeAnim for creating this channel.
     * @param[in] id Unique ID of this channel.
     */
    Channel(Scene * const scene, const aiNodeAnim * const channel, const size_t id);
    /**
     * @brief Destructor.
     */
    virtual ~Channel();

    /**
     * @brief Set the current frame of the animation.
     * @param[in] frame Frame number of the animation.
     *
     * Setting the frame usually changes the local transform
     * that can be retrieved afterwards using getLocalTransform().
     */
    void setFrame(const size_t frame);

    /**
     * @brief Returns the scene graph node that this camera is attached to.
     * @return The scene graph node.
     *
     * The node is set in the constructor based on the aiNodeAnim::mNodeName field.
     */
    inline Node * getNode() const {
        return node;
    }

    /**
     * @brief Frame number where this animation channel starts.
     * @return Start frame.
     */
    inline const size_t& getStartFrame() const {
        return startFrame;
    }

    /**
     * @brief Frame number where this animation channel ends.
     * @return End frame.
     */
    inline const size_t& getEndFrame() const {
        return endFrame;
    }

    /**
     * @brief Duration of this animation channel in frames.
     * @return Number of frames.
     */
    inline const size_t& getNumFrames() const {
        return numFrames;
    }

    /**
     * @brief Returns the current local transform.
     * @return Local transform.
     *
     * Call setFrame() first to set the frame and to recalculate
     * the local transform.
     */
    inline const glm::mat4& getLocalTransform() const {
        return localTransform;
    }

protected:
    const size_t id;         ///< Unique ID, see getId().
    Node * const node;       ///< The scene graph node assigned to this animation channel, see getNode().

    typedef std::map<size_t, glm::vec3> Locations;    ///< Location key frames, maps frame number to location vector.
    typedef std::map<size_t, glm::quat> Rotations;    ///< Rotation key frames, maps frame number to rotation quaternion.
    typedef std::map<size_t, glm::vec3> Scales;       ///< Scale key frames, maps frame number to scale vector.

    Locations locations;    ///< Location keyframes.
    Rotations rotations;    ///< Rotation keyframes.
    Scales scales;          ///< Scale keyframes.

    glm::mat4 localTransform;     ///< Local transform matrix of the animation channel.

    /**
     * @brief Converts time in seconds to frame number.
     * @param[in] time Time in seconds.
     * @return Corresponding frame number.
     */
    size_t timeToFrame(const double& time) const;

    size_t startFrame;           ///< Start frame of the animation.
    size_t endFrame;             ///< End frame of the animation.
    size_t numFrames;            ///< Number of frames of the animation.

    /**
     * Finds the nearest two key frames in a keyframe map.
     * @tparam M The keyframe map type
     * @param[in] map Keyframe map
     * @param[in] key Key
     * @return Pair of iterators to key frames.
     */
    template<class M>
    static std::pair<typename M::const_iterator, typename M::const_iterator> findInterval(const M& map,
            const typename M::key_type& key) {
        typename M::const_iterator a = map.begin();
        typename M::const_iterator b = map.begin();
        for (typename M::const_iterator it = map.begin(); it != map.end(); ++it) {
            if (it->first <= key)
                a = it;
            if (it->first <= key || a->first != key)
                b = it;
            if (it->first > key)
                break;
        }
        return std::make_pair(a, b);
    }

#if HAS_GTEST
    FRIEND_TEST(Channel, findInterval);
#endif
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_CHANNEL_H_ */
