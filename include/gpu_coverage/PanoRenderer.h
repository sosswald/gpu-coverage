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

#ifndef INCLUDE_ARTICULATION_PANORENDERER_H_
#define INCLUDE_ARTICULATION_PANORENDERER_H_

#include <gpu_coverage/Programs.h>
#include <gpu_coverage/AbstractRenderer.h>
#include <gpu_coverage/Config.h>
#include <list>

namespace gpu_coverage {

class CameraPanorama;

class PanoRenderer: public AbstractRenderer {
public:
    PanoRenderer(const Scene * const scene, const bool renderToWindow, AbstractRenderer * const bellmanFordRenderer);
    virtual ~PanoRenderer();
    virtual void display();
    inline const GLuint& getTexture() const {
        return textures[PANO];
    }
    inline const GLuint& getCostmapIndexTexture() const {
        return textures[COSTMAP_INDEX];
    }
    inline const int& getTextureWidth() const {
        return panoWidth;
    }
    inline const int& getTextureHeight() const {
        return panoHeight;
    }
    inline void setCamera(CameraPanorama * const camera) {
        this->camera = camera;
    }

protected:
    const bool renderToWindow;
    const bool renderSemantic;
    CameraPanorama * camera;
    ProgramPano *progPano;
    ProgramPanoSemantic *progPanoSemantic;
    ProgramCostmapIndex *progCostmapIndex;
    const int cubemapWidth, cubemapHeight;
    int panoWidth, panoHeight;
    GLuint framebuffer;
    Config::PanoOutputValue panoOutputFormat;
    bool renderToCubemap;

    AbstractProgramMapProjection *progMapProjection;
    ProgramShowTexture progShowTexture;
    GLuint mapProjectionVao;
    GLuint mapProjectionVbo;
    GLsizei mapProjectionCount;

    const bool debug;
    GLuint depthCubeMap, colorCubeMap;

    GLuint vao;
    GLuint vbo;
    GLuint textures[2];

    enum TextureRole {
        PANO,
        COSTMAP_INDEX
    };

    typedef std::list<Node *> TargetNodes;
    TargetNodes targetNodes;
    Node * projectionPlaneNode;
    Node * floorNode;
    AbstractRenderer * bellmanFordRenderer;

    bool link(const GLuint program, const char * const name) const;

};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_PANORENDERER_H_ */
