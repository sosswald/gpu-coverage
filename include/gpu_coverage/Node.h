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

#ifndef INCLUDE_ARTICULATION_NODE_H_
#define INCLUDE_ARTICULATION_NODE_H_

#include <gpu_coverage/Programs.h>
#include <assimp/scene.h>
#include <glm/detail/type_mat4x4.hpp>
#include <vector>
#include <iostream>

namespace gpu_coverage {

// Forward declarations
struct LocationsCommonRender;
class Mesh;
class AbstractCamera;
class Light;
class Channel;

/**
 * @brief %Scene graph node, corresponding to Assimp's aiNode.
 */
class Node {
public:
    typedef std::vector<Node *> Children;
    typedef std::vector<Mesh *> Meshes;
    typedef std::vector<AbstractCamera *> Cameras;
    typedef std::vector<Light *> Lights;
    typedef std::vector<Channel *> Channels;

    /**
     * @brief Constructor for creating a node from an Assimp aiNode.
     * @param node The Assimp aiNode for creating this node.
     * @param scene The scene containing this node.
     * @param meshes All meshes loaded beforehand.
     * @param parent The parent node (NULL if this node is the root node).
     */
    Node(const aiNode * const node, const aiScene * scene, const std::vector<Mesh*>& meshes, Node * const parent =
            NULL);
    /**
     * @brief Constructor for manually creating a child node.
     * @param name Name of the child node.
     * @param parent Parent node.
     */
    Node(const std::string& name, Node * const parent);
    /**
     * @brief Destructor.
     */
    virtual ~Node();

    /**
     * @brief Set the current frame of the animation.
     * @param[in] frame Frame number of the animation.
     *
     * This method passes the new frame number to all animation Channels
     * and recomputes the local transforms of all tree nodes recursively.
     */
    void setFrame();

    /**
     * @brief Renders the scene using the current shaders.
     * @param view View matrix (or matrices) as set by AbstractCamera::setViewProjection().
     * @param locationsMVP Location of the shader variables for model, view, and projection matrices.
     * @param locationsMaterial Location of the shader variables for material and light variables.
     * @param hasTesselationShader True if the current shader has a tesselation stage.
     */
    void render(const std::vector<glm::mat4>& view, const LocationsMVP * const locationsMVP,
            const LocationsMaterial * const locationsMaterial, const bool hasTesselationShader) const;

    /**
     * @brief Write Graphviz %Dot node representing this node to file for debugging.
     * @param[in] dot Output %Dot file.
     */
    void toDot(FILE *dot);

    /**
     * @brief Returns a vector of all direct children.
     * @return Children of this node.
     */
    inline const Children& getChildren() const {
        return children;
    }

    /**
     * @brief Returns a vector of all meshes associated with this node.
     * @return Children of this node.
     */
    inline const Meshes& getMeshes() const {
        return meshes;
    }

    /**
     * @brief Returns the unique ID of this node.
     * @return ID.
     */
    inline size_t getId() const {
        return id;
    }

    /**
     * @brief Returns the name of this node for logging.
     * @return Name of this node.
     *
     * If available, the name included in the input file read by Assimp is used.
     */
    inline const std::string& getName() const {
        return name;
    }

    /**
     * @brief Returns the parent node.
     * @return Parent node.
     */
    inline const Node * getParent() const {
        return parent;
    }

    /**
     * @brief Returns the local transform of the node relative to the parent node.
     * @return Local transform.
     */
    inline const glm::mat4& getLocalTransform() const {
        return localTransform;
    }

    /**
     * @brief Returns the transform of the node in world coordinates.
     * @return World transform.
     */
    inline const glm::mat4& getWorldTransform() const {
        return worldTransform;
    }

    /**
     * @brief Sets the local transform of the node relative to the parent node.
     * @param localTransform The local transform.
     *
     * This method also updates the node's world transform, but does
     * not update the transformations of the children.
     */
    inline void setLocalTransform(const glm::mat4& localTransform) {
        this->localTransform = localTransform;
        updateWorldTransform();
    }

    /**
     * @brief Adds a camera to this node.
     * @param camera
     */
    void addCamera(AbstractCamera * const camera);

    /**
     * @brief Adds a light source to this node.
     * @param camera
     */
    void addLight(Light * const light);

    /**
     * @brief Adds an animation channel to this node.
     * @param camera
     */
    void addAnimationChannel(Channel * const channel);

    /**
     * @brief Returns true if the node is visible.
     * @param True if node is visible.
     */
    inline bool isVisible() const {
        return visible;
    }

    /**
     * @brief Sets the visibility of this node.
     * @param visible True if node is visible.
     *
     * Children of invisible nodes will not be rendered.
     */
    inline void setVisible(bool visible) {
        this->visible = visible;
    }

    /**
     * @brief Returns the cameras associated with this node.
     * @return Vector of cameras.
     */
    inline const Cameras& getCameras() const {
        return cameras;
    }

    /**
     * @brief Returns the light sources associated with this node.
     * @return Vector of light sources.
     */
    inline const Lights& getLights() const {
        return lights;
    }

    /**
     * @brief Returns the animation channels influencing this node.
     * @return Vector of animation channels.
     */
    inline const Channels& getChannels() const {
        return channels;
    }

protected:
    const size_t id;             ///< Unique ID, see getId().
    const std::string name;      ///< Name of this node for logging, see getName().

    Children children;                        ///< Direct children of this node.
    std::vector<bool> allocatedChild;         ///< Bit map indicating which children this node has allocated and is in charge of deleting later.
    Meshes meshes;                            ///< Meshes associated with this node.
    Cameras cameras;                          ///< Cameras associated with this node.
    Lights lights;                            ///< Light sources associated with this node.
    Channels channels;                        ///< Animation channels influencing this node.
    Node * const parent;                      ///< Parent node.
    glm::mat4 worldTransform;                 ///< Current world transform, see getWorldTransform().
    glm::mat4 localTransform;                 ///< Current world transform, see getLocalTransform().
    bool visible;                             ///< True if node is visible while rendering, see setVisible() and isVisible().

    /**
     * @brief Updates the current frame number recursively in all child frames.
     * @param needsUpdate True if the local transform needs to be updated.
     *
     * needsUpdate is true if the local transform of this node or one of its ancestors in the
     * tree has changed due to an animation channel. In this case, the world transform of all
     * children will also have to be recomputed.
     */
    void setFrameRecursive(bool needsUpdate);

    /**
     * @brief Recomputes the world transform after the local transform has changed.
     *
     * setLocalTransform() triggers this method.
     */
    void updateWorldTransform();

    friend class Scene;
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_NODE_H_ */
