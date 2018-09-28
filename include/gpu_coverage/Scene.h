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

#ifndef INCLUDE_ARTICULATION_SCENE_H_
#define INCLUDE_ARTICULATION_SCENE_H_

#include <gpu_coverage/Light.h>
#include <gpu_coverage/Material.h>
#include <gpu_coverage/Mesh.h>
#include <gpu_coverage/Node.h>
#include <gpu_coverage/Animation.h>
#include <gpu_coverage/CameraPanorama.h>
#include <gpu_coverage/CameraPerspective.h>
#include <gpu_coverage/Image.h>
#include <gpu_coverage/Programs.h>
#include <map>

// Forward declaration
struct aiScene;

namespace gpu_coverage {

// Forward declaration
struct Locations;

/**
 * @brief %Scene graph corresponding to Assimp's aiScene.
 *
 * Limitations: Currently only one light source is supported for
 * rendering the visual representation of the 3D scene.
 */
class Scene {
public:
	/**
	 * @brief Constructor.
	 * @param aiScene[in] Assimp scene structure.
	 * @param dir[in] Filesystem directory from where to load materials.
	 */
    Scene(const aiScene * const aiScene, const std::string& dir);

    /**
     * @brief Destructor.
     */
    virtual ~Scene();

    /**
     * @brief Render the scene with the current shader.
     * @param[in] camera Camera rendering the scene.
     * @param[in] locationsMVP Locations of the model, view, and projection shader variables, must not be NULL.
     * @param[in] locationsLight Location of the light source shader variables, ignored if NULL.
     * @param[in] locationsMaterial Location of the material shader variables, ignored if NULL.
     * @param[in] hasTesselationShader Set to true if a tesselation shader is present.
     * @exception std::invalid_argument locationsMVP is NULL.
     */
    void render(const AbstractCamera * const camera,
            const LocationsMVP * const locationsMVP,
            const LocationsLight * const locationsLight = NULL,
            const LocationsMaterial * const locationsMaterial = NULL,
            const bool hasTesselationShader = false) const;

    /**
     * @brief Finds a node by name, returns NULL if not found.
     * @param name Name of the node.
     * @return Pointer to the node of NULL if not found.
     */
    Node *findNode(const std::string& name) const;

    /**
     * @brief Finds a material by name, returns NULL if not found.
     * @param name Name of the material.
     * @return Pointer to the material of NULL if not found.
     */
    Material *findMaterial(const std::string& name) const;

    /**
     * @brief Finds a camera by name, returns NULL if not found.
     * @param name Name of the camera.
     * @return Pointer to the camera of NULL if not found.
     */
    CameraPerspective *findCamera(const std::string& name) const;

    /**
     * @brief Adds a new panorama camera to the scene and returns a pointer.
     * @param camera Scene graph node where to attach the new camera.
     * @return Pointer to the new camera.
     */
    CameraPanorama *makePanoramaCamera(Node * const camera);

    typedef std::vector<CameraPerspective*> Cameras;           ///< Vector of cameras of the sceene.
    typedef std::vector<Light*> Lights;                        ///< Vector of light sources of the scene.
    typedef std::vector<Material*> Materials;                  ///< Vector of materials of the scene.
    typedef std::vector<Mesh*> Meshes;                         ///< Vector of meshes of the scene.
    typedef std::vector<Image*> Textures;                      ///< Vector of mesh textures of the scene.
    typedef std::vector<Animation*> Animations;                ///< Vector of animations of the scene.
    typedef std::vector<CameraPanorama*> PanoramaCameras;      ///< Vector of panorama cameras of the scene.
    typedef std::map<std::string, Node*> NodeMap;              ///< Map mapping node names to node pointers.
    typedef std::vector<Channel*> Channels;                    ///< Vector of animation channels of the scene.
    NodeMap nodeMap;                                           ///< Map mapping node names to node pointers.
    Meshes meshes;                                             ///< Vector of meshes of the scene.
    Cameras cameras;                                           ///< Vector of cameras of the sceene.
    PanoramaCameras panoramaCameras;                           ///< Vector of panorama cameras of the scene.
    Lights lights;                                             ///< Vector of light sources of the scene.
    Textures textures;                                         ///< Vector of mesh textures of the scene.
    Materials materials;                                       ///< Vector of materials of the scene.
    Animations animations;                                     ///< Vector of animations of the scene.
    Channels channels;                                         ///< Vector of animation channels of the scene.
    Node *root;                                                ///< Root node of the scene graph.
    gpu_coverage::Node * lampNode;                             ///< Scene graph node where the first light source is attached.

    /**
     * @brief Returns the number of animation frames.
     * @return Number of animation frames.
     */
    inline const size_t& getNumFrames() const {
        return numFrames;
    }
    /**
     * @brief Returns the start frame of the animation.
     * @return Start frame.
     */
    inline const size_t& getStartFrame() const {
        return startFrame;
    }
    /**
     * @brief Returns the last frame of the animation.
     * @return End frame.
     */
    inline const size_t& getEndFrame() const {
        return endFrame;
    }

    /**
     * Returns the animation channels.
     * @return Animation channels.
     */
    inline const Channels& getChannels() const {
        return channels;
    }

    /**
     * @brief Get the root node of the scene graph.
     * @return Root node.
     */
    inline Node * getRoot() const {
        return root;
    }

    /**
     * @brief Write the scene graph structure as a GraphViz Dot file for debugging.
     * @param dotFilePath File path for the output Dot file.
     */
    void toDot(const char* dotFilePath) const;

protected:
    void collectNodes(Node * node);

    size_t numFrames;          ///< Number of animation frames, see getNumFrames().
    size_t startFrame;         ///< First animation frame, see getStartFrame().
    size_t endFrame;           ///< Last animation frame, see getEndFrame().
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_SCENE_H_ */
