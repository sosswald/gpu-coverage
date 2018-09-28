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

#ifndef INCLUDE_ARTICULATION_BELLMANFORDRENDERER_H_
#define INCLUDE_ARTICULATION_BELLMANFORDRENDERER_H_

#include <gpu_coverage/AbstractRenderer.h>
#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/Dot.h>

namespace gpu_coverage {

class BellmanFordRenderer: public AbstractRenderer {
public:
    BellmanFordRenderer(const Scene * const scene, const CostMapRenderer * const costmapRenderer,
            const bool renderToWindow, const bool visual);
    ~BellmanFordRenderer();

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

    inline void setRobotPosition(const glm::mat4x4& worldTransform) {
        robotWorldTransform = worldTransform;
    }

protected:
    const CostMapRenderer * const costmapRenderer;
    const bool renderToWindow;
    const bool renderVisual;
    ProgramBellmanFordInit progInit;
    ProgramBellmanFordStep progStep;
    ProgramCounterToFB progCounterToFB;
    ProgramShowTexture *progShowTexture;
    ProgramVisualizeIntTexture *progVisualizeIntTexture;
    const int width, height;
    const size_t maxIterations;
    GLuint framebuffer;
    GLuint textures[5];
    GLuint vao;
    GLuint vbo;
    GLuint counterBuffer;
    GLuint pbo[4];
    const size_t numPbo;
    Dot robotSeedDot;
    glm::mat4x4 robotWorldTransform;
    glm::vec4 clearColor;
    enum TextureRole {
        SWAP1 = 0,
        SWAP2 = 1,
        COUNTER = 2,   // 1x1 texture for counter transfer to PBO
        OUTPUT = 3,    // output as R32I
        VISUAL = 4     // output color-coded
    };
};

}  // namespace gpu_coverage

#endif /* INCLUDE_ARTICULATION_BELLMANFORDRENDERER_H_ */
