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

#ifndef INCLUDE_ARTICULATION_ABSTRACTRENDERER_H_
#define INCLUDE_ARTICULATION_ABSTRACTRENDERER_H_

#include <gpu_coverage/Scene.h>

namespace gpu_coverage {

/**
 * @brief Abstract superclass for all renderers.
 *
 * Subclasses must implement the following methods:
 * * display(): renders the scene
 * * getTexture(): returns the OpenGL texture ID of the result
 * * getTextureWidth() and getTextureHeight(): return the dimensions of the result texture
 */class AbstractRenderer {
public:
    /**
     * @brief Constructor.
     * @param[in] scene Scene to be rendered.
     * @param[in] name Human-readable name for this renderer, used for logging.
     */
    AbstractRenderer(const Scene * const scene, const std::string& name);

    /**
     * @brief Destructor.
     */
    virtual ~AbstractRenderer();

    /**
     * @brief Renders the scene.
     *
     * This method does the main work of the renderer.
     */
    virtual void display() = 0;

    /**
     * @brief Returns the OpenGL texture ID of the result texture.
     * @return OpenGL texture ID.
     */
    virtual const GLuint& getTexture() const = 0;

    /**
     * @brief Width of the result texture.
     * @return Width in pixels.
     */
    virtual const int& getTextureWidth() const = 0;

    /**
     * @brief Height of the result texture.
     * @return Height in pixels.
     */
    virtual const int& getTextureHeight() const = 0;

    /**
     * @brief Returns true if the renderer has been initialized correctly.
     * @return True if renderer is ready.
     */
    inline const bool& isReady() const {
        return ready;
    }

    /**
     * @brief Returns the name of the renderer for logging purposes.
     * @return Name of the renderer.
     */
    inline const std::string& getName() const {
        return name;
    }

protected:
    const Scene * const scene;   ///< Pointer to the scene to be rendered.
    const std::string name;      ///< Name of the renderer, see getName().
    bool ready;                  ///< Set to true when renderer is ready, see isReady().
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_ABSTRACTRENDERER_H_ */
