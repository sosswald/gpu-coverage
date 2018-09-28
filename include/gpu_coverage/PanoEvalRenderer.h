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

#ifndef INCLUDE_ARTICULATION_PanoEvalRenderer_H_
#define INCLUDE_ARTICULATION_PanoEvalRenderer_H_

#include <gpu_coverage/Programs.h>
#include <gpu_coverage/AbstractRenderer.h>
#include <gpu_coverage/BellmanFordRenderer.h>
#include <gpu_coverage/PanoRenderer.h>
#include <list>

namespace gpu_coverage {

class PanoEvalRenderer: public AbstractRenderer {
public:
    PanoEvalRenderer(const Scene * const scene, const bool renderToWindow, const bool renderToTexture,
            PanoRenderer * const panoRenderer, AbstractRenderer * const bellmanFordRenderer);
    virtual ~PanoEvalRenderer();
    virtual void display();
    inline const GLuint& getTexture() const {
        return textures[EVAL];
    }
    inline const int& getTextureWidth() const {
        return panoWidth;
    }
    inline const int& getTextureHeight() const {
        return panoHeight;
    }
    inline const GLuint& getUtilityMap() const {
        return textures[curUtilityMap];
    }
    inline const GLuint& getUtilityMapVisual() const {
        return textures[UTILITY_MAP_VISUAL];
    }

    inline void setBenchmark() {
        benchmark = true;
    }

    void addCamera(CameraPanorama * const cam);
    void addCameraPair(CameraPanorama * const first, CameraPanorama * const second);

protected:
    const bool renderToWindow;
    const bool renderToTexture;
    bool benchmark;
    PanoRenderer * const panoRenderer;
    ProgramVisualizeIntTexture *progVisualizeIntTexture;
    ProgramTLEdge progTLEdge;
    ProgramTLStep progTLStep;
    ProgramCounterToFB progCounterToFB;
    ProgramPanoEval progPanoEval;
    ProgramUtility1 progUtility1;
    ProgramUtility2 progUtility2;
    ProgramShowTexture *progShowTexture;

    GLuint framebuffer;
    GLuint vao;
    GLuint vbo;
    GLuint textures[9];

    GLuint counterBuffer;
    GLuint pbo[3];
    const size_t numPbo;
    const size_t maxIterations;

    GLuint panoTexture;
    int panoWidth;
    int panoHeight;

    GLuint bellmanFordTexture;
    GLuint bellmanFordWidth;
    GLuint bellmanFordHeight;

    enum TextureRole {
        EVAL,
        SWAP1,
        SWAP2,
        GAIN1,
        GAIN2,
        UTILITY_MAP_1,
        UTILITY_MAP_2,
        UTILITY_MAP_VISUAL,
        COUNTER
    } textureToVisualize, curUtilityMap;

    struct PanoEdge {
        CameraPanorama * camera;
        bool clockwise;
        PanoEdge(CameraPanorama * const camera, bool clockwise) : camera(camera), clockwise(clockwise) {}
    };
    typedef std::pair<PanoEdge, PanoEdge> PanoEdgePair;
    typedef std::list<PanoEdgePair> PanoEdgePairs;
    PanoEdgePairs panoEdgePairs;

    Node * targetNode;
    Node * projectionPlaneNode;

    bool link(const GLuint program, const char * const name) const;

};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_PanoEvalRenderer_H_ */
