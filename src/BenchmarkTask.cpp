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

#include <gpu_coverage/BellmanFordRenderer.h>
#include <gpu_coverage/BenchmarkTask.h>
#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/VisibilityRenderer.h>
#include <gpu_coverage/RobotSceneConfiguration.h>
#include <gpu_coverage/Config.h>
#include <gpu_coverage/Utilities.h>

#include <sys/time.h>
#include <sys/mman.h>

namespace gpu_coverage {

BenchmarkTask::TaskSharedData *BenchmarkTask::taskSharedData = NULL;

BenchmarkTask::BenchmarkTask(Scene * const scene, const size_t threadNr, SharedData * const sharedData)
: AbstractTask(sharedData, threadNr), scene(scene),
  costmapRenderer(NULL), bellmanFordRenderer(NULL), bellmanFordXfbRenderer(NULL), bellmanFordRendererToUse(NULL), visibilityRenderer(NULL),
  panoRenderer(NULL), panoEvalRenderer(NULL),
  runtime(10.f), maxIterations(100000)
{
    // Get scene nodes
    Node * const projectionPlane = scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));
    cameraNode = scene->findNode(Config::getInstance().getParam<std::string>("robotCamera"));

    // Create renderers
    costmapRenderer = new CostMapRenderer(scene, projectionPlane, false, false);
    if (!costmapRenderer->isReady()) {
        return;
    }
    bellmanFordRenderer = new BellmanFordRenderer(scene, costmapRenderer, false, false);
    if (!bellmanFordRenderer->isReady()) {
        return;
    }
    bellmanFordRendererToUse = bellmanFordRenderer;

#if USE_XFB
    bellmanFordXfbRenderer = new BellmanFordXfbRenderer(scene, costmapRenderer, false, false);
    if (!bellmanFordXfbRenderer->isReady()) {
        return;
    }
    bellmanFordRendererToUse = bellmanFordXfbRenderer();
#endif

    visibilityRenderer = new VisibilityRenderer(scene, false, true);
    if (!visibilityRenderer->isReady()) {
        return;
    }
    panoRenderer = new PanoRenderer(scene, false, bellmanFordRendererToUse);
    if (!panoRenderer->isReady()) {
        return;
    }
    panoEvalRenderer = new PanoEvalRenderer(scene, false, false, panoRenderer, bellmanFordRendererToUse);
    if (!panoEvalRenderer->isReady()) {
        return;
    }
    panoEvalRenderer->setBenchmark();

    gpu_coverage::Node * const panoCameraNode = scene->findNode("Camera_001");
    if (!panoCameraNode) {
        logError("Camera_001 not found");
        return;
    }
    CameraPanorama * const panoCamera = scene->makePanoramaCamera(panoCameraNode);
    panoRenderer->setCamera(panoCamera);
    panoEvalRenderer->addCamera(panoCamera);

    targetPoints.push_back(glm::vec3(0., 0., 0.));

    ready = true;
}

BenchmarkTask::~BenchmarkTask()
{
}

void BenchmarkTask::prepareCostmap() {
    RobotSceneConfiguration rsc;
    rsc.setRandomArticulation(seed);
    rsc.applyToScene(scene);
}

void BenchmarkTask::prepareBellmanFord() {
    RobotSceneConfiguration rsc;
    rsc.setRandomArticulation(seed);
    rsc.applyToScene(scene);
    costmapRenderer->display();
}

void BenchmarkTask::prepareVisibility() {
    RobotSceneConfiguration rsc;
    rsc.setRandomArticulation(seed);
    rsc.setRandomCameraHeight(seed);
    rsc.setRandomCameraPosition(seed, &targetPoints);
    rsc.applyToScene(scene);
}

void BenchmarkTask::preparePano() {
    static int i = 0;
    RobotSceneConfiguration rsc;
    if (i % 20 == 0 ) {
        rsc.setRandomArticulation(seed);
        rsc.applyToScene(scene);
        costmapRenderer->display();
        bellmanFordRendererToUse->display();
    }
    rsc.setRandomCameraHeight(seed);
    rsc.setRandomCameraPosition(seed, &targetPoints);
    cameraNode->setLocalTransform(rsc.getCameraLocalTransform());
    ++i;
}

void BenchmarkTask::benchmark(const char * const name, AbstractRenderer * const renderer, PrepareFn prepare) {
    pthread_barrier_wait(&sharedData->barrier);

    struct timespec startTime, endTime;
    double ellapsed = 0.;
    size_t iteration;
    std::vector<double> times(maxIterations);
    times[0] = 0.;
    for (iteration = 0; ellapsed - times[0] < runtime && iteration < maxIterations; ++iteration) {
        if (prepare) {
            (this->*(prepare))();
        }
        clock_gettime(CLOCK_MONOTONIC, &startTime);
        renderer->display();
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        const double startT = static_cast<double>(startTime.tv_sec) + static_cast<double>(startTime.tv_nsec) * 1e-9;
        const double endT = static_cast<double>(endTime.tv_sec) + static_cast<double>(endTime.tv_nsec) * 1e-9;
        const double diffT = endT - startT;
        ellapsed += diffT;
        times[iteration] = diffT;
    }

    const double meanDuration = ellapsed / iteration;
    double stdevDuration = 0.0;
    for (size_t i = 0; i < iteration; ++i) {
        stdevDuration += (times[i] - meanDuration) * (times[i] - meanDuration);
    }
    stdevDuration = sqrt(stdevDuration / iteration);

    pthread_barrier_wait(&sharedData->barrier);

    if (threadNr == 0) {
        printf("%s\t%zu\t%f\t%f\t%f\t%f\n", name, iteration, ellapsed, iteration / ellapsed, meanDuration * 1e6, stdevDuration * 1e6);
        for (size_t i = 0; i < iteration; ++i) {
            printf("%f ", times[i]);
        }
        printf("\n");
    }
}

void BenchmarkTask::benchmark2() {
    RobotSceneConfiguration rsc;
    struct timespec times[6];
    double dtimes[6];
    const char *algorithms[5] = {
            "costmap",
            "test",
            "visibility",
            "pano",
            "pano-eval"
    };

    size_t slice = taskSharedData->numIterations / sharedData->numThreads;
    for (size_t iteration = threadNr * slice; iteration < (threadNr + 1) * slice; ++iteration) {
        rsc.setRandomArticulation(seed);
        rsc.setRandomCameraHeight(seed);
        rsc.setRandomCameraPosition(seed, &targetPoints);
        rsc.applyToScene(scene);
        clock_gettime(CLOCK_MONOTONIC, &times[0]);
        costmapRenderer->display();
        clock_gettime(CLOCK_MONOTONIC, &times[1]);
        bellmanFordRendererToUse->display();
        clock_gettime(CLOCK_MONOTONIC, &times[2]);
        visibilityRenderer->display();
        clock_gettime(CLOCK_MONOTONIC, &times[3]);
        panoRenderer->display();
        clock_gettime(CLOCK_MONOTONIC, &times[4]);
        panoEvalRenderer->display();
        clock_gettime(CLOCK_MONOTONIC, &times[5]);
        for (size_t i = 0; i < 6; ++i) {
            dtimes[i] = static_cast<double>(times[i].tv_sec) + static_cast<double>(times[i].tv_nsec) * 1e-9;
            if (i > 0) {
                const double d = dtimes[i] - dtimes[i - 1];
                taskSharedData->data[i-1][iteration] = d;
            }
        }
    }
    pthread_barrier_wait(&sharedData->barrier);

    if (threadNr == 0) {
        double sum[5];
        double mean[5];
        double stdev[5];
        for (size_t i = 0; i < 5; ++i) {
            sum[i] = 0.;
            stdev[i] = 0.;
            for (size_t j = 0; j < taskSharedData->numIterations; ++j) {
                sum[i] += taskSharedData->data[i][j];
            }
            mean[i] = sum[i] / taskSharedData->numIterations;
            for (size_t j = 0; j < taskSharedData->numIterations; ++j) {
                const double d = (taskSharedData->data[i][j] - mean[i]);
                stdev[i] += d * d;
            }
            stdev[i] = sqrt(stdev[i] / taskSharedData->numIterations);
        }
        for (size_t i = 0; i < 5; ++i) {
            printf("%s\t%zu\t%f\t%f\t%f\t%f\n",
                    algorithms[i],
                    taskSharedData->numIterations,
                    sum[i], // duration
                    taskSharedData->numIterations / sum[i],  // FPS
                    mean[i] * 1e6, stdev[i] * 1e6);
            for (size_t j = 0; j < 7; ++j) {
                printf("%f ", taskSharedData->data[i][j]);
            }
            printf("\n");
        }
    }

}


void BenchmarkTask::run() {
    if (threadNr == 0) {
        std::cout << "Algorithm\tframes\tduration [s]\tFPS\tduration mean [us]\tduration stdev [us]" << std::endl;
    }

    benchmark("costmap", costmapRenderer, &BenchmarkTask::prepareCostmap);
    benchmark("bellmanford", bellmanFordRenderer, &BenchmarkTask::prepareBellmanFord);
    //benchmark("bellmanfordxfb", bellmanFordXfbRenderer, &BenchmarkTask::prepareBellmanFord);
    benchmark("visibility", visibilityRenderer, &BenchmarkTask::prepareVisibility);
    benchmark("pano", panoRenderer, &BenchmarkTask::preparePano);
    benchmark("pano-eval", panoEvalRenderer, &BenchmarkTask::preparePano);
    printf("---\n");
    benchmark2();
}

void BenchmarkTask::allocateSharedData() {
    if (!taskSharedData) {
        taskSharedData = (TaskSharedData *) mmap(NULL,
                sizeof(TaskSharedData),
                PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        taskSharedData->numIterations = sizeof(taskSharedData->data[0]) / sizeof(taskSharedData->data[0][0]);
    }
}

void BenchmarkTask::freeSharedData() {
    if (taskSharedData) {
        munmap(taskSharedData, sizeof(TaskSharedData));
        taskSharedData = NULL;
    }
}

} /* namespace gpu_coverage */
