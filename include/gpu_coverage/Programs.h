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

#ifndef INCLUDE_ARTICULATION_PROGRAMS_H_
#define INCLUDE_ARTICULATION_PROGRAMS_H_

#include GL_INCLUDE

namespace gpu_coverage {

/**
 * @brief Abstract superclass for all shader programs.
 */
class AbstractProgram {
protected:
	/**
	 * @brief Constructor.
	 */
    AbstractProgram();
    /**
     * @brief Destructor.
     */
    virtual ~AbstractProgram();

public:
    /**
     * @brief Reads versions and available extensions from the GPU.
     *
     * This method must be called after initializing OpenGL and
     * before instantiating any shader program.
     */
    static void setOpenGLVersion();

    /**
     * @brief Bind this program to the GPU.
     */
    void use() const;

    /**
     * @brief Returns true if the program is ready to be used.
     * @return True if ready.
     */
    inline bool isReady() const {
        return ready;
    }

protected:
    const GLuint program;          ///< OpenGL program ID
    bool ready;                    ///< True if program is ready, see isReady().

    /**
     * @brief Create a shader from source code.
     * @param[in] shaderType Shader type GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, ...
     * @param[in] code Source code for the shader.
     * @return Shader number or 0 in case of failure.
     */
    GLuint createShader(const GLenum shaderType, const GLchar* const code, const char * const name) const;
    /**
     * @brief Create a shader from a file.
     * @param[in] shaderType Shader type GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, ...
     * @param[in] code File name of the shader source code file.
     * @return Shader number or 0 in case of failure.
     */
    GLuint loadShader(const GLenum shaderType, const char * const filename) const;

    /**
     * @brief Link the associated shader stages.
     * @param name Human-readable program name for debugging.
     * @return True on success.
     */
    bool link(const char * const name) const;
};

/**
 * @brief Locations of model, view, and projection shader variables.
 */
struct LocationsMVP {
    GLint modelMatrix;          ///< 4x4 Model matrix.
    GLint viewMatrix[6];        ///< 4x4 View matrix (6 matrices for cube map rendering).
    GLint projectionMatrix;     ///< 4x4 Projection matrix.
    GLint normalMatrix[6];      ///< 3x3 Normal matrix = transposed inverse of model view matrix (6 matrices for cube mapping).
    GLint mvp;                  ///< 4x4 Pre-multiplied model-view-projection matrix.
    /**
     * @brief Constructor, initalizes all locations to invalid.
     */
    LocationsMVP()
            : modelMatrix(-1), projectionMatrix(-1), mvp(-1) {
        for (size_t i = 0; i < 6; ++i) {
            viewMatrix[i] = -1;
            normalMatrix[i] = -1;
        }
    }
};

/**
 * @brief Locations of light source shader variables
 */
struct LocationsLight {
    GLint lightPosition;        ///< vec4 position of the light source in homogeneous coordinates.
    GLint lightAmbient;         ///< vec3 RGB ambient color of the light source.
    GLint lightDiffuse;         ///< vec3 diffuse color of the light source.
    GLint lightSpecular;        ///< vec3 specular color of the light source.

    /**
     * @brief Constructor, initalizes all locations to invalid.
     */
    LocationsLight()
            : lightPosition(-1), lightAmbient(-1), lightDiffuse(-1), lightSpecular(-1) {
    }
};

/**
 * @brief Locations of material shader variables.
 */
struct LocationsMaterial {
    GLint materialAmbient;       ///< vec3 ambient color of the material.
    GLint materialDiffuse;       ///< vec3 diffuse color of the material.
    GLint materialSpecular;      ///< vec3 specular color of the material.
    GLint materialShininess;     ///< float shininess factor of the material.
    GLint textureUnit;           ///< ID of the texture unit holding the diffuse material texture.
    GLint hasTexture;            ///< True if the texture unit should be used, false if material parameters should be used.

    /**
     * @brief Constructor, initalizes all locations to invalid.
     */
    LocationsMaterial()
            : materialAmbient(-1), materialDiffuse(-1), materialSpecular(-1), materialShininess(-1), textureUnit(-1),
                    hasTexture(-1) {
    }
};

class ProgramPano: public AbstractProgram {
public:
    ProgramPano();
    ~ProgramPano();
    LocationsMVP locationsMVP;
    LocationsLight locationsLight;
    LocationsMaterial locationsMaterial;
    bool hasTesselationShader;
};

class ProgramPanoEval: public AbstractProgram {
public:
    ProgramPanoEval();
    ~ProgramPanoEval();
    struct Locations {
        GLint textureUnit;
        GLint integral;
        GLint resolution;
        Locations()
                : textureUnit(-1), integral(-1), resolution(-1) {
        }
    } locations;
};

class ProgramPanoSemantic: public AbstractProgram {
public:
    ProgramPanoSemantic();
    ~ProgramPanoSemantic();
    LocationsMVP locationsMVP;
    LocationsMaterial locationsMaterial;
};

class ProgramVisualTexture: public AbstractProgram {
public:
    ProgramVisualTexture();
    ~ProgramVisualTexture();
    LocationsMVP locationsMVP;
    LocationsLight locationsLight;
    LocationsMaterial locationsMaterial;
};

class ProgramVisualFlat: public AbstractProgram {
public:
    ProgramVisualFlat();
    ~ProgramVisualFlat();
    LocationsMVP locationsMVP;
    struct Locations {
        GLint color;
        Locations()
                : color(-1) {
        }
    } locations;
};

class ProgramVisualVertexcolor: public AbstractProgram {
public:
    ProgramVisualVertexcolor();
    ~ProgramVisualVertexcolor();
    LocationsMVP locationsMVP;
};

/**
 * @brief Abstract superclass for all shader programs projecting panoramas to texture images.
 */
class AbstractProgramMapProjection: public AbstractProgram {
protected:
    AbstractProgramMapProjection(const char * const name);
    public:
    ~AbstractProgramMapProjection();

    struct Locations {
        GLint textureUnit;
        Locations()
                : textureUnit(-1) {
        }
    } locations;
};

class ProgramFlatProjection2dArray: public AbstractProgramMapProjection {
public:
    ProgramFlatProjection2dArray();
    ~ProgramFlatProjection2dArray();
};

class ProgramFlatProjectionCube: public AbstractProgramMapProjection {
public:
    ProgramFlatProjectionCube();
    ~ProgramFlatProjectionCube();
};

class ProgramEquirectangular: public AbstractProgramMapProjection {
public:
    ProgramEquirectangular();
    ~ProgramEquirectangular();
};

class ProgramCylindrical: public AbstractProgramMapProjection {
public:
    ProgramCylindrical();
    ~ProgramCylindrical();
};

class ProgramOccupancyMap: public AbstractProgram {
public:
    ProgramOccupancyMap();
    ~ProgramOccupancyMap();
    LocationsMVP locationsMVP;
};

class ProgramDistanceMapSeed: public AbstractProgram {
public:
    ProgramDistanceMapSeed();
    ~ProgramDistanceMapSeed();
    LocationsMVP locationsMVP;
    struct Locations {
        GLint resolution;
        Locations()
                : resolution(-1) {
        }
    } locations;
};

class ProgramDistanceMapJFA: public AbstractProgram {
public:
    ProgramDistanceMapJFA();
    ~ProgramDistanceMapJFA();
    struct Locations {
        GLint textureUnit;
        GLint stepSize;
        GLint resolution;
        Locations()
                : textureUnit(-1), stepSize(-1), resolution(-1) {
        }
    } locations;
};

class ProgramDistanceMap: public AbstractProgram {
public:
    ProgramDistanceMap();
    ~ProgramDistanceMap();
    struct Locations {
        GLint textureUnit;
        GLint resolution;
        Locations()
                : textureUnit(-1), resolution(-1) {
        }
    } locations;
};

class ProgramCostMap: public AbstractProgram {
public:
    ProgramCostMap();
    ~ProgramCostMap();
    struct Locations {
        GLint textureUnit;
        GLint resolution;
        Locations()
                : textureUnit(-1), resolution(-1) {
        }
    } locations;
};

class ProgramCostMapVisual: public AbstractProgram {
public:
    ProgramCostMapVisual();
    ~ProgramCostMapVisual();
    struct Locations {
        GLint textureUnit;
        GLint resolution;
        Locations()
                : textureUnit(-1), resolution(-1) {
        }
    } locations;
};

class ProgramBellmanFordInit: public AbstractProgram {
public:
    ProgramBellmanFordInit();
    ~ProgramBellmanFordInit();
    //LocationsMVP locationsMVP;
    struct Locations {
        GLint resolution;
        GLint costmapTextureUnit;
        GLint robotPixel;
        Locations()
                : resolution(-1), costmapTextureUnit(-1), robotPixel(-1) {
        }
    } locations;
};

class ProgramBellmanFordStep: public AbstractProgram {
public:
    ProgramBellmanFordStep();
    ~ProgramBellmanFordStep();
    struct Locations {
        GLint resolution;
        GLint costmapTextureUnit;
        GLint inputTextureUnit;
        Locations()
                : resolution(-1), costmapTextureUnit(-1), inputTextureUnit(-1) {
        }
    } locations;
};

class ProgramVisibility: public AbstractProgram {
public:
    ProgramVisibility();
    ~ProgramVisibility();
    struct Locations {
        GLint resolution;
        GLint textureUnit;
        Locations()
                : resolution(-1), textureUnit(-1) {
        }
    } locations;
    LocationsMaterial locationsMaterial;
    LocationsMVP locationsMVP;
};

class ProgramShowTexture: public AbstractProgram {
public:
    ProgramShowTexture();
    ~ProgramShowTexture();
    struct Locations {
        GLint textureUnit;
        Locations()
                : textureUnit(-1) {
        }
    } locations;
};

class ProgramPixelCounter: public AbstractProgram {
public:
    ProgramPixelCounter();
    ~ProgramPixelCounter();
    struct Locations {
        GLint textureUnit;
        Locations()
                : textureUnit(-1) {
        }
    } locations;
};

class ProgramCounterToFB: public AbstractProgram {
public:
    ProgramCounterToFB();
    ~ProgramCounterToFB();
};

class ProgramVisualizeIntTexture: public AbstractProgram {
public:
    ProgramVisualizeIntTexture();
    ~ProgramVisualizeIntTexture();
    struct Locations {
        GLint textureUnit;
        GLint minDist;
        GLint maxDist;
        Locations()
                : textureUnit(-1), minDist(-1), maxDist(-1) {
        }
    } locations;
};

class ProgramTLEdge: public AbstractProgram {
public:
    ProgramTLEdge();
    ~ProgramTLEdge();
    struct Locations {
        GLint textureUnit;
        GLint width;
        GLint height;
        Locations()
                : textureUnit(-1), width(-1), height(-1) {
        }
    } locations;
};

class ProgramTLStep: public AbstractProgram {
public:
    ProgramTLStep();
    ~ProgramTLStep();
    struct Locations {
        GLint textureUnit;
        GLint width;
        GLint height;
        Locations()
                : textureUnit(-1), width(-1), height(-1) {
        }
    } locations;
};

class ProgramBellmanFordXfbInit : public AbstractProgram {
public:
	ProgramBellmanFordXfbInit();
	~ProgramBellmanFordXfbInit();
    struct Locations {
        GLint resolution;
        GLint robotPosition;
        Locations()
                : resolution(-1), robotPosition(-1) {
        }
    } locations;
};

class ProgramBellmanFordXfbStep : public AbstractProgram {
public:
	ProgramBellmanFordXfbStep();
	~ProgramBellmanFordXfbStep();
    struct Locations {
        GLint textureUnit;
        GLint resolution;
        Locations()
                : textureUnit(-1), resolution(-1) {
        }
    } locations;
};


class ProgramCostmapIndex : public AbstractProgram {
public:
    ProgramCostmapIndex();
    ~ProgramCostmapIndex();
    struct Locations {
        GLint textureUnit;
        GLint width;
        GLint height;
        Locations()
                : textureUnit(-1), width(-1), height(-1) {
        }
    } locations;
};

class ProgramUtility1 : public AbstractProgram {
public:
    ProgramUtility1();
    ~ProgramUtility1();
    struct Locations {
        GLint utilityUnit;
        GLint gain1Unit;
        Locations()
                : utilityUnit(-1), gain1Unit(-1) {
        }
    } locations;
};

class ProgramUtility2 : public AbstractProgram {
public:
    ProgramUtility2();
    ~ProgramUtility2();
    struct Locations {
        GLint utilityUnit;
        GLint gain1Unit;
        GLint gain2Unit;
        Locations()
                : utilityUnit(-1), gain1Unit(-1), gain2Unit(-1) {
        }
    } locations;
};


} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_PROGRAMS_H_ */
