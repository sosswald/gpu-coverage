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

#include <gpu_coverage/CameraPerspective.h>
#include <gpu_coverage/Node.h>
#include <gpu_coverage/Mesh.h>
#include <gpu_coverage/Light.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/Channel.h>
#include <gpu_coverage/Utilities.h>

#include <cstdio>
#include <iostream>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/io.hpp>

#include GL_INCLUDE
#include GLEXT_INCLUDE

namespace gpu_coverage {

static size_t highestId = 0;

Node::Node(const aiNode * const node, const aiScene * scene, const std::vector<Mesh*>& allMeshes,
        Node * const parent)
        : id(++highestId), name(std::string(node->mName.C_Str())), parent(parent), visible(true) {
    setLocalTransform(glm::transpose(glm::make_mat4(node->mTransformation[0])));

    children.resize(node->mNumChildren, 0);
    allocatedChild.resize(node->mNumChildren, true);
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        children[i] = new Node(node->mChildren[i], scene, allMeshes, this);
    }
    meshes.resize(node->mNumMeshes);
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        meshes[i] = allMeshes[node->mMeshes[i]];
    }
}

Node::Node(const std::string& name, Node * const parent)
        : id(++highestId), name(name), parent(parent), visible(true) {
    setLocalTransform(glm::mat4());
    parent->children.push_back(this);
    parent->allocatedChild.push_back(false);
    logInfo("Created node %s [dummy]", name.c_str());
}

Node::~Node() {
    for (size_t i = 0; i < children.size(); ++i) {
        if (allocatedChild[i]) {
            delete children[i];
        }
    }
    children.clear();
}

void Node::toDot(FILE *dot) {
    const glm::vec3 position(glm::column(worldTransform, 3));
    const glm::vec3 scaling = glm::vec3(glm::length(glm::column(worldTransform, 0)),
            glm::length(glm::column(worldTransform, 1)), glm::length(glm::column(worldTransform, 2)));
    const glm::mat3 unscale = glm::mat3(
            glm::vec3(glm::column(worldTransform, 0)) / scaling[0],
            glm::vec3(glm::column(worldTransform, 1)) / scaling[1],
            glm::vec3(glm::column(worldTransform, 2)) / scaling[2]);
    const glm::quat rotation(unscale);
    fprintf(dot,
            "  n%zu [label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"1\"><tr><td colspan=\"2\">%s</td></tr><tr><td>position</td><td>%.2f, %.2f, %.2f</td></tr><tr><td>rotation</td><td>%.1f&#176;, %.1f&#176;, %.1f&#176;</td></tr><tr><td>scale</td><td>%.1f, %.1f, %.1f</td></tr></table>>, shape=\"plaintext\"];\n",
            id, name.c_str(),
            position.x, position.y, position.z,
            glm::degrees(glm::roll(rotation)), glm::degrees(glm::pitch(rotation)), glm::degrees(glm::yaw(rotation)),
            scaling.x, scaling.y, scaling.z
            );
    if (parent != NULL) {
        fprintf(dot, "  n%zu -> n%zu;\n", parent->getId(), id);
    }
    for (size_t i = 0; i < children.size(); ++i) {
        children[i]->toDot(dot);
    }
    for (size_t i = 0; i < meshes.size(); ++i) {
        fprintf(dot, "  n%zu -> m%zu", id, meshes[i]->getId());
    }
}

void Node::setFrame() {
    setFrameRecursive(false);
}

void Node::setFrameRecursive(bool needsUpdate) {
    for (Channels::iterator channelIt = channels.begin(); channelIt != channels.end(); ++channelIt) {
        localTransform = (*channelIt)->getLocalTransform();
        needsUpdate = true;
    }
    if (needsUpdate) {
        updateWorldTransform();
    }

    for (Children::const_iterator childIt = children.begin(); childIt != children.end(); ++childIt) {
        if ((*childIt)->isVisible())
            (*childIt)->setFrameRecursive(needsUpdate);
    }
}

void Node::render(const std::vector<glm::mat4>& view,
        const LocationsMVP * const locationsMVP,
        const LocationsMaterial * const locationsMaterial,
        const bool hasTesselationShader) const {
    for (Meshes::const_iterator meshIt = meshes.begin(); meshIt != meshes.end(); ++meshIt) {
        glm::mat4 model;
        if ((*meshIt)->getBones().empty()) {
            model = worldTransform;
        } else {
            model = (*meshIt)->getBones()[0]->getNode()->getWorldTransform()
                    * (*meshIt)->getBones()[0]->getOffsetMatrix();
        }
        for (size_t i = 0; i < view.size(); ++i) {
            if (locationsMVP->normalMatrix[i] != -1) {
                const glm::mat3 normal = glm::inverse(glm::transpose(glm::mat3(view[i] * model)));
                glUniformMatrix3fv(locationsMVP->normalMatrix[i], 1, GL_FALSE, glm::value_ptr(normal));
            }
        }
        glUniformMatrix4fv(locationsMVP->modelMatrix, 1, GL_FALSE, glm::value_ptr(model));
        checkGLError();
        (*meshIt)->render(locationsMaterial, hasTesselationShader);
        checkGLError();
    }
    // render children recursively
    for (Children::const_iterator childIt = children.begin(); childIt != children.end(); ++childIt) {
        if ((*childIt)->isVisible())
            (*childIt)->render(view, locationsMVP, locationsMaterial, hasTesselationShader);
    }
    checkGLError();

}

void Node::updateWorldTransform() {
    if (parent)
        worldTransform = parent->worldTransform * localTransform;
    else
        worldTransform = localTransform;
}

void Node::addCamera(AbstractCamera * const camera) {
    cameras.push_back(camera);
}

void Node::addLight(Light * const light) {
    lights.push_back(light);
}

void Node::addAnimationChannel(Channel * const channel) {
    channels.push_back(channel);
}

} /* namespace gpu_coverage */
