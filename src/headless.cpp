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

#include <gpu_coverage/VideoTask.h>
#include <gpu_coverage/RandomSearchTask.h>
#include <gpu_coverage/HillclimbingTask.h>
#include <gpu_coverage/BenchmarkTask.h>
#include <gpu_coverage/UtilityMapSystematicTask.h>
#include <gpu_coverage/UtilityAnimationTask.h>
#include <gpu_coverage/Utilities.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>

#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <unistd.h>
#include <pthread.h>

#include <iostream>
#include <cstdio>
#include <list>
#include <set>

#ifndef DATADIR
#define DATADIR "."
#endif

#ifdef HAS_NVML
#include <nvml.h>
#endif

using namespace gpu_coverage;

static const int MAX_DEVICES = 16;
static const EGLint configAttribs[] = {
EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
EGL_BLUE_SIZE, 8,
EGL_GREEN_SIZE, 8,
EGL_RED_SIZE, 8,
EGL_DEPTH_SIZE, 8,
//EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
EGL_NONE
};

namespace gpu_coverage {
enum Command {
    VIDEO,
    RANDOM,
    HILLCLIMBING,
    BENCHMARK,
    UTILITY_MAP_SYSTEMATIC,
    UTILITY_ANIMATION,
    NONE
};

static struct ConfigData {
    std::string configFile;
    const aiScene * ai_scene;
    Command task;
    size_t numDevices;
    size_t randomIterations;
    size_t randomArticulationConfigs;
    size_t randomCameraPoses;
    bool setSeed;
    unsigned int seed;
} configData;

struct ThreadData {
    int threadNr;
    int deviceNr;
};
}  // namespace gpu_coverage

SharedData * sharedData;

#ifdef DEBUG
static void APIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, const GLchar* message, const void *userParam) {
    const int threadNr = *static_cast<const int*>(userParam);
    /*if (source == GL_DEBUG_SOURCE_API_ARB && type == GL_DEBUG_TYPE_OTHER_ARB && severity == GL_DEBUG_SEVERITY_LOW) {
     // do not print buffer info messages
     return;
     }
     if (source == GL_DEBUG_SOURCE_API_ARB && type == GL_DEBUG_TYPE_PERFORMANCE_ARB && severity == GL_DEBUG_SEVERITY_MEDIUM) {
     // do not print buffer performance messages
     return;
     }*/
    if (type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP) {
        // do not print debug group push/pop
        return;
    }
    if (strncmp(message, "Buffer detailed info:", 21) == 0) {
        return;
    }

    pthread_mutex_lock(&sharedData->mutex);
    bool print = true;
    switch(severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION: logInfo("[%d] [NOTE] %s", threadNr, message); break;
        case GL_DEBUG_SEVERITY_LOW: logInfo("[%d] [LOW ] %s", threadNr, message); break;
        case GL_DEBUG_SEVERITY_MEDIUM: logWarn("[%d] [MED ] %s", threadNr, message); break;
        case GL_DEBUG_SEVERITY_HIGH: logError("[%d] [HIGH] %s", threadNr, message); break;
    }
    pthread_mutex_unlock(&sharedData->mutex);
}
#endif

void *doWork(void * voidptr) {
    ThreadData *data = (ThreadData *) voidptr;

    pthread_mutex_lock(&sharedData->mutex);

    bool ok = true;

    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
            (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");

    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT =
            (PFNEGLQUERYDEVICESEXTPROC) eglGetProcAddress("eglQueryDevicesEXT");

    EGLint error = eglGetError();
    if (error != EGL_SUCCESS) {
        logError("[%d] EGL error before querying devices: %d", data->threadNr, error);
        ok = false;
    }

    EGLint totalDevices;
    EGLDeviceEXT eglDevs[MAX_DEVICES];
    EGLDisplay eglDpy = EGL_NO_DISPLAY;
    EGLSurface eglSurf = EGL_NO_SURFACE;
    EGLContext eglCtx;
    EGLConfig eglCfg;

    // 1. Initialize EGL
    if (eglQueryDevicesEXT && eglGetPlatformDisplayEXT) {
        eglQueryDevicesEXT(MAX_DEVICES, eglDevs, &totalDevices);
        if (eglGetError() == EGL_SUCCESS) {
            eglDpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevs[data->deviceNr], 0);

            if (eglDpy == EGL_NO_DISPLAY) {
                logError("[%d] EGL_NO_DISPLAY", data->threadNr);
            }
            if (eglGetError() != EGL_SUCCESS) {
                logError("[%d] Could not create display", data->threadNr);
                ok = false;
            }
        } else {
            logError("[%d] Could not query devices", data->threadNr);
            ok = false;
        }
    } else {
        eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }


    if (ok) {
        EGLint major, minor;
        if (!eglInitialize(eglDpy, &major, &minor)) {
            logError("[%d] Could not initialize EGL: %d", data->threadNr, eglGetError());
            ok = false;
        } else {
            //logInfo("[%d] EGL version %d.%d", data->threadNr, major, minor);
        }
    }

    // 2. Select an appropriate configuration
    if (ok) {
        EGLint numConfigs;
        if (!eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs)) {
            ok = false;
        } else if (numConfigs == 0) {
            logError("[%d] Did not find suitable display config", data->threadNr);
            ok = false;
        }
    }

    // 3. Create surfaces
    if (ok) {
        /*eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg, pbufferAttribs);
          if (eglSurf == 0) {
          logError("[%d] Could not create surface: %d", data->threadNr, eglGetError());
          ok = false;
        }*/
        eglSurf = EGL_NO_SURFACE;
    }

    // 4. Bind the API
    if (ok) {
        if (!eglBindAPI(GET_API(EGL))) {
            logError("[%d] Could not bind API: %d", data->threadNr, eglGetError());
            ok = false;
        }
        EGLenum api = eglQueryAPI();
        logInfo("[%d] API = OpenGL %s", data->threadNr, api == EGL_OPENGL_ES_API ? "ES" : "");
    }

    // 5. Create a context
    if (ok) {
        EGLint const attrib_list[] = {
                EGL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR,
                EGL_CONTEXT_MINOR_VERSION, OPENGL_MINOR,
                EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                EGL_NONE
        };
        eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT, attrib_list);
        if (eglCtx == 0) {
            logError("[%d] Could not create context: %d", data->threadNr, eglGetError());
            ok = false;
        }
    }

    // 6. Make context current
    if (ok) {
        if (eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx)) {
        } else {
            ok = false;
            logError("[%d] Could not make context current: %d", data->threadNr, eglGetError());
        }
    }

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glDebugCallback, &data->threadNr);
#endif

    error = eglGetError();
    if (error != EGL_SUCCESS) {
        logError("[%d] eglError %d", data->threadNr, error);
        ok = false;
    }

    // Load scene and create task
    AbstractTask *task = NULL;
    if (ok) {
        AbstractProgram::setOpenGLVersion();
        Scene * const scene = new Scene(configData.ai_scene, DATADIR "/models");

        const size_t framePart = scene->getNumFrames() / configData.numDevices;
        const size_t startFrame = data->threadNr * framePart;
        const size_t endFrame = startFrame + framePart;

        switch(configData.task) {
        case VIDEO:
            task = new VideoTask(scene, data->threadNr, sharedData, startFrame, endFrame);
            break;
        case RANDOM:
            task = new RandomSearchTask(scene, data->threadNr, sharedData, configData.randomIterations,
                    configData.randomArticulationConfigs, configData.randomCameraPoses);
            break;
        case HILLCLIMBING:
            task = new HillclimbingTask(scene, data->threadNr, sharedData);
            break;
        case BENCHMARK:
            task = new BenchmarkTask(scene, data->threadNr, sharedData);
            break;
        case UTILITY_MAP_SYSTEMATIC:
            task = new UtilityMapSystematicTask(scene, data->threadNr, sharedData, startFrame, endFrame);
            break;
        case UTILITY_ANIMATION:
            task = new UtilityAnimationTask(scene, data->threadNr, sharedData);
            break;
        default:
            logError("Unknown task");
            break;
        }
        if (configData.setSeed) {
            task->setSeed(configData.seed + data->threadNr);
        }
    }

    pthread_mutex_unlock(&sharedData->mutex);
    pthread_barrier_wait(&sharedData->barrier);

    if (task && task->isReady()) {
        struct timeval time;
        gettimeofday(&time, NULL);
        const double startTime = static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_usec) * 1e-6;

        task->run();

        gettimeofday(&time, NULL);
        const double endTime = static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_usec) * 1e-6;
        pthread_mutex_lock(&sharedData->mutex);
        try {
            logInfo("[%d] Elapsed time: %.6f seconds.", data->threadNr, endTime - startTime);
        } catch (...) {
        }
        pthread_mutex_unlock(&sharedData->mutex);
    } else {
        pthread_mutex_lock(&sharedData->mutex);
        try {
            logError("[%d] Task not ready", data->threadNr);
        } catch (...) {
        }
        pthread_mutex_unlock(&sharedData->mutex);
    }

    pthread_barrier_wait(&sharedData->barrier);

    if (ok && task->isReady()) {
        task->finish();
    }
    eglMakeCurrent(NULL, NULL, NULL, NULL);

    // 8. Terminate EGL when finished
    if (eglDpy != 0) {
        eglTerminate(eglDpy);
    }

    delete task;

    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_setname_np(pthread_self(), "main");
    bool showHelp = false;
    bool argError = false;
    bool checkGpuFree = false;
    bool multiProcess = true;
    std::set<unsigned int> limitDevices;
    configData.configFile = std::string("config/config.txt");
    configData.randomIterations = 10;
    configData.randomArticulationConfigs = 10;
    configData.randomCameraPoses = 100;
    configData.setSeed = false;
    configData.seed = 0;
    configData.task = NONE;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "video") == 0) {
            configData.task = VIDEO;
        } else if (strcmp(argv[i], "random") == 0) {
            configData.task = RANDOM;
        } else if (strcmp(argv[i], "hillclimbing") == 0) {
            configData.task = HILLCLIMBING;
        } else if (strcmp(argv[i], "benchmark") == 0) {
            configData.task = BENCHMARK;
        } else if (strcmp(argv[i], "utility") == 0) {
            configData.task = UTILITY_MAP_SYSTEMATIC;
        } else if (strcmp(argv[i], "utilityanimation") == 0) {
           configData.task = UTILITY_ANIMATION;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--devices") == 0) {
            if (i < argc - 1) {
                ++i;
                char *p = strtok(argv[i], ",");
                while (p != NULL) {
                    if (!isdigit(p[0])) {
                        std::cerr << "Error: Could not parse device number " << p << std::endl;
                        showHelp = true;
                        argError = true;
                        break;
                    }
                    const int d = atoi(p);
                    limitDevices.insert(d);
                    p = strtok(NULL, ",");
                }
                if (argError) {
                    break;
                }
            } else {
                std::cerr << "Error: " << argv[i] << " must be followed by a device number." << std::endl;
                showHelp = true;
                argError = true;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            showHelp = true;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--seed") == 0) {
            if (i < argc - 1) {
                ++i;
                if (strlen(argv[i]) == 0 || !isdigit(argv[i][0])) {
                    std::cerr << "Error: Seed must be an unsigned integer, got " << argv[i] << std::endl;
                    showHelp = true;
                    argError = true;
                    break;
                }
                configData.setSeed = true;
                configData.seed = atoi(argv[i]);
            } else {
                std::cerr << "Error: " << argv[i] << " must be followed by a seed." << std::endl;
                showHelp = true;
                argError = true;
            }
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--articulations") == 0) {
            if (i < argc - 1) {
                ++i;
                if (strlen(argv[i]) == 0 || !isdigit(argv[i][0])) {
                    std::cerr << "Error: Number of articulation configs must be an unsigned integer, got " << argv[i]
                            << std::endl;
                    showHelp = true;
                    argError = true;
                    break;
                }
                configData.randomArticulationConfigs = atoi(argv[i]);
            } else {
                std::cerr << "Error: " << argv[i] << " must be followed by an integer." << std::endl;
                showHelp = true;
                argError = true;
            }
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cameras") == 0) {
            if (i < argc - 1) {
                ++i;
                if (strlen(argv[i]) == 0 || !isdigit(argv[i][0])) {
                    std::cerr << "Error: Number of camera poses must be an unsigned integer, got " << argv[i]
                            << std::endl;
                    showHelp = true;
                    argError = true;
                    break;
                }
                configData.randomCameraPoses = atoi(argv[i]);
            } else {
                std::cerr << "Error: " << argv[i] << " must be followed by an integer." << std::endl;
                showHelp = true;
                argError = true;
            }
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--iterations") == 0) {
            if (i < argc - 1) {
                ++i;
                if (strlen(argv[i]) == 0 || !isdigit(argv[i][0])) {
                    std::cerr << "Error: Number of iterations must be an unsigned integer, got " << argv[i]
                            << std::endl;
                    showHelp = true;
                    argError = true;
                    break;
                }
                configData.randomIterations = atoi(argv[i]);
            } else {
                std::cerr << "Error: " << argv[i] << " must be followed by an integer." << std::endl;
                showHelp = true;
                argError = true;
            }
        } else if (strcmp(argv[i], "--check-gpu-free") == 0) {
            checkGpuFree = true;
        } else if (strcmp(argv[i], "--threads") == 0) {
            multiProcess = false;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i < argc - 1) {
                ++i;
                if (strlen(argv[i]) == 0) {
                    std::cerr << "Error: Config file name must be a string, got " << argv[i]
                            << std::endl;
                    showHelp = true;
                    argError = true;
                    break;
                }
                configData.configFile = std::string(argv[i]);
            } else {
                std::cerr << "Error: " << argv[i] << " must be followed by a filename." << std::endl;
                showHelp = true;
                argError = true;
            }
        } else {
            std::cerr << "Unknown argument " << argv[i] << std::endl;
            showHelp = true;
            argError = true;
        }
    }
    if (!showHelp && configData.task == NONE) {
        std::cerr << "No command specified." << std::endl;
        showHelp = true;
        argError = true;
    }
    if (showHelp) {
        fprintf(argError ? stderr : stdout,
                "Usage: %s [options] task" "\n"
                        "" "\n"
                        "Tasks:" "\n"
                        "  * video:              Render video of external camera and panorama to /tmp/\n"
                        "  * hillclimbing:       Run hillclimbing algorithm\n"
                        "  * random:             Run random (brute-force) algorithm\n"
                        "  * utility:            Compute true utility map through systematic sampling\n"
                        "  * utilityanimation:   Utility animation for video\n"
                        "  * benchmark:          Benchmark the GPU algorithms\n"
                        "\n"
                        "Options:\n"
                        "  * --cameras, -c NUM:       Use NUM random camera poses in random search (default: %zu)\n"
#ifdef HAS_NVML
                "  * --check-gpu-free:        Only use GPUs that are not currently in use\n"
#endif
                "  * --articulations, -a NUM: Use NUM random articulation configurations (default: %zu)\n"
                "  * --config, -c FILE:       Use config file (default: config/config.txt))\n"
                "  * --devices, -d DEV:       Use the given GPU device numbers (0-n) separated by comma\n"
                "  * --iterations, -i NUM:    Use NUM random interations (default: %zu)\n"
                "  * --help, -h:              Show this help\n"
                "  * --seed, -s SEED:         Set the random seed (default: random)\n"
                "  * --threads:               Use threads instead of processes for workers\n"
                "\n", argv[0], configData.randomArticulationConfigs, configData.randomCameraPoses, configData.randomIterations);
        return argError ? 1 : 0;
    }

    if (configData.setSeed) {
        srand(configData.seed);
    } else {
        srand(time(0));
    }

    const char * args[2] = {NULL, configData.configFile.c_str()};
    Config::init(2, args);

    // Load scene with Assimp
    const std::string path = std::string(DATADIR) + "/" + Config::getInstance().getParam<std::string>("file");
    const size_t p = path.rfind(".dae");
    if (p != std::string::npos) {
        std::string materialsPath = path;
        materialsPath.replace(p, 4, "-materials.txt");
        struct stat buffer;
        if (stat(materialsPath.c_str(), &buffer) == 0) {
            Material::loadMaterialsFile(materialsPath.c_str());
        }
    }
    Assimp::Importer importer;
    importer.ReadFile(path.c_str(), aiProcess_JoinIdenticalVertices);
    configData.ai_scene = importer.GetScene();
    if (!configData.ai_scene) {
        logError("Unable to load scene: %s", importer.GetErrorString());
        return 2;
    }

#ifdef HAS_NVML
    if (nvmlInit() != NVML_SUCCESS) {
        logError("Could not initialize NVML");
        return 1;
    }
    unsigned int totalDevices;
    nvmlDeviceGetCount(&totalDevices);

#else
    unsigned int totalDevices = 1;
#endif

    unsigned int usingDevices[MAX_DEVICES];

    // Determine how many GPU devices we can use
    configData.numDevices = 0;
    if (totalDevices == 1 && (limitDevices.empty() || limitDevices.find(0) != limitDevices.end())) {
        // only one GPU available -> use
        usingDevices[configData.numDevices] = 0;
        ++configData.numDevices;
    } else {
        for (unsigned int i = 0; i < totalDevices; ++i) {
            // Filter unwanted devices
            if (!limitDevices.empty() && limitDevices.find(i) == limitDevices.end()) {
                continue;
            }
#ifdef HAS_NVML
            if (checkGpuFree) {
                // Check if device is free
                nvmlDevice_t deviceHandle;
                nvmlDeviceGetHandleByIndex(i, &deviceHandle);
                unsigned int gpuProcessCount = 2, computeProcessCount = 2;
                nvmlProcessInfo_t gpuProcessInfo[gpuProcessCount], computeProcessInfo[computeProcessCount];
                if (nvmlDeviceGetGraphicsRunningProcesses(deviceHandle, &gpuProcessCount, gpuProcessInfo) == NVML_SUCCESS &&
                        nvmlDeviceGetComputeRunningProcesses(deviceHandle, &computeProcessCount, computeProcessInfo) == NVML_SUCCESS) {
                    if (gpuProcessCount == 0 && computeProcessCount == 0) {
                        usingDevices[configData.numDevices] = i;
                        ++configData.numDevices;
                    }
                    continue;
                }
            }
#endif
            // NVML process listing not supported or not enabled -> accept GPU
            usingDevices[configData.numDevices] = i;
            ++configData.numDevices;
        }
    }

    if (configData.numDevices == 0) {
        logError("Did not find any free GPU");
        return 4;
    }

    std::stringstream ss;
    ss << "Detected " << totalDevices << " devices, using " << configData.numDevices;
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, 0, GL_DEBUG_SEVERITY_NOTIFICATION,
            ss.str().size(), ss.str().c_str());

    // Create threads or processes
    ThreadData threadData[configData.numDevices];
    for (size_t i = 0; i < configData.numDevices; ++i) {
        threadData[i].threadNr = i;
        threadData[i].deviceNr = usingDevices[i];
    }

    sharedData = (SharedData *) mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1,
            0);

    pthread_barrierattr_t barrierAttr;
    pthread_barrierattr_init(&barrierAttr);
    pthread_barrierattr_setpshared(&barrierAttr, PTHREAD_PROCESS_SHARED);
    pthread_barrier_init(&sharedData->barrier, &barrierAttr, configData.numDevices);

    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&sharedData->mutex, &mutexAttr);

    sharedData->numThreads = configData.numDevices;
    RobotSceneConfiguration::loadCosts();

    switch (configData.task) {
    case RANDOM:
        RandomSearchTask::allocateSharedData();
        break;
    case HILLCLIMBING:
        HillclimbingTask::allocateSharedData();
        break;
    case BENCHMARK:
        BenchmarkTask::allocateSharedData();
        break;
    default:
        break;
    }

    if (configData.numDevices == 1) {
        doWork(&threadData[0]);
    } else {
        if (multiProcess) {
            pid_t pids[configData.numDevices];
            for (size_t i = 0; i < configData.numDevices; ++i) {
                pid_t pid = fork();
                if (pid == 0) {
                    char threadname[256];
                    snprintf(threadname, sizeof(threadname), "worker_%zu", i);
                    pthread_setname_np(pthread_self(), threadname);
                    doWork(&threadData[i]);
                    return 0;
                } else {
                    pids[i] = pid;
                }
            }
            for (size_t i = 0; i < configData.numDevices; ++i) {
                int status;
                waitpid(pids[i], &status, 0);
                if (status != 0) {
                    logError("Child %zu exited with status %d", i, status);
                }
            }
        } else {
            pthread_t threads[configData.numDevices];
            for (size_t i = 0; i < configData.numDevices; ++i) {
                if (pthread_create(&threads[i], NULL, doWork, &threadData[i])) {
                    logError("Error creating thread");
                    return 1;
                }
            }
            for (size_t i = 0; i < configData.numDevices; ++i) {
                pthread_join(threads[i], NULL);
            }
        }
    }

    pthread_barrier_destroy(&sharedData->barrier);

    switch (configData.task) {
    case RANDOM:
        RandomSearchTask::freeSharedData();
        break;
    case HILLCLIMBING:
        HillclimbingTask::freeSharedData();
        break;
    default:
        break;
    }
    munmap(sharedData, sizeof(sharedData));

    return 0;
}

