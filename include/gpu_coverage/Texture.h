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

#ifndef INCLUDE_ARTICULATION_TEXTURE_H_
#define INCLUDE_ARTICULATION_TEXTURE_H_

#include <string>
#include <assimp/scene.h>
#include <gpu_coverage/Image.h>
#include GL_INCLUDE

namespace gpu_coverage {

/**
 * @brief %Texture object.
 *
 * This method manages the OpenGL texture object and metadata, but no image data.
 */
class Texture {
public:
	/**
	 * @brief Constructor for creating a texture from a file.
	 * @param[in] path Image file path passed to Image::get().
	 * @param[in] mapping Assimp texture mapping, unused.
	 * @param[in] uvIndex Assimp UV index, unused.
	 * @param[in] blend Assimp blend mode, unused.
	 * @param[in] textureOp Assimp texture operator, unused.
	 * @param[in] mapMode Assimp map mode, unused.
	 *
	 * The destructor will later delete the OpenGL texture and call Image::release() on the loaded image.
	 */
    Texture(const std::string& path, const aiTextureMapping mapping, const unsigned int uvIndex, const float blend,
            const aiTextureOp textureOp, const aiTextureMapMode mapMode);
    /**
     * @brief Constructor for creating a texture object from an existing OpenGL texture.
     * @param[in] textureObject Existing OpenGL texture.
     *
     * The destructor will **not** delete the OpenGL texture.
     */
    Texture(const GLuint textureObject);

    /**
     * @brief Constructor for creating a texture object from an Image.
     * @param[in] image Input image.
     *
     * The destructor will later delete the OpenGL texture and call Image::release() on the loaded image.
     */
    Texture(const Image * const image);

    /**
     * @brief Destructor.
     */
    ~Texture();

    /**
     * @brief Bind the texture to an OpenGL texture unit as a GL_TEXTURE_2D.
     * @param[in] unit The texture unit.
     */
    void bindToUnit(const GLuint unit) const;

    /**
     * @brief Returns the OpenGL texture ID.
     * @return OpenGL Texture ID.
     */
    inline GLuint getTextureObject() const {
        return textureObject;
    }

protected:
    const Image * const texture;               ///< The texture or NULL if the texture object has been created from an existing OpenGL texture.
    GLuint textureObject;                      ///< OpenGL texture ID.
    bool createdTextureObject;                 ///< True if the constructor has created the OpenGL texture object and the destructor should delete it later.

    /**
     * Create texture object, called from constructors.
     * @param mapMode Assimp texture mode.
     */
    void createTextureObject(const aiTextureMapMode& mapMode);
};
} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_TEXTURE_H_ */
