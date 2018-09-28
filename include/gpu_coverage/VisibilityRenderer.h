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

#ifndef INCLUDE_ARTICULATION_VISIBILITYRENDERER_H_
#define INCLUDE_ARTICULATION_VISIBILITYRENDERER_H_

#include <gpu_coverage/AbstractRenderer.h>
#include <gpu_coverage/Programs.h>
#include <list>

#undef GET_BUFFER_DIRECT

namespace gpu_coverage {

/**
 * @brief Determines regions visible from a given camera pose and marks the regions as observed on the texture.
 */
class VisibilityRenderer: public AbstractRenderer {
public:
	/**
	 * @brief Constructor
	 * @param[in] scene Sceen to be rendered.
	 * @param[in] renderToWindow Set to true to render also to the window framebuffer.
	 * @param[in] countPixels Set to true to count the number of observed pixels.
	 */
    VisibilityRenderer(const Scene * const scene, const bool renderToWindow, const bool countPixels);

    /**
     * @brief Destructor.
     */
    ~VisibilityRenderer();

    /**
     * @brief Renders the scene.
     *
     * This method redirects to display(const bool countPixels) with the
     * default value for countPixels passed to the constructor.
     */

    virtual void display();
    /**
     * @brief Renders the scene.
     * @param countPixels True if number of newly observed pixels should be counted.
     *
     * This method does the main work of the renderer.
     */
    void display(const bool countPixels);

    /**
     * @brief Returns the list of observed pixels of the previous frames.
     * @param[out] counts Number of pixels that were observed in the previous frames.
     * @exception std::invalid_argument Pixel counting has been disabled in the constructor.
     *
     * This method clears fills the vector countPixels with the number
     * of target texels that have been observed since the last call
     * to this method.
     *
     * This method causes the GPU pipeline to be flushed in order to get access
     * to the pixel count of the most recent frame. Hence, this method should be
     * called as rarely as possible, in the optimal case only once after all
     * rendering has been completed.
     */
    void getPixelCounts(std::vector<GLuint>& counts);

    /**
     * @brief Returns the OpenGL texture ID of the result texture.
     * @return OpenGL texture ID.
     */
    inline const GLuint& getTexture() const {
        return textures[VISIBILITY];
    }
    /**
     * @brief Returns the OpenGL texture ID of the result texture in case of multiple textures.
     * @param i Number of the texture
     * @return OpenGL texture ID.
     */
    inline const GLuint& getTexture(const size_t& i) const {
        return textures[VISIBILITY + i];
    }

    /**
     * @brief Height of the result texture.
     * @return Height in pixels.
     */
    inline const int& getTextureHeight() const {
        return textureHeight;
    }

    /**
     * @brief Width of the result texture.
     * @return Width in pixels.
     */
    inline const int& getTextureWidth() const {
        return textureWidth;
    }

protected:
    const bool renderToWindow;                                ///< True if renderer should also render to window framebuffer
    const bool countPixels;                                   ///< True if observed pixels should be counted, can be overwritten by argument to display(const bool countPixels)
    ProgramVisualFlat progFlat;                               ///< Shader for 3D rendering without material (used to fill depth buffer)
    ProgramVisibility progVisibility;                         ///< Shader for marking observed texels
    ProgramShowTexture *progShowTexture;                      ///< Shader for rendering observation texture for debugging (only if renderToWindow is true)
    ProgramPixelCounter *progPixelCounter;                    ///< Shadere for counting observed pixels (only if countPixels is true)
    const int width;                                          ///< Width of the framebuffer for rendering the 3D scene in pixels
    const int height;                                         ///< Height of the framebuffer for rendering the 3D scene in pixels
    const int textureWidth;                                   ///< Width of the result texture in pixels
    const int textureHeight;                                  ///< Height of the result texture in pixels
    std::list<GLuint> pixelCounts;                            ///< List of number of observed pixels in the previous frames, see getPixelCounts()
    GLuint pixelCountBuffer;                                  ///< Atomic counter buffer for reading back pixels counts from GPU
    GLuint framebuffers[2];                                   ///< Framebuffers for rendering 3D scene and for rendering visibility texture
    GLuint textures[20];                                      ///< 4 internal textures and up to 16 target textures
    size_t numTextures;                                       ///< Number of allocated teextures
    GLuint vao;                                               ///< Vertex array object
    GLuint vbo;                                               ///< Vertex buffer objeect
    Node * projectionPlaneNode;                               ///< Virtual surface where camera can be placed, only used to hide while rendering
    AbstractCamera * camera;                                  ///< Camera observing the scene
    typedef std::list<std::pair<Node *, GLuint> > Targets;    ///< List of targets (regions of interest) with corresponding texture
    Targets targets;                                          ///< List of targets (regions of interest) with corresponding texture

    enum TextureRole {
        DEPTH = 0,                                            ///< Depth buffer of the 3D scene
        COLOR = 1,                                            ///< Color buffer of the 3D scene, unused
        COLORTEXTURE = 2,                                     ///< Visibility texture for debugging
        COUNTER = 3,                                          ///< Texture for reading back pixel counter
        VISIBILITY = 4                                        ///< Result texture
    };

#ifndef GET_BUFFER_DIRECT
    ProgramCounterToFB *progCounterToFB;                      ///< Shader counting pixels and writing result directly to framebuffer
    GLuint pbo[4];                                            ///< Pixel buffer objects as ring buffer
    const GLuint numPbo;                                      ///< Number of allocated pixel buffer objects
    GLuint curPbo;                                            ///< Current pixel buffer object
    GLuint run;                                               ///< Ring buffer index
    GLuint vaoPoint;                                          ///< Vertex array object for rendering single output pixel
    GLuint vboPoint;                                          ///< Vertex buffer object for rendering single output pixel
    void readBack();                                          ///< Read back ring bufer
#endif

};

}  // namespace gpu_coverage

#endif /* INCLUDE_ARTICULATION_VISIBILITYRENDERER_H_ */
