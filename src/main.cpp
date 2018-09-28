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
#include <gpu_coverage/BellmanFordXfbRenderer.h>
#include <gpu_coverage/CostMapRenderer.h>
#include <gpu_coverage/Renderer.h>
#include <gpu_coverage/PanoRenderer.h>
#include <gpu_coverage/PanoEvalRenderer.h>
#include <gpu_coverage/VisibilityRenderer.h>
#include <gpu_coverage/Config.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Channel.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <sys/signal.h>
#include <sys/stat.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <list>
#include <string>
#include <sstream>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS true
#endif
#include <glm/gtc/matrix_transform.hpp>

using namespace gpu_coverage;

#ifndef DATADIR
#define DATADIR "."
#endif

#ifdef DEBUG
static void APIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, const GLchar* message, const void *userParam) {
    if (type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP) {
        // do not print debug group push/pop
        return;
    }
    if (strstr(message, "will use VIDEO memory") != NULL) {
        // do not print buffer performance info if buffer is on GPU
        return;
    }
    if (strstr(message, "Texture 0 is base level inconsistent") != NULL) {
        // false positive in VisibilityRenderer (glClearBufferfv)
        return;
    }
    bool print = true;
    switch(severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION: logInfo("[NOTE] %s", message); break;
        case GL_DEBUG_SEVERITY_LOW: logInfo("[LOW ] %s", message); break;
        case GL_DEBUG_SEVERITY_MEDIUM: logWarn("[MED ] %s", message); break;
        case GL_DEBUG_SEVERITY_HIGH: logError("[HIGH] %s", message); break;
    }
}
#endif

static void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void keyCallback(GLFWwindow * const window, const int key, const int scancode, const int action, const int mods);

namespace gpu_coverage {
struct WindowScene {
    GLFWwindow * window;
    Scene * scene;
    struct ConditionalRenderer {
        AbstractRenderer * const renderer;
        bool always;
        bool onArticulationChange;
        bool onCameraChange;
        bool onWindowDamage;
        ConditionalRenderer(AbstractRenderer * const renderer)
                : renderer(renderer), always(false), onArticulationChange(false), onCameraChange(false), onWindowDamage(
                        false) {
        }
        ;
        ~ConditionalRenderer() {
        }
        ;
    };
    bool isDamaged;
    typedef std::list<ConditionalRenderer> ConditionalRenderers;
    ConditionalRenderers renderers;
    const std::string title;

    WindowScene(const size_t width, const size_t height, const std::string title, GLFWwindow* share,
            const aiScene * const aiScene)
            : window(NULL), scene(NULL), isDamaged(true), title(title) {
        window = glfwCreateWindow(width, height, title.c_str(), NULL, share);
        if (!window) {
            logError("Could not create window");
            exit(1);
            return;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        glfwSetKeyCallback(window, keyCallback);
        scene = new Scene(aiScene, DATADIR "/models");
    }
    ~WindowScene() {
        if (scene) {
            delete scene;
            scene = NULL;
        }
        if (window) {
            glfwDestroyWindow(window);
            window = NULL;
        }
    }
};

typedef std::list<WindowScene *> WindowScenes;
}

static WindowScenes windows;
bool rotateMain = false;
bool articulationChanged = true;
bool cameraChanged = true;

static void keyCallback(GLFWwindow * const window, const int key, const int scancode, const int action,
        const int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, 1);
            break;
        case GLFW_KEY_SPACE:
            rotateMain = !rotateMain;
            break;
        }
    }
}

static int cvframe;

static void on_setSlider(int frame, void *data) {
    int c = *static_cast<int * >(data);
    for (WindowScenes::const_iterator it = windows.begin(); it != windows.end(); ++it) {
        Channel * channel = (*it)->scene->channels[c];
        channel->setFrame(channel->getStartFrame() + frame);
    }
    articulationChanged = true;
}

static void on_setPlaneHeight(int stp, void *) {
    Node * node = windows.front()->scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"));
    glm::mat4x4 m = node->getLocalTransform();
    m[3][2] = stp / 100.;
    for (WindowScenes::const_iterator it = windows.begin(); it != windows.end(); ++it) {
        (*it)->scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane"))->setLocalTransform(m);
    }
    articulationChanged = true;
}

int main(int argc, char *argv[]) {
    // Load scene with Assimp
    Config::init(argc, argv);
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
    const aiScene * const aiScene = importer.GetScene();
    if (!aiScene) {
        logError("Unable to load scene: %s", importer.GetErrorString());
        return 1;
    }

    // Initialize GLFW
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        logError("Could not initialize GLFW");
        return 1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GET_API(GLFW));
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    // Create windows and scenes
    int panoWidth, panoHeight;
    switch (Config::getInstance().getParam<Config::PanoOutputValue>("panoOutputFormat")) {
    case Config::EQUIRECTANGULAR:
        panoWidth = 1024;
        panoHeight = 512;
        break;
    case Config::CYLINDRICAL:
        panoWidth = 1024;
        panoHeight = 512;
        break;
    case Config::CUBE:
        panoWidth = 1024;
        panoHeight = 768;
        break;
    case Config::IMAGE_STRIP_HORIZONTAL:
        panoWidth = 1020;
        panoHeight = 170;
        break;
    case Config::IMAGE_STRIP_VERTICAL:
        panoWidth = 170;
        panoHeight = 1020;
        break;
    default:
        logError("Unknown panorama output format");
        return 3;
    }

    WindowScene * const mainWindow = new WindowScene(1920 / 2, 1080 / 2, "Render", NULL, aiScene);
    WindowScene * const costmapWindow = new WindowScene(512, 512, "Bellman-Ford", mainWindow->window, aiScene);
    WindowScene * const panoWindow = new WindowScene(panoWidth, panoHeight, "Panorama", mainWindow->window, aiScene);
    WindowScene * const visibilityWindow = new WindowScene(512, 512, "Visibility", mainWindow->window, aiScene);
    windows.push_back(costmapWindow);
    windows.push_back(visibilityWindow);
    windows.push_back(mainWindow);
    windows.push_back(panoWindow);

    glfwIconifyWindow(costmapWindow->window);
    glfwIconifyWindow(mainWindow->window);
    glfwIconifyWindow(visibilityWindow->window);
    //glfwIconifyWindow(panoWindow->window);

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glDebugCallback, NULL);
#endif

    // Create renderers
    glfwMakeContextCurrent(mainWindow->window);
    AbstractProgram::setOpenGLVersion();
    Renderer renderer(mainWindow->scene, true, false);
    if (!renderer.isReady()) {
        return 1;
    }
    const std::string projectionPlaneName = Config::getInstance().getParam<std::string>("projectionPlane");
    mainWindow->scene->findNode(projectionPlaneName)->setVisible(false);
    mainWindow->scene->toDot("/tmp/scene.dot");
    glfwMakeContextCurrent(costmapWindow->window);
    CostMapRenderer costmapRenderer(costmapWindow->scene, costmapWindow->scene->findNode(projectionPlaneName), false,
            false);
    if (!costmapRenderer.isReady()) {
        return 1;
    }
    BellmanFordXfbRenderer bellmanFordRenderer(costmapWindow->scene, &costmapRenderer, true, false);
    bellmanFordRenderer.setRobotPosition(
            mainWindow->scene->findNode(Config::getInstance().getParam<std::string>("robotCamera"))->getWorldTransform());
    if (!bellmanFordRenderer.isReady()) {
        return 1;
    }
    {
        WindowScene::ConditionalRenderer r1(&costmapRenderer);
        r1.onArticulationChange = true;
        costmapWindow->renderers.push_back(r1);
        WindowScene::ConditionalRenderer r2(&bellmanFordRenderer);
        r2.onArticulationChange = true;
        costmapWindow->renderers.push_back(r2);
    }

    glfwMakeContextCurrent(panoWindow->window);

    bool panoSemantic = Config::getInstance().getParam<bool>("panoSemantic");
    PanoRenderer panoRenderer(panoWindow->scene, !panoSemantic, &bellmanFordRenderer);
    if (!panoRenderer.isReady()) {
        return 1;
    }
    PanoEvalRenderer panoEvalRenderer(panoWindow->scene, panoSemantic, true, &panoRenderer, &bellmanFordRenderer);
    {
        WindowScene::ConditionalRenderer r1(&panoRenderer);
        r1.onArticulationChange = true;
        panoWindow->renderers.push_back(r1);
        WindowScene::ConditionalRenderer r2(&panoEvalRenderer);
        r2.onArticulationChange = true;
        r2.onWindowDamage = true;
        panoWindow->renderers.push_back(r2);
    }

    {
        WindowScene::ConditionalRenderer r3(&renderer);
        r3.onArticulationChange = true;
        r3.onCameraChange = true;
        r3.onWindowDamage = true;
        mainWindow->renderers.push_back(r3);
    }

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
                panoEvalRenderer.addCameraPair(first, second);
            } else {
                logError("Panorama camera pair with less than two cameras");
                return 2;
            }
            isPair = false;
            first = NULL;
            second = NULL;
        } else if (!panoCameraName.empty()) {
            gpu_coverage::Node * const cameraNode = panoWindow->scene->findNode(panoCameraName);
            if (!cameraNode) {
                logError("Could not find panorama camera %s", panoCameraName.c_str());
                return 2;
            }
            CameraPanorama * const panoCam = panoWindow->scene->makePanoramaCamera(cameraNode);
            mainWindow->scene->makePanoramaCamera(mainWindow->scene->findNode(panoCameraName));
            if (isPair) {
                if (first == NULL) {
                    first = panoCam;
                } else if (second == NULL) {
                    second = panoCam;
                } else {
                    logError("More than two cameras in panorama camera pair");
                }
            } else {
                panoEvalRenderer.addCamera(panoCam);
            }
        }
    }

    panoWindow->scene->toDot("/tmp/pano-scene.dot");
    if (panoWindow->scene->panoramaCameras.empty()) {
        logError("No panorama cameras found");
        return 2;
    }
    panoRenderer.setCamera(panoWindow->scene->panoramaCameras[0]);

    glfwMakeContextCurrent(visibilityWindow->window);
    VisibilityRenderer visibilityRenderer(visibilityWindow->scene, true, false);
    if (!visibilityRenderer.isReady()) {
        return 1;
    }
    {
        WindowScene::ConditionalRenderer r(&visibilityRenderer);
        r.onArticulationChange = true;
        r.onWindowDamage = true;
        visibilityWindow->renderers.push_back(r);
    }

    // Create link costmap texture to costmap renderer
    Material * projectionPlaneMaterial = mainWindow->scene->findNode(projectionPlaneName)->getMeshes().front()->getMaterial();
    if (!projectionPlaneMaterial) {
        logError("Could not find costmap material");
    }
    Texture costmapTexture(bellmanFordRenderer.getTexture());
    projectionPlaneMaterial->setTexture(&costmapTexture);

    // Create link visibility texture to target
    std::list<Texture * > visibilityTextures;
    std::stringstream targets(Config::getInstance().getParam<std::string>("target"));
    size_t targetI = 0;
    while (targets.good()) {
        std::string targetName;
        targets >> targetName;
        if (!targetName.empty()) {
            const Node * const target = mainWindow->scene->findNode(targetName);
            Material * targetMaterial = target->getMeshes().front()->getMaterial();
            if (!targetMaterial) {
                logError("Could not find target material");
            }
            Texture * visibilityTexture = new Texture(visibilityRenderer.getTexture(targetI));
            visibilityTextures.push_back(visibilityTexture);
            ++targetI;
            targetMaterial->setTexture(visibilityTexture);
        }
    }

    // Utility map to floor in main window
    Node * const floorProjectionNode = mainWindow->scene->findNode(Config::getInstance().getParam<std::string>("floorProjection"));
    if (!floorProjectionNode) {
        logError("Could not find floor projection node %s", Config::getInstance().getParam<std::string>("floorProjection").c_str());
        return 1;
    }
    Material * floorProjectionMaterial = floorProjectionNode->getMeshes().front()->getMaterial();
    if (!floorProjectionMaterial) {
        logError("Could not find floor projection material");
        return 1;
    }
    Texture utilityMapTexture(panoEvalRenderer.getUtilityMapVisual());
    //Texture bellmanFordTexture(bellmanFordRenderer.getVisualTexture());
    //Texture costmapIndexTexture(panoRenderer.getCostmapIndexTexture());
    floorProjectionMaterial->setTexture(&utilityMapTexture);
    mainWindow->scene->findNode(Config::getInstance().getParam<std::string>("floor"))->setVisible(false);
    //mainWindow->scene->findNode(Config::getInstance().getParam<std::string>("floorProjection"))->setVisible(false);

    //panoWindow->scene->findNode(Config::getInstance().getParam<std::string>("target"))->getMeshes().front()->getMaterial()->setTexture(visibilityTexture);

    cv::namedWindow("Settings", 1);
    int planeHeight = 140;
    const Scene::Channels& channels = mainWindow->scene->getChannels();
    std::vector<int> sliders(channels.size(), 0);
    std::vector<int> channelIDs(channels.size());
    for (size_t i = 0; i < channels.size(); ++i) {
        channelIDs[i] = i;
        cv::createTrackbar(channels[i]->getNode()->getName().c_str(), "Settings", &sliders[i], channels[i]->getNumFrames(),
                on_setSlider, &channelIDs[i]);
    }
    cv::createTrackbar("Plane height", "Settings", &planeHeight, 150, on_setPlaneHeight,
            mainWindow->scene->findNode(Config::getInstance().getParam<std::string>("projectionPlane")));

    Node * const mainWindowCam = mainWindow->scene->findNode(
            Config::getInstance().getParam<std::string>("externalCamera"));

    /*{
        float x = -3.85882807;
        float y = -1.72914064;
        float pitch = 1.74532938;
        float yaw = 0.34906584;
        static const glm::vec3 worldUp(0.f, 0.f, -1.f);
        const glm::vec3 eye(x, y, 1.4);  // TODO
        const glm::vec3 look = glm::vec3(sin(pitch) * cos(yaw), sin(pitch) * sin(yaw), cos(pitch));
        const glm::vec3 right(glm::cross(look, worldUp));
        const glm::vec3 up(glm::cross(look, right));
        const glm::mat4 camT(glm::inverse(glm::lookAt(eye, eye + look, up)));
        for (WindowScenes::const_iterator windowIt = windows.begin(); windowIt != windows.end(); ++windowIt) {
            (*windowIt)->scene->findNode(Config::getInstance().getParam<std::string>("externalCamera"))->setLocalTransform(camT);
            (*windowIt)->scene->findNode(Config::getInstance().getParam<std::string>("robotCamera"))->setLocalTransform(camT);
        }
    }*/

    size_t curFrame = windows.front()->scene->getStartFrame();
    bool shouldTerminate = false;
    float camTheta = 0.;
    const glm::vec3 worldUp(0.f, 0.f, -1.f);
    while (!shouldTerminate) {
        curFrame = windows.front()->scene->getStartFrame() + cvframe;

        if (rotateMain) {
            const glm::vec3 eye(10. * sin(camTheta), 10. * cos(camTheta), 10.f);
            const glm::vec3 look(glm::normalize(eye));
            const glm::vec3 right(glm::cross(look, worldUp));
            const glm::vec3 up(glm::cross(look, right));

            const glm::mat4x4 mainCamTransform = glm::inverse(glm::lookAt(
                    eye,
                    glm::vec3(0., 0., 0.),
                    up
                    ));
            camTheta += 0.01f;
            mainWindowCam->setLocalTransform(mainCamTransform);
            cameraChanged = true;
        }

        int width, height;
        for (WindowScenes::const_iterator windowIt = windows.begin(); windowIt != windows.end(); ++windowIt) {
            glfwGetWindowSize((*windowIt)->window, &width, &height);
            glfwMakeContextCurrent((*windowIt)->window);
            glViewport(0, 0, width, height);
            bool didDraw = false;
            for (WindowScene::ConditionalRenderers::const_iterator rendererIt = (*windowIt)->renderers.begin();
                    rendererIt != (*windowIt)->renderers.end(); ++rendererIt) {
                if (rendererIt->always
                        || (rendererIt->onArticulationChange && articulationChanged)
                        || (rendererIt->onCameraChange && cameraChanged)
                        || (rendererIt->onWindowDamage && (*windowIt)->isDamaged)
                        || true // TODO remove debug
                        )
                        {
                    rendererIt->renderer->display();
                    didDraw = true;
                    /*if (rendererIt->renderer == &visibilityRenderer) {
                        std::vector<GLuint> pixelCounts;
                        visibilityRenderer.getPixelCounts(pixelCounts);
                        std::cout << "Visibility count: " << pixelCounts[0] << std::endl;
                    }*/
                }
            }
            (*windowIt)->isDamaged = false;
            if (didDraw) {
                glfwSwapBuffers((*windowIt)->window);
            }
        }

        articulationChanged = false;
        cameraChanged = false;

        cv::waitKey(1);

        glfwPollEvents();
        for (WindowScenes::const_iterator it = windows.begin(); !shouldTerminate && it != windows.end(); ++it) {
            if (glfwWindowShouldClose((*it)->window)) {
                shouldTerminate = true;

            }
        }
    }

    for (std::list<Texture * >::const_iterator it = visibilityTextures.begin(); it != visibilityTextures.end(); ++it) {
        delete(*it);
    }
    for (WindowScenes::const_iterator it = windows.begin(); it != windows.end(); ++it) {
        delete (*it);
    }

    glfwTerminate();
    cv::destroyAllWindows();
    return 0;
}
