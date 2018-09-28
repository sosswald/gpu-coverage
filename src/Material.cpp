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

#include <gpu_coverage/Image.h>
#include <gpu_coverage/Material.h>
#include <gpu_coverage/Utilities.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <map>

namespace gpu_coverage {

typedef std::map<std::string, std::string> AddTextures;
static AddTextures addTextures;

static std::string getAiMatName(const aiMaterial * const material) {
    aiString aiName;
    material->Get(AI_MATKEY_NAME, aiName);
    std::string n = aiName.C_Str();
    const std::size_t p = n.rfind("-material");
    if (p != std::string::npos && p == n.size() - 9) {
        n.replace(p, 9, "");
    }
    return n;
}

Material::Material(const aiMaterial * const material, size_t id, const std::string& dir)
        : id(id), name(getAiMatName(material)), diffuseTexture(NULL), allocatedDiffuseTexture(false) {
    aiColor4D aiAmbient, aiDiffuse, aiSpecular;
    material->Get(AI_MATKEY_COLOR_AMBIENT, aiAmbient);
    material->Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuse);
    material->Get(AI_MATKEY_COLOR_SPECULAR, aiSpecular);
    material->Get(AI_MATKEY_SHININESS, shininess);
    for (size_t i = 0; i < 4; ++i) {
        //ambient[i] = aiAmbient[i];
        ambient[i] = i < 3 ? 0.3 * aiDiffuse[i] : 1.f;
        diffuse[i] = aiDiffuse[i];
        specular[i] = aiSpecular[i];
    }
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString path;
        aiTextureMapping mapping;
        unsigned int uvIndex;
        float blend;
        aiTextureOp textureOp;
        aiTextureMapMode mapMode;
        material->GetTexture(aiTextureType_DIFFUSE, 0, &path, &mapping, &uvIndex, &blend, &textureOp, &mapMode);
        diffuseTexture = new Texture(dir + "/" + path.C_Str(), mapping, uvIndex, blend, textureOp, mapMode);
        allocatedDiffuseTexture = true;
    } else {
        AddTextures::const_iterator t = addTextures.find(name);
        if (t != addTextures.end()) {
            diffuseTexture = new Texture(dir + "/" + t->second, aiTextureMapping_UV, 0, 1., aiTextureOp_Add, aiTextureMapMode_Clamp);
            allocatedDiffuseTexture = true;
        }
    }
}

Material::~Material() {
    if (allocatedDiffuseTexture) {
        delete diffuseTexture;
        diffuseTexture = NULL;
    }
}

void Material::loadMaterialsFile(const char * const filename) {
    addTextures.clear();
    std::ifstream ifs(filename);
    std::string material;
    std::string texture;
    while (ifs.good()) {
        ifs >> material >> texture;
        if (!material.empty() && !texture.empty()) {
            addTextures[material] = texture;
        }
    }
}

} /* namespace gpu_coverage */
