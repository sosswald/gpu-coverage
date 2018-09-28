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

#include <assimp/scene.h>
#include <gpu_coverage/Scene.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/Utilities.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#define GLM_FORCE_RADIANS true
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/io.hpp>

#include GL_INCLUDE
#include GLEXT_INCLUDE

namespace gpu_coverage {

Scene::Scene(const aiScene * const aiScene, const std::string& dir)
        : root(NULL), lampNode(NULL) {
    for (unsigned int i = 0; i < aiScene->mNumTextures; ++i) {
        textures.push_back(new Image(aiScene->mTextures[i], i));
    }
    for (unsigned int i = 0; i < aiScene->mNumMaterials; ++i) {
        materials.push_back(new Material(aiScene->mMaterials[i], i, dir));
    }
    for (unsigned int i = 0; i < aiScene->mNumMeshes; ++i) {
        meshes.push_back(new Mesh(aiScene->mMeshes[i], i, materials));
    }
    root = new Node(aiScene->mRootNode, aiScene, meshes);
    collectNodes(root);

    for (unsigned int i = 0; i < aiScene->mNumCameras; ++i) {
        Node * const cameraNode = findNode(aiScene->mCameras[i]->mName.C_Str());
        CameraPerspective * const camera = new CameraPerspective(aiScene->mCameras[i], i, cameraNode);
        cameras.push_back(camera);
        cameraNode->addCamera(camera);
    }
    for (unsigned int i = 0; i < aiScene->mNumLights; ++i) {
        Node * const lightNode = findNode(aiScene->mLights[i]->mName.C_Str());
        Light * const light = new Light(aiScene->mLights[i], i, lightNode);
        lights.push_back(light);
        lightNode->addLight(light);
        if (i == 0) {
            lampNode = lightNode;
        }
    }
    startFrame = std::numeric_limits<size_t>::max();
    endFrame = 0;
    for (unsigned int i = 0; i < aiScene->mNumAnimations; ++i) {
        Animation * const animation = new Animation(this, aiScene->mAnimations[i], i);
        animations.push_back(animation);
        if (animation->getStartFrame() < startFrame) {
            startFrame = animation->getStartFrame();
        }
        if (animation->getEndFrame() > endFrame) {
            endFrame = animation->getEndFrame();
        }
        for (Animation::Channels::const_iterator channelIt = animation->getChannels().begin();
                channelIt != animation->getChannels().end(); ++channelIt) {
            channels.push_back(*channelIt);
        }
    }
    if (startFrame > endFrame) {
        startFrame = endFrame = 0;
    }
    numFrames = endFrame - startFrame;

    root->setFrame();
}

Scene::~Scene() {
    for (unsigned int i = 0; i < meshes.size(); ++i) {
        delete meshes.at(i);
    }
    meshes.clear();
    for (unsigned int i = 0; i < materials.size(); ++i) {
        delete materials.at(i);
    }
    materials.clear();
    for (unsigned int i = 0; i < lights.size(); ++i) {
        delete lights.at(i);
    }
    lights.clear();
    for (unsigned int i = 0; i < textures.size(); ++i) {
        delete textures.at(i);
    }
    textures.clear();
    for (unsigned int i = 0; i < cameras.size(); ++i) {
        delete cameras.at(i);
    }
    cameras.clear();
    for (unsigned int i = 0; i < animations.size(); ++i) {
        delete animations.at(i);
    }
    animations.clear();
    delete root;
}

void Scene::toDot(const char* dotFilePath) const {
    FILE* dot = fopen(dotFilePath, "w");
    fputs("digraph scene {\n", dot);
    root->toDot(dot);
    for (Meshes::const_iterator meshIt = meshes.begin(); meshIt != meshes.end();
            ++meshIt) {
        (*meshIt)->toDot(dot);
    }
    for (Cameras::const_iterator cameraIt = cameras.begin();
            cameraIt != cameras.end(); ++cameraIt) {
        (*cameraIt)->toDot(dot);
    }
    for (Lights::const_iterator lightIt = lights.begin();
            lightIt != lights.end(); ++lightIt) {
        (*lightIt)->toDot(dot);
    }
    for (Animations::const_iterator animationIt = animations.begin();
            animationIt != animations.end(); ++animationIt) {
        (*animationIt)->toDot(dot);
    }
    fputs("}\n", dot);
    fclose(dot);
}

void Scene::render(const AbstractCamera * const camera,
        const LocationsMVP * const locationsMVP,
        const LocationsLight * const locationsLight,
        const LocationsMaterial * const locationsMaterial,
        const bool hasTesselationShader) const {
    if (!locationsMVP) {
        throw std::invalid_argument("Scene::render: locationsMVP must not be NULL");
    }
    // Compute animation
    root->setFrame();

    std::vector<glm::mat4> view;
    camera->setViewProjection(*locationsMVP, view);

    // Set lighting
    if (locationsLight && lampNode) {
        const glm::mat4 lampTransform = lampNode->getWorldTransform();
        glUniform4fv(locationsLight->lightPosition, 1, glm::value_ptr(glm::column(lampTransform, 3)));
        glUniform3fv(locationsLight->lightDiffuse, 1, glm::value_ptr(lampNode->getLights()[0]->getDiffuse()));
        glUniform3fv(locationsLight->lightAmbient, 1, glm::value_ptr(lampNode->getLights()[0]->getAmbient()));
        glUniform3fv(locationsLight->lightSpecular, 1, glm::value_ptr(lampNode->getLights()[0]->getSpecular()));
    }

    checkGLError();
    if (root->isVisible()) {
        // Render
        root->render(view, locationsMVP, locationsMaterial, hasTesselationShader);
    }
}

Node *Scene::findNode(const std::string& name) const {
    const NodeMap::const_iterator nodeIt = nodeMap.find(name);
    if (nodeIt == nodeMap.end()) {
        return NULL;
    } else {
        return nodeIt->second;
    }
}

CameraPerspective *Scene::findCamera(const std::string& name) const {
    for (Cameras::const_iterator cameraIt = cameras.begin(); cameraIt != cameras.end(); ++cameraIt) {
        if ((*cameraIt)->getName() == name) {
            return *cameraIt;
        }
    }
    return NULL;
}

CameraPanorama * Scene::makePanoramaCamera(Node * const node) {
    CameraPanorama * const pcam = new CameraPanorama(cameras.size(), node);
    node->addCamera(pcam);
    cameras.push_back(pcam);
    panoramaCameras.push_back(pcam);
    return pcam;
}

void Scene::collectNodes(Node * node) {
    nodeMap[node->getName()] = node;
    for (Node::Children::const_iterator childIt = node->getChildren().begin(); childIt != node->getChildren().end();
            ++childIt) {
        collectNodes(*childIt);
    }
}

Material *Scene::findMaterial(const std::string& name) const {
    for (Materials::const_iterator it = materials.begin(); it != materials.end(); ++it) {
        if ((*it)->getName() == name) {
            return *it;
        }
    }
    return NULL;
}

} /* namespace gpu_coverage */
