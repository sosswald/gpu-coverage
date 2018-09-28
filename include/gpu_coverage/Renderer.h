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

#ifndef INCLUDE_ARTICULATION_RENDERER_H_
#define INCLUDE_ARTICULATION_RENDERER_H_

#include <gpu_coverage/AbstractRenderer.h>
#include <gpu_coverage/CameraPerspective.h>
#include <gpu_coverage/CoordinateAxes.h>
#include <gpu_coverage/Programs.h>

namespace gpu_coverage {

/**
 * @brief %Renderer for rendering a 3D scene using textures and materials.
 */
class Renderer: public AbstractRenderer {
public:
	/**
	 * @brief Constructor.
	 * @param scene The scene to be rendered.
	 * @param renderToWindow True if scene should be rendered to the window framebuffer.
	 * @param renderToTexture True if scene should be rendered off-screen to texture.
	 *
	 * It is possible to render to both the window framebuffer and off-screen to texture.
	 */
    Renderer(const Scene * const scene, const bool renderToWindow, const bool renderToTexture);

    /**
     * @brief Destructor.
     */
    virtual ~Renderer();

    /**
     * @brief Renders the scene.
     *
     * This method does the main work of the renderer.
     */
    virtual void display();

    /**
     * @brief Returns the OpenGL texture ID of the result texture.
     * @return OpenGL texture ID.
     */
    inline const GLuint& getTexture() const {
        return textures[COLOR];
    }

    /**
     * @brief Height of the result texture.
     * @return Height in pixels.
     */
    inline const int& getTextureHeight() const {
        return height;
    }

    /**
     * @brief Width of the result texture.
     * @return Width in pixels.
     */
    inline const int& getTextureWidth() const {
        return width;
    }

    /**
     * @brief Change the active camera used for rendering.
     * @param cameraNew The new camera.
     */
    void setCamera(CameraPerspective * cameraNew) {
        camera = cameraNew;
    }

protected:
    const bool renderToWindow;         ///< True if renderer should render to the window framebufer.
    const bool renderToTexture;        ///< True if renderer should render to an off-screen texture.
    const int width;                   ///< Width of the off-screen texture in pixels.
    const int height;                  ///< Height of the off-screen texture in pixels.

    CameraPerspective *camera;         ///< Active camera used for rendering.

    ProgramVisualTexture progVisualTexture;           ///< Shader program for rendering a mesh with materials and/or textures.
    ProgramVisualVertexcolor progVisualVertexcolor;   ///< Shader program for rendering a mesh with per-vertex colors.
    ProgramShowTexture *progShowTexture;              ///< Program for rendering a texture to a framebuffer.
    CoordinateAxes coordinateAxes;                    ///< Coordinate axes scene object.

    GLuint framebuffer;                ///< Framebuffer for offscreen rendering
    GLuint textures[2];                ///< Color and depth textures for offscreen rendering
    GLuint vao;                        ///< Vertex array object.
    GLuint vbo;                        ///< Vertex buffer object.

    /**
     * @brief Texture roles.
     */
    enum TextureRole {
        COLOR = 0,                     ///< Color buffer.
        DEPTH = 1                      ///< Depth buffer.
    };

};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_RENDERER_H_ */
