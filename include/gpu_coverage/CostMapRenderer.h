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

#ifndef INCLUDE_ARTICULATION_COSTMAPRENDERER_H_
#define INCLUDE_ARTICULATION_COSTMAPRENDERER_H_

#include <gpu_coverage/AbstractRenderer.h>
#include <gpu_coverage/CameraOrtho.h>
#include <gpu_coverage/CoordinateAxes.h>

namespace gpu_coverage {

class CostMapRenderer: public AbstractRenderer {
public:
    CostMapRenderer(const Scene * const scene, const Node * const projectionPlane,
            const bool renderToWindow, const bool visual);
    virtual ~CostMapRenderer();

    virtual void display();
    inline const GLuint& getTexture() const {
        return textures[OUTPUT];
    }
    inline const GLuint& getVisualTexture() const {
        return textures[VISUAL];
    }
    inline const int& getTextureWidth() const {
        return width;
    }
    inline const int& getTextureHeight() const {
        return height;
    }
    inline const AbstractCamera * getCamera() const {
        return camera;
    }

protected:
    const bool renderToWindow;
    const bool renderVisual;
    AbstractCamera *camera;
    Node * cameraNode;
    Node * floorNode;
    Node * floorProjectionNode;
    Node * planeNode;
    ProgramDistanceMapSeed progSeed;
    ProgramDistanceMapJFA progJFA;
    ProgramCostMap progCostMap;
    ProgramCostMapVisual * progCostMapVisual;
    ProgramShowTexture *progShowTexture;
    const int width, height;
    GLuint framebuffer;
    GLuint textures[4];
    GLuint vao;
    GLuint vbo;
    enum TextureRole {
        SWAP1 = 0,
        SWAP2 = 1,
        OUTPUT = 2,
        VISUAL = 3
    };
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_COSTMAPRENDERER_H_ */
