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
#include <gpu_coverage/UtilityAnimationTask.h>
#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/PanoRenderer.h>
#include <gpu_coverage/VisibilityRenderer.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Config.h>
#include <gpu_coverage/Channel.h>

namespace gpu_coverage {

UtilityAnimationTask::UtilityAnimationTask(Scene * const scene, const size_t threadNr, SharedData * const sharedData)
        : AbstractTask(sharedData, threadNr), scene(scene), costmapRenderer(NULL), bellmanFordRenderer(NULL), panoRenderer(NULL), panoEvalRenderer(NULL)

{
    const char *filename = "/home/sosswald/papers/osswald18iros/video/sources/utility-animation.txt";
    outdir = "/home/sosswald/papers/osswald18iros/video/sources/utility-animation/";

    // Get scene nodes
    Node * const projectionPlane = scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));
    Material * const costmapMaterial = projectionPlane->getMeshes().front()->getMaterial();
    if (!costmapMaterial) {
        logError("Could not find costmap material");
        return;
    }


    // Create renderers
    costmapRenderer = new CostMapRenderer(scene, projectionPlane, false, true);
    bellmanFordRenderer = new BellmanFordXfbRenderer(scene, costmapRenderer, false, true);
    panoRenderer = new PanoRenderer(scene, false, bellmanFordRenderer);
    panoEvalRenderer = new PanoEvalRenderer(scene, false, true, panoRenderer, bellmanFordRenderer);

    panoTexture = panoEvalRenderer->getUtilityMapVisual();

    if (!costmapRenderer->isReady() || !bellmanFordRenderer->isReady() || !panoRenderer->isReady() || !panoEvalRenderer->isReady()) {
        return;
    }


    ifs.open(filename);
    if (!ifs.good()) {
        logError("Could not open input file %s", filename);
        return;
    }
    char line[2048];
    ifs.getline(line, sizeof(line));
    std::stringstream ss(line);
    std::string dummy;
    ss >> dummy;
    while(ss.good()) {
        std::string n;
        ss >> n;
        if (!n.empty()) {
            const Scene::Channels& channels = scene->getChannels();
            bool found = false;
            for (size_t i = 0; i < channels.size(); ++i) {
                if (channels[i]->getNode()->getName() == n) {
                    channelHandles.push_back(i);
                    found = true;
                }
            }
            if (!found) {
                logError("Could not find animation channel %s", n.c_str());
                return;
            }
        }
    }

    // Add pano cameras to scene
    const std::string panoCameraNames = Config::getInstance().getParam<std::string>("panoCamera");
    std::istringstream iss(panoCameraNames);
    std::string panoCameraName;
    bool isPair = false;
    CameraPanorama * first = NULL;
    CameraPanorama * second = NULL;
    while (!iss.eof()) {
        std::getline(iss, panoCameraName, ' ');
        if (panoCameraName.compare("(") == 0) {
            isPair = true;
        } else if (panoCameraName.compare(")") == 0) {
            if (first && second) {
                panoEvalRenderer->addCameraPair(first, second);
            } else {
                logError("Panorama camera pair with less than two cameras");
                return;
            }
            isPair = false;
            first = NULL;
            second = NULL;
        } else if (!panoCameraName.empty()) {
            gpu_coverage::Node * const cameraNode = scene->findNode(panoCameraName);
            if (!cameraNode) {
                logError("Could not find panorama camera %s", panoCameraName.c_str());
                return;
            }
            CameraPanorama * const panoCam = scene->makePanoramaCamera(cameraNode);
            scene->makePanoramaCamera(scene->findNode(panoCameraName));
            if (isPair) {
                if (first == NULL) {
                    first = panoCam;
                } else if (second == NULL) {
                    second = panoCam;
                } else {
                    logError("More than two cameras in panorama camera pair");
                }
            } else {
                panoEvalRenderer->addCamera(panoCam);
            }
        }
    }

    rsc.setCameraLocalTransform(scene->findNode(Config::getInstance().getParam<std::string>("robotCamera"))->getLocalTransform());
    ready = true;
}

UtilityAnimationTask::~UtilityAnimationTask() {
}

void UtilityAnimationTask::run() {
    if (!ready) {
        return;
    }
    cv::Mat mat(bellmanFordRenderer->getTextureHeight(), bellmanFordRenderer->getTextureWidth(), CV_8UC3);
    cv::Mat flip(bellmanFordRenderer->getTextureHeight(), bellmanFordRenderer->getTextureWidth(), CV_8UC3);

    char filename[1024];
    while(ifs.good()) {
        int frame = 0;
        ifs >> frame;
        if (frame == 0) {
            break;
        }
        for (size_t i = 0; i < channelHandles.size(); ++i) {
            float p;
            ifs >> p;
            rsc.setArticulation(channelHandles[i], p);
        }
        rsc.applyToScene(scene);
        costmapRenderer->display();
        bellmanFordRenderer->display();
        panoEvalRenderer->display();
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, panoTexture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, mat.data);
        cv::flip(mat, flip, 0);
        snprintf(filename, sizeof(filename), "%s/%06d.png", outdir.c_str(), frame);
        cv::imwrite(filename, flip);
        checkGLError();
    }
}

} /* namespace gpu_coverage */
