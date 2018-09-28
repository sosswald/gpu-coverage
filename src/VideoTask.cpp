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

#include <gpu_coverage/BellmanFordXfbRenderer.h>
#include <gpu_coverage/VideoTask.h>
#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/PanoRenderer.h>
#include <gpu_coverage/VisibilityRenderer.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Config.h>

namespace gpu_coverage {

VideoTask::VideoTask(Scene * const scene, const size_t threadNr, SharedData * const sharedData, const size_t startFrame,
        const size_t endFrame)
        : AbstractTask(sharedData, threadNr), scene(scene), startFrame(startFrame), endFrame(endFrame), captureFrame(
                (startFrame + endFrame) / 2),
                writeVideo(true), costmapTexture(NULL), outputVideo(NULL), outputCubemapVideo(NULL)
{
    // Add pano camera to scene
    scene->makePanoramaCamera(scene->findCamera(Config::getInstance().getParam<std::string>("panoCamera"))->getNode());

    // Get scene nodes
    Node * const projectionPlane = scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));
    Material * const costmapMaterial = projectionPlane->getMeshes().front()->getMaterial();
    if (!costmapMaterial) {
        logError("Could not find costmap material");
        return;
    }

    // Create renderers
    CostMapRenderer * const costmapRenderer = new CostMapRenderer(scene, projectionPlane, false, false);
    BellmanFordXfbRenderer * const bellmanFordRenderer = new BellmanFordXfbRenderer(scene, costmapRenderer, false, false);
    Renderer * const renderer = new Renderer(scene, false, true);
    VisibilityRenderer * const visibilityRenderer = new VisibilityRenderer(scene, false, false);
    PanoRenderer *panoRenderer = new PanoRenderer(scene, false, bellmanFordRenderer);
    renderers.push_back(costmapRenderer);
    renderers.push_back(bellmanFordRenderer);
    renderers.push_back(visibilityRenderer);
    renderers.push_back(renderer);
    renderers.push_back(panoRenderer);

    panoTexture = panoRenderer->getTexture();

    for (Renderers::const_iterator rendererIt = renderers.begin(); rendererIt != renderers.end(); ++rendererIt) {
        if (!(*rendererIt)->isReady()) {
            return;
        }
    }

    // Create links
    costmapTexture = new Texture(bellmanFordRenderer->getTexture());
    costmapMaterial->setTexture(costmapTexture);

    pbufferHeight = 1080 / 2;
    pbufferWidth = 1920 / 2;
    panoWidth = panoRenderer->getTextureWidth();
    panoHeight = panoRenderer->getTextureHeight();
    mat = cv::Mat(pbufferHeight, pbufferWidth, CV_8UC3);
    flip = cv::Mat(pbufferHeight, pbufferWidth, CV_8UC3);
    panoMat = cv::Mat(panoHeight, panoWidth, CV_8UC3);
    panoFlip = cv::Mat(panoHeight, panoWidth, CV_8UC3);
    if (writeVideo) {
        char filename[256];
        snprintf(filename, sizeof(filename), "/tmp/output_%zu.avi", threadNr);
        outputVideo = new cv::VideoWriter(filename, CV_FOURCC('D', 'I', 'V', 'X'), 30.,
                cv::Size(pbufferWidth, pbufferHeight), true);
        if (!outputVideo->isOpened()) {
            fprintf(stderr, "Could not open output video file\n");
            return;
        }
        snprintf(filename, sizeof(filename), "/tmp/cubemap_%zu.avi", threadNr);
        outputCubemapVideo = new cv::VideoWriter(filename, CV_FOURCC('D', 'I', 'V', 'X'), 30.,
                cv::Size(panoWidth, panoHeight), true);
        if (!outputCubemapVideo->isOpened()) {
            fprintf(stderr, "Could not open cubemap output video file\n");
            return;
        }
    }
    ready = true;
}

VideoTask::~VideoTask() {
    if (outputVideo) {
        outputVideo->release();
    }
    if (outputCubemapVideo) {
        outputCubemapVideo->release();
    }
    for (Renderers::const_iterator rendererIt = renderers.begin(); rendererIt != renderers.end(); ++rendererIt) {
        delete *rendererIt;
    }
    renderers.clear();
    delete costmapTexture;
}

void VideoTask::run() {
    if (!ready) {
        return;
    }
    char filename[256];
    for (size_t curFrame = startFrame; curFrame < endFrame; ++curFrame) {
        for (Renderers::const_iterator rendererIt = renderers.begin(); rendererIt != renderers.end(); ++rendererIt) {
            //TODO set channels
            (*rendererIt)->display();
        }
        glFlush();
        if (writeVideo) {
            glReadPixels(0, 0, pbufferWidth, pbufferHeight, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
            cv::flip(mat, flip, 0);
            outputVideo->write(flip);

            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, panoTexture);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, panoMat.data);
            cv::flip(panoMat, panoFlip, 0);
            outputCubemapVideo->write(panoFlip);
        }
        if (curFrame == captureFrame) {
            snprintf(filename, sizeof(filename), "/tmp/output_%zu.png", threadNr);
            glReadPixels(0, 0, pbufferWidth, pbufferHeight, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
            cv::flip(mat, flip, 0);
            cv::imwrite(filename, flip);

            snprintf(filename, sizeof(filename), "/tmp/cubemap_%zu.png", threadNr);
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_2D, panoTexture);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, panoMat.data);
            cv::flip(panoMat, panoFlip, 0);
            cv::imwrite(filename, panoFlip);
        }
    }
}

} /* namespace gpu_coverage */
