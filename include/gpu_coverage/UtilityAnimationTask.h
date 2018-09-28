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

#ifndef INCLUDE_ARTICULATION_UTILITY_ANIMATION_TASK_H
#define INCLUDE_ARTICULATION_UTILITY_ANIMATION_TASK_H

#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/PanoRenderer.h>
#include <gpu_coverage/PanoEvalRenderer.h>
#include <gpu_coverage/AbstractTask.h>
#include <gpu_coverage/BellmanFordXfbRenderer.h>
#include <gpu_coverage/Scene.h>
#include <gpu_coverage/RobotSceneConfiguration.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <vector>
#include <fstream>

namespace gpu_coverage {

class UtilityAnimationTask: public AbstractTask {
public:
    UtilityAnimationTask(Scene * const scene, const size_t threadNr, SharedData * const sharedData);
    virtual ~UtilityAnimationTask();

    virtual void run();

protected:
    Scene * const scene;
    std::ifstream ifs;
    GLuint panoTexture;

    CostMapRenderer *costmapRenderer;
    BellmanFordXfbRenderer *bellmanFordRenderer;
    PanoRenderer *panoRenderer;
    PanoEvalRenderer *panoEvalRenderer;

    std::vector<size_t> channelHandles;
    RobotSceneConfiguration rsc;
    std::string outdir;
};

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_UTILITY_ANIMATION_TASK_H */
