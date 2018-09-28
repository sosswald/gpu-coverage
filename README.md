# GPU-Accelerated Next-Best-View Coverage of Articulated Scenes

This repository contains the code accompanying our paper on GPU-accelerated coverage of articulated scenes.

**Disclaimer: The code in this repository is research code created for a feasibility study. The code is not ready for production and the authors will not provide support. Use at your own risk.**

For details on the approach see our paper:

Stefan Oßwald and Maren Bennewitz: **GPU-Accelerated Next-Best-View Coverage of Articulated Scenes.**
Proceedings of the IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS), 2018.

## License
The code is released under the 3-clause BSD license. See [LICENSE](LICENSE) for details.

## Requirements
* An OpenGL implementation and GPU hardware supporting at least OpenGL ES 3.2 or OpenGL Core 3.3 profiles.
* An EGL implementation for headless access to the GPU.
* [Assimp](http://www.assimp.org/), the Open Asset Importer Library.
* [GLFW3](https://www.glfw.org/) for handling OpenGL windows, contexts, and surfaces.
* [GLM](https://glm.g-truc.net/), the OpenGL mathematics library.
* [CMake](https://cmake.org/) is used as the build system.
* Optional: [Catkin](https://wiki.ros.org/catkin) for ROS package building support.
* Optional: [OpenCV](https://opencv.org/) for loading image textures, both versions 2.4 and 3.0 are supported.
* Optional: [NVLM](https://developer.nvidia.com/nvidia-management-library-nvml), the NVIDIA management library for handling GPUs on multi-GPU servers

## Compiling

Compile the code using either CMake or Catkin:
```sh
mkdir build
cd build
cmake .. [-D FLAG=VALUE ...]
make
```

Compile flags:

| Flag           | Default value   | Description                                                                                                             |
| -------------- | --------------- | ----------------------------------------------------------------------------------------------------------------------- | 
| `OPENGL_API`   | `OPENGL_API`    | OpenGL API to use. Set this either to `OPENGL_API` for the core API or to `OPENGL_ES_API` for the embedded systems API. |
| `OPENGL_MAJOR` | `4`             | Major version of the OpenGL or OpenGL ES API                                                                            |
| `OPENGL_MINOR` | `4`             | Minor version of the OpenGL or OpenGL ES API                                                                            |
| `USE_XFB`      | `ON`            | Boolean flag, use transfer buffer (XFB) variant of the Bellman-Ford implementation                                      |


## API Documentation

See the [Doxygen documentation](https://sosswald.github.io/gpu-coverage/) for an overview of the source code and API.
