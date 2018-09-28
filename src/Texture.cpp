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

#include <gpu_coverage/Texture.h>
#include <gpu_coverage/Utilities.h>
#include <stdexcept>
#include <iostream>

namespace gpu_coverage {

Texture::Texture(const std::string& path, const aiTextureMapping, const unsigned int, const float, const aiTextureOp,
        const aiTextureMapMode mapMode)
        : texture(Image::get(path)), createdTextureObject(true) {
    createTextureObject(mapMode);
}

Texture::Texture(const Image * const image)
        : texture(image), createdTextureObject(true) {
    createTextureObject(aiTextureMapMode_Clamp);
}

Texture::Texture(const GLuint textureObject)
        : texture(NULL), textureObject(textureObject), createdTextureObject(false)
{
}

Texture::~Texture() {
    if (texture) {
        Image::release(texture);
    }
    if (createdTextureObject) {
        glDeleteTextures(1, &textureObject);
    }
}

void Texture::createTextureObject(const aiTextureMapMode& mapMode) {
    createdTextureObject = true;
    glGenTextures(1, &textureObject);
    glBindTexture(GL_TEXTURE_2D, textureObject);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    switch (mapMode) {
    case aiTextureMapMode_Wrap:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        break;
    case aiTextureMapMode_Clamp:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;
    case aiTextureMapMode_Mirror:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        break;
    default:
        throw std::invalid_argument(std::string("Unsupported texture map mode"));
    }
    GLenum format;
    if (texture->image().channels() == 3) {
        format = GL_RGB;
    } else if (texture->image().channels() == 4) {
        format = GL_RGBA;
    } else {
        throw std::invalid_argument(std::string("Unsupported number of channels in texture"));
    }
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8,
            texture->image().cols,
            texture->image().rows);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            texture->image().cols, texture->image().rows,
            format,
            GL_UNSIGNED_BYTE,
            texture->image().data);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->image().cols, texture->image().rows, 0, format, GL_UNSIGNED_BYTE, texture->image().data);
    checkGLError();
    GLint status;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT, &status);
    glBindTexture(GL_TEXTURE_2D, 0);
    checkGLError();
}
void Texture::bindToUnit(const GLuint unit) const {
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, textureObject);
}

} /* namespace gpu_coverage */
