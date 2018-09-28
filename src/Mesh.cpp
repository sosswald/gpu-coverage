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

#include <gpu_coverage/Mesh.h>
#include <gpu_coverage/Material.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/Utilities.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <iostream>
#include <sstream>
#include <cstdio>

#include GLEXT_INCLUDE

namespace gpu_coverage {

inline static std::string getMeshName(const aiMesh * const mesh, const size_t id) {
    if (mesh->mName.length > 0) {
        return std::string(mesh->mName.C_Str());
    } else {
        std::stringstream ss;
        ss << "Mesh " << id;
        return ss.str();
    }
}

/**
 *	Constructor, loading the specified aiMesh
 **/
Mesh::Mesh(const aiMesh * const mesh, const size_t id, const std::vector<Material*>& materials)
        : id(id), name(getMeshName(mesh, id)), material(NULL)
{
    vbo[VERTEX_BUFFER] = 0;
    vbo[COLOR_BUFFER] = 0;
    vbo[TEXCOORD_BUFFER] = 0;
    vbo[NORMAL_BUFFER] = 0;
    vbo[INDEX_BUFFER] = 0;

    glGenVertexArrays(1, &vao);
    checkGLError();
    glBindVertexArray(vao);
    checkGLError();

    if (mesh->HasPositions()) {
        float *vertices = new float[mesh->mNumVertices * 3];
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            vertices[i * 3] = mesh->mVertices[i].x;
            vertices[i * 3 + 1] = mesh->mVertices[i].y;
            vertices[i * 3 + 2] = mesh->mVertices[i].z;
        }

        glGenBuffers(1, &vbo[VERTEX_BUFFER]);
        checkGLError();
        glBindBuffer(GL_ARRAY_BUFFER, vbo[VERTEX_BUFFER]);
        checkGLError();
        glBufferData(GL_ARRAY_BUFFER, 3 * mesh->mNumVertices * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
        checkGLError();

        glVertexAttribPointer(VERTEX_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        checkGLError();
        glEnableVertexAttribArray(VERTEX_BUFFER);
        checkGLError();

        delete[] vertices;
    }

    if (mesh->mMaterialIndex < materials.size()) {
        material = materials[mesh->mMaterialIndex];
    }

    if (mesh->HasTextureCoords(0)) {
        float *texCoords = new float[mesh->mNumVertices * 2];
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            texCoords[i * 2] = mesh->mTextureCoords[0][i].x;
            texCoords[i * 2 + 1] = mesh->mTextureCoords[0][i].y;
        }

        glGenBuffers(1, &vbo[TEXCOORD_BUFFER]);
        checkGLError();
        glBindBuffer(GL_ARRAY_BUFFER, vbo[TEXCOORD_BUFFER]);
        checkGLError();
        glBufferData(GL_ARRAY_BUFFER, 2 * mesh->mNumVertices * sizeof(GLfloat), texCoords, GL_STATIC_DRAW);
        checkGLError();

        glVertexAttribPointer(TEXCOORD_BUFFER, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        checkGLError();
        glEnableVertexAttribArray(TEXCOORD_BUFFER);
        checkGLError();

        delete[] texCoords;
    }

    if (mesh->HasNormals()) {
        float *normals = new float[mesh->mNumVertices * 3];
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            normals[i * 3] = mesh->mNormals[i].x;
            normals[i * 3 + 1] = mesh->mNormals[i].y;
            normals[i * 3 + 2] = mesh->mNormals[i].z;
        }

        glGenBuffers(1, &vbo[NORMAL_BUFFER]);
        checkGLError();
        glBindBuffer(GL_ARRAY_BUFFER, vbo[NORMAL_BUFFER]);
        checkGLError();
        glBufferData(GL_ARRAY_BUFFER, 3 * mesh->mNumVertices * sizeof(GLfloat), normals, GL_STATIC_DRAW);
        checkGLError();

        glVertexAttribPointer(NORMAL_BUFFER, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        checkGLError();
        glEnableVertexAttribArray(NORMAL_BUFFER);
        checkGLError();

        delete[] normals;
    }

    if (mesh->HasFaces() && mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE) {
        elementCount = mesh->mNumFaces * 3;
        unsigned int *indices = new unsigned int[mesh->mNumFaces * 3];
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            indices[i * 3] = mesh->mFaces[i].mIndices[0];
            indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
            indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
        }

        glGenBuffers(1, &vbo[INDEX_BUFFER]);
        checkGLError();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[INDEX_BUFFER]);
        checkGLError();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * mesh->mNumFaces * sizeof(GLuint), indices, GL_STATIC_DRAW);
        checkGLError();

        glVertexAttribPointer(INDEX_BUFFER, 3, GL_UNSIGNED_INT, GL_FALSE, 0, NULL);
        checkGLError();
        glEnableVertexAttribArray(INDEX_BUFFER);
        checkGLError();

        delete[] indices;
    } else {
        elementCount = 0;
    }

    if (mesh->HasBones()) {
        bones.reserve(mesh->mNumBones);
        for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
            bones.push_back(new Bone(mesh->mBones[i], id, i));
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    checkGLError();
    glBindVertexArray(0);
    checkGLError();
}

/**
 *	Deletes the allocated OpenGL buffers
 **/
Mesh::~Mesh() {
    if (vbo[VERTEX_BUFFER]) {
        glDeleteBuffers(1, &vbo[VERTEX_BUFFER]);
    }

    if (vbo[TEXCOORD_BUFFER]) {
        glDeleteBuffers(1, &vbo[TEXCOORD_BUFFER]);
    }

    if (vbo[NORMAL_BUFFER]) {
        glDeleteBuffers(1, &vbo[NORMAL_BUFFER]);
    }

    if (vbo[INDEX_BUFFER]) {
        glDeleteBuffers(1, &vbo[INDEX_BUFFER]);
    }

    glDeleteVertexArrays(1, &vao);
    for (size_t i = 0; i < bones.size(); ++i) {
        delete bones[i];
    }
    bones.clear();
}

void Mesh::render(const LocationsMaterial * const locations, const bool hasTesselationShader) const {
    if (material && locations) {
        glUniform1f(locations->materialShininess, material->getShininess());
        glUniform3fv(locations->materialAmbient, 1, material->getAmbient());
        glUniform3fv(locations->materialDiffuse, 1, material->getDiffuse());
        glUniform3fv(locations->materialSpecular, 1, material->getSpecular());
        if (material->hasTexture()) {
            material->getTexture()->bindToUnit(GL_TEXTURE2);
            glUniform1i(locations->textureUnit, 2);
            glUniform1i(locations->hasTexture, true);
        } else {
            glUniform1i(locations->hasTexture, false);
        }
    } else if (locations) {
        glUniform1i(locations->hasTexture, false);
    }
    checkGLError();
    glBindVertexArray(vao);
    checkGLError();
    /*char msg[256];
     snprintf(msg, sizeof(msg), "Rendering mesh %s", name.c_str());
     glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 1, GL_DEBUG_SEVERITY_NOTIFICATION, strlen(msg), msg);*/
    glDrawElements(hasTesselationShader ? GL_PATCHES : GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, NULL);
    checkGLError();
    glBindVertexArray(0);
    checkGLError();
}

void Mesh::toDot(FILE *dot) const {
    fprintf(dot, "  m%zu [label=\"{%s|{Material|%s}}\", shape=\"record\"];\n", id, name.c_str(),
            material->getName().c_str());
    for (Bones::const_iterator boneIt = bones.begin(); boneIt != bones.end(); ++boneIt) {
        (*boneIt)->toDot(dot);
    }
}
}  // namespace gpu_coverage
