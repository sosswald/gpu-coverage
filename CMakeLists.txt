cmake_minimum_required(VERSION 2.8.3)
project(gpu_coverage)

set(CMAKE_CXX_FLAGS "-mtune=native -march=native -Wall -Wno-unused -Werror -fstack-protector -fstack-protector-all ${CMAKE_CXX_FLAGS}")

set(OPENGL_MAJOR 4 CACHE STRING "OpenGL major version")
set(OPENGL_MINOR 4 CACHE STRING "OpenGL minor version")
set(OPENGL_API OPENGL_API CACHE STRING "OpenGL API")
set(USE_XFB ON CACHE BOOL "Use testRenderer instead of dijkstraRenderer")
add_definitions(-DOPENGL_MAJOR=${OPENGL_MAJOR} -DOPENGL_MINOR=${OPENGL_MINOR} -DOPENGL_API=${OPENGL_API} -DGL_GLEXT_PROTOTYPES=1)

find_package(catkin QUIET)

find_package(OpenGL REQUIRED)
set(OpenCV_LIBS "${OPENGL_glu_LIBRARY};${OPENGL_gl_LIBRARY}")
add_definitions(-DGL_INCLUDE=<GL/gl.h> -DGLEXT_INCLUDE=<GL/glext.h> -DGLU_INCLUDE=<GL/glu.h>)

find_package(Threads REQUIRED)
find_package(OpenCV 3.0.0 QUIET COMPONENTS core imgproc imgcodecs video videoio highgui)
if(NOT OpenCV_FOUND)
  find_package(OpenCV 2.4.8 REQUIRED COMPONENTS core imgproc highgui)
endif()
add_definitions(-DHAS_OPENCV=1)

include(FindPkgConfig)
pkg_check_modules(assimp REQUIRED assimp)
pkg_check_modules(glfw3 REQUIRED glfw3)
pkg_check_modules(egl REQUIRED egl)

include(CheckCSourceCompiles)
check_c_source_compiles("
#include <EGL/egl.h>
#include <EGL/eglext.h>
int main() { EGLDeviceEXT a; }" HAS_EGLEXT)

find_path(NVML_INCLUDE_DIRS NAMES nvml.h PATHS /usr/local/cuda/targets/x86_64-linux/include)
if(NVML_INCLUDE_DIRS)
    file(GLOB NVIDIA_DIRS /usr/lib/nvidia-*)
    find_library(NVML_LIBS NAMES nvidia-ml PATHS ${NVIDIA_DIRS})
    if (NVML_LIBS)
        add_definitions(-DHAS_NVML)
    endif()
else()
    set(NVML_INCLUDE_DIRS "")
    set(NVML_LIBS "")
endif()

find_path(GLM_INCLUDE_DIRS NAMES glm/glm.hpp)
set(GLM_LIBS "")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

if(catkin_FOUND)
    catkin_package(
        DEPENDS assimp glfw3 egl OPENGL OpenCV NVML GLM
    ) 
    #add_definitions(-DDATADIR="${CMAKE_INSTALL_PREFIX}/${CATKIN_PACKAGE_SHARE_DESTINATION}")
else()
    set(catkin_INCLUDE_DIRS "")
    set(catkin_LIBRARY_DIRS "")
    set(catkin_LIBRARIES "")
endif()
add_definitions(-DDATADIR="${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(
	include
	${assimp_INCLUDE_DIRS}
	${catkin_INCLUDE_DIRS}
	${egl_INCLUDE_DIRS}
	${glfw3_INCLUDE_DIRS}
	${OpenCV_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${NVML_INCLUDE_DIRS}
	${GLM_INCLUDE_DIRS}
)

link_directories(
	${assimp_LIBRARY_DIRS}
	${catkin_LIBRARY_DIRS}
	${egl_LIBRARY_DIRS}
	${glfw3_LIBRARY_DIRS}
)

add_library(${PROJECT_NAME}
    src/AbstractCamera.cpp
    src/AbstractRenderer.cpp
    src/AbstractTask.cpp
    src/Animation.cpp
    src/BellmanFordRenderer.cpp
    src/BellmanFordXfbRenderer.cpp
    src/BenchmarkTask.cpp
    src/Bone.cpp
    src/CameraOrtho.cpp
    src/CameraPerspective.cpp
    src/CameraPanorama.cpp
    src/Channel.cpp
    src/Config.cpp
    src/CoordinateAxes.cpp
    src/CostMapRenderer.cpp
    src/Dot.cpp
    src/HillclimbingTask.cpp
    src/Image.cpp
    src/Light.cpp
    src/Material.cpp
    src/Mesh.cpp
    src/Node.cpp
    src/PanoEvalRenderer.cpp
    src/PanoRenderer.cpp
    src/Programs.cpp
    src/RandomSearchTask.cpp
    src/Renderer.cpp
    src/RobotSceneConfiguration.cpp
    src/Scene.cpp
    src/Texture.cpp
    src/Utilities.cpp
    src/UtilityAnimationTask.cpp
    src/UtilityMapSystematicTask.cpp
    src/VideoTask.cpp
    src/VisibilityRenderer.cpp
)
target_link_libraries(${PROJECT_NAME} ${assimp_LIBRARIES} ${OPENGL_LIBRARIES} ${OpenCV_LIBS})

add_executable(render
    src/main.cpp
)
target_link_libraries(render ${PROJECT_NAME} ${catkin_LIBRARIES} ${glfw3_LIBRARIES})

if(${HAS_EGLEXT})
    add_executable(headless src/headless.cpp)
    target_link_libraries(headless ${PROJECT_NAME} ${catkin_LIBRARIES} ${egl_LIBRARIES} ${OpenCV_LIBS} ${NVML_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()
add_executable(headless-glfw src/headless-glfw.cpp)
target_link_libraries(headless-glfw ${PROJECT_NAME} ${catkin_LIBRARIES} ${glfw3_LIBRARIES} ${OpenCV_LIBS} ${NVML_LIBS} ${CMAKE_THREAD_LIBS_INIT})

if(catkin_FOUND)
    install(DIRECTORY include/${PROJECT_NAME}/ DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
    install(DIRECTORY config DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
    install(DIRECTORY shaders DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
    install(DIRECTORY models DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
endif()

