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

#ifndef INCLUDE_ARTICULATION_ROBOTSCENECONFIGURATION_H_
#define INCLUDE_ARTICULATION_ROBOTSCENECONFIGURATION_H_

#include <gpu_coverage/AbstractRenderer.h>
#include <gpu_coverage/Node.h>
#include <glm/detail/type_mat4x4.hpp>
#include <stdexcept>
#include <list>

namespace gpu_coverage {

/**
 * @brief Represents a combined configuration of the robot and the articulation objects.
 */
class RobotSceneConfiguration {
public:
    /**
     * @brief Constructor.
     */
    RobotSceneConfiguration();
    /**
     * @brief Copy constructor.
     * @param[in] other RobotSceneConfiguration to copy data from.
     */
    explicit RobotSceneConfiguration(const RobotSceneConfiguration& other);
    /**
     * @brief Destructor.
     */
    virtual ~RobotSceneConfiguration();

    /**
     * @brief Computes the cost for transitioning from a previous configuration.
     * @param[in] previousConfig The previous configuration.
     * @return Cost value.
     *
     * The cost consists of costs for moving the camera from the old position to the new position
     * and the costs for changing the articulated objects' configurations. See loadCosts()
     * for loading the cost coefficients.
     */
    float getCost(RobotSceneConfiguration& previousConfig) const;

    /**
     * @brief Calculates the information gain achieved by moving to the new configuration.
     * @param[in] previousConfig The previous configuration.
     * @return Information gain value.
     *
     * Before calling this method, the setCount() method needs to be called
     * with the number of observed pixels for both the previous and current
     * configuration.
     */
    float getGain(RobotSceneConfiguration& previousConfig) const;

    /**
     * @brief Returns the evaluation of the current config.
     * @param[in] previousConfig The previous config.
     * @return Evaluation value.
     *
     * The evaluation value is the information gain minus the costs for
     * getting from the previous configuration to the current configuration.
     *
     * See getGain() and getCosts() for the individual components.
     */
    inline float getEvaluation(RobotSceneConfiguration& previousConfig) const {
        return getGain(previousConfig) - getCost(previousConfig);
    }

    /**
     * @brief Copy the configuration over from another configuration.
     * @param[in] other The other configuration to copy from.
     */
    void set(const RobotSceneConfiguration& other) {
        cameraLocalTransform = other.cameraLocalTransform;
        cameraPosition = other.cameraPosition;
        // numArticulation is the same for all instances, hence the
        // array does not need to be reallocated or resized.
        memcpy(articulation, other.articulation, numArticulation * sizeof(float));
        count = other.count;
    }

    /**
     * @brief Sets a random articulated object to a random configuration and the other articulated objects to the zero configuration.
     * @param[in] seed Random seed.
     */
    void setRandomArticulation(unsigned int& seed);

    /**
     * @brief Sets the camera height to a random value within the feasible range.
     * @param[in] seed Random seed.
     */
    void setRandomCameraHeight(unsigned int& seed);

    /**
     * @brief Sets the camera pose to a random pose with the camera facing a random point from targetPoints.
     * @param[in] seed Random seed.
     * @param[in] targetPoints A list of 3D points in world coordinates
     */
    void setRandomCameraPosition(unsigned int& seed, const std::vector<glm::vec3> * const targetPoints);

    /**
     * @brief Sets the camera height.
     * @param[in] value The new camera height above the ground.
     */
    void setCameraHeight(const float& value);

    /**
     * @brief Sets the camera to a given pose.
     * @param[in] value The camera pose as a 4x4 homogeneous transformation matrix in world coordinates.
     */
    void setCameraLocalTransform(const glm::mat4x4& cameraTransform);

    /**
     * @brief Returns the camera pose.
     * @return The camera pose as a 4x4 homogeneous transformation matrix in world coordinates.
     */
    inline const glm::mat4x4& getCameraLocalTransform() const {
        return cameraLocalTransform;
    }

    /**
     * @brief Applies the articulation configuration to the scene and re-computes the scene graph transformations.
     * @param[in] scene The scene to which to apply the articulation configuration.
     */
    void applyToScene(Scene * const scene) const;

    /**
     * @brief Returns the current articulation pose for a given articulated object.
     * @param[in] handle The index of the articulated object.
     * @return The articulation value between 0.0 and 1.0.
     * @exception std::out_of_range The index of the articulated object is out of range.
     */
    inline float getArticulation(const size_t& handle) const {
#ifdef DEBUG
        if (handle >= numArticulation) {
            throw std::out_of_range("RobotSceneConfiguration::getArticulation index out of range");
        }
#endif
        return articulation[handle];
    }

    /**
     * @brief Sets the current articulation pose for a given articulated object.
     * @param[in] handle The index of the articulated object.
     * @param[in] The articulation value between 0.0 and 1.0.
     * @exception std::out_of_range The index of the articulated object is out of range.
     */
    inline void setArticulation(const size_t& handle, const float& value) {
#ifdef DEBUG
        if (handle >= numArticulation) {
            throw std::out_of_range("RobotSceneConfiguration::setArticulation index out of range");
        }
#endif
        articulation[handle] = value;
    }

    /**
     * @brief Returns the total number of observed pixels so far.
     * @return Number of observed pixels.
     * @sa setCount()
     */
    inline const GLuint& getCount() const {
        return count;
    }

    /**
     * @brief Sets the total number of observed pixels so far.
     * @return Number of observed pixels.
     * @sa getCount()
     */
    inline void setCount(const GLuint& count) {
        this->count = count;
    }

    /**
     * @brief Load the cost coefficients and feasible ranges from the configuration file.
     *
     * The following parameters are used:
     * | Parameter              | Description | Comment                                |
     * | ---------------------- | ----------- | -------------------------------------- |
     * | costCameraHeightChange | Cost factor for changing the height of the camera | |
     * | costDistance           | Cost factor for moving the camera (Euclidean distance) | |
     * | costArticulation       | Linear cost function for changing articulation (see ArticulationCost) | hard-coded, should be moved to config file |
     * | minCameraHeight        | Minimum feasible camera height above ground | |
     * | maxCameraHeight        | Maximum feasible camera height above ground | |
     * | gainFactor             | Coefficient for weighting the information gain relative to the costs | |
     */
    static void loadCosts();

    static size_t numArticulation;   ///< The maximum number of articulated objects, hard-coded to 20

protected:
    glm::mat4x4 cameraLocalTransform;          ///< Camera pose as homogeneous transformation matrix in world coordinates
    glm::vec3 cameraPosition;                  ///< Camera position in world coordinates
    float *articulation;                       ///< Array current of articulation positions
    GLuint count;                              ///< Total number of observed pixels so far

    /**
     * @brief Linear cost function for manipulating an articulated scene object.
     */
    struct ArticulationCost {
        float constant;   ///< Constant cost factor for manipulating the object.
        float linear;     ///< Linear cost factor for manipulating the object.
         /**
          * @brief Constructor.
          * @param[in] constant Constant cost factor for manipulating the object.
          * @param[in] linear Linear cost factor for manipulating the object.
          */
        ArticulationCost(const float& constant = 0.0f, const float& linear = 0.0f)
                : constant(constant), linear(linear) {
        }
    };

    static float costCameraHeightChange;        ///< Cost factor for changing the height of the camera, see loadCosts()
    static float costDistance;                  ///< Cost factor for moving the camera (Euclidean distance), see loadCosts()
    static float minCameraHeight;               ///< Minimum feasible camera height above ground, see loadCosts()
    static float maxCameraHeight;               ///< Maximum feasible camera height above ground, see loadCosts()
    static float gainFactor;                    ///< Coefficient for weighting the information gain relative to the costs, see loadCosts()
    static ArticulationCost *costArticulation;  ///< Linear cost function for changing articulation, see loadCosts()

    friend class RandomSearchTask;
    friend class HillclimbingTask;

};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_ROBOTSCENECONFIGURATION_H_ */
