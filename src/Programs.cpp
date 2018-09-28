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

#include <gpu_coverage/Programs.h>
#include <gpu_coverage/Utilities.h>
#include <gpu_coverage/Config.h>
#include GL_INCLUDE
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <map>
#include <set>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/detail/type_mat.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gpu_coverage {

static const std::string CORE = "CORE";
static const std::string NOTFOUND = "NOTFOUND";
static std::string versionString = "#version 440";
typedef std::map<std::string, std::string> ExtensionMap;
static ExtensionMap extensionMap;
static struct {
    const char * const name;
    const int glslCore;
    const int esCore;
} EXTENSIONS[] = {
        { "shader_image_load_store", 430, 330 },
        { "explicit_attrib_location", 430, 300 },
        { "shader_atomic_counters", 420, 310 },
        { "shader_image_load_store", 420, 310 },
        { "shader_image_atomic", 420, 320 },
        { "shading_language_420pack", 420, 0 },
        { "tessellation_shader", 400, 320 },
        { "gpu_shader5", 400, 300 },
        { "shading_language_packing", 400, 300 },
        { "shader_bit_encoding", 330, 300 },
        { "geometry_shader4", 320, 0 },
        { "texture_array", 300, 0 },
        { "texture_cube_map", 0, 100 },
        { "depth_texture_cube_map", 0, 100 },
        { "", -1, -1 }
};
static const char *EXTENSION_CLASSES[] = {
        "ARB",
        "EXT",
        "OES",
        NULL
};

static const char * getShortFilename(const char * name) {
    const char * result = name;
    size_t foundSlash = false;
    for (size_t i = strlen(name) - 1; i > 0; --i) {
        if (*(name + i) == '/') {
            if (foundSlash) {
                result = name + i + 1;
                break;
            } else {
                foundSlash = true;
            }
        }
    }
    return result;
}

AbstractProgram::AbstractProgram()
        : program(glCreateProgram()), ready(false) {
}

AbstractProgram::~AbstractProgram() {
    glDeleteProgram(program);
}

void AbstractProgram::setOpenGLVersion() {
    std::set<std::string> availableExtensions;
    const char * const shadingLanguageVersion = (const char *) glGetString(GL_SHADING_LANGUAGE_VERSION);
    logInfo("shading language version = %s", shadingLanguageVersion);
    GLint numExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (int i = 0; i < numExtensions; ++i) {
        const char * const extensionName = (const char *) glGetStringi(GL_EXTENSIONS, i);
        availableExtensions.insert(std::string(extensionName));
    }

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    GLint version = major * 100 + minor * 10;
    std::stringstream ss;
    ss << "#version " << version;
#if HAS_GLES
    ss << " es";
#endif
    versionString = ss.str();
    extensionMap.clear();
    for (size_t i = 0; EXTENSIONS[i].glslCore != -1; ++i) {
#if HAS_GLES
        const int& coreVersion = EXTENSIONS[i].esCore;
#else
        const int& coreVersion = EXTENSIONS[i].glslCore;
#endif
        if (version >= coreVersion) {
            extensionMap[EXTENSIONS[i].name] = CORE;
        } else {
            bool found = false;
            for (size_t k = 0; !found && EXTENSION_CLASSES[k] != NULL; ++k) {
                std::string ename = std::string("GL_") + EXTENSION_CLASSES[k] + "_" + EXTENSIONS[i].name;
                if (availableExtensions.find(ename) != availableExtensions.end()) {
                    extensionMap[EXTENSIONS[i].name] = ename;
                    found = true;
                }
            }
            if (!found) {
                extensionMap[EXTENSIONS[i].name] = NOTFOUND;
            }
        }
    }
}

void AbstractProgram::use() const {
    glUseProgram(program);
}

GLuint AbstractProgram::loadShader(const GLenum shaderType, const char * const filename) const {
    GLuint result = 0;
    std::stringstream ss;
    char line[2048];
    size_t lineNo = 0;

#if HAS_GLES
    bool hasWrittenPrecision = false;
#endif


#ifdef __ANDROID__
    AAssetManager * const apkAssetManager = Config::getAssetManager();
    if (!apkAssetManager) {
        logError("Asset manager not initialized");
        return 0;
    }
    AAsset* asset = AAssetManager_open(apkAssetManager, filename[0] == '/' ? &filename[1] : filename, AASSET_MODE_UNKNOWN);
    if (!asset) {
        logError("Could not open shader file %s", filename);
        return 0;
    }
    const int length = AAsset_getLength(asset);
    char * data = (char *) calloc(static_cast<size_t>(length + 1), sizeof(data));
    int bytesRead = AAsset_read(asset, data, length);
    if (bytesRead < length) {
        logError("Asset read did not return full file");
    }
    AAsset_close(asset);
    data[length] = 0;  // Asset text seems not to be zero terminated
    std::stringstream ifs(data);
#else
    std::ifstream ifs(filename);
#endif
    if (!ifs.good()) {
        logError("Could not open shader file %s", filename);
        goto end;
    }
    while (!ifs.eof()) {
        ++lineNo;
        ifs.getline(line, sizeof(line));
        if (ifs.fail() && !ifs.eof()) {
            logError("Could not read shader file %s:%zu", filename, lineNo);
            goto end;
        }
        if (strncmp(line, "#version", 8) == 0) {
            ss << versionString << std::endl;
        } else if (strncmp(line, "// EXTENSION ", 13) == 0) {
            const char * extension = strtok(&line[13], " ");
            if (!extension) {
                logError("Syntax error in shader file %s:%zu", filename, lineNo);
                goto end;
            }
            ExtensionMap::const_iterator eIt = extensionMap.find(std::string(extension));
            if (eIt == extensionMap.end()) {
                logError("Unknown extension %s in shader file %s:%zu", extension, filename, lineNo);
                goto end;
            }
            if (eIt->second == CORE) {
                // discard extension line
                ss << std::endl;
            } else if (eIt->second == NOTFOUND) {
                logError("Could not find required extension %s for shader file %s", extension, filename);
            } else {
                ss << "#extension " << eIt->second << " : require" << std::endl;
            }
        } else {
#if HAS_GLES
            if (!hasWrittenPrecision && line[0] != '#') {
                ss << "precision highp float;" << std::endl << "precision highp int;" << std::endl;
                hasWrittenPrecision = true;
            }
#endif

            ss << line << std::endl;
        }
    }
    result = createShader(shaderType, ss.str().c_str(), filename);
    end:
    #if __ANDROID__
    free(data);
#else
    ifs.close();
#endif
    return result;
}

GLuint AbstractProgram::createShader(const GLenum shaderType, const GLchar* code, const char * const name) const {
    const GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        logError("Could not create shader: %u", glGetError());
        std::stringstream ss;
        ss << "Could not create shader: " << glGetError();
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH,
                ss.str().size(), ss.str().c_str());
        return 0;
    }
    const GLint shaderLength = strlen(code);
    glShaderSource(shader, 1, &code, &shaderLength);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint blen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &blen);
        if (blen > 1) {
            GLchar* compiler_log = (GLchar*) (malloc(static_cast<size_t>(blen)));
            GLsizei slen = 0;
            glGetShaderInfoLog(shader, blen, &slen, compiler_log);
            logError("Could not compile shader %s: %s\n----\n%s\n----", getShortFilename(name), compiler_log, code);
            free(compiler_log);
        } else {
            logError("Could not compile shader %s.", getShortFilename(name));
        }
        return 0;
    }
    return shader;
}

bool AbstractProgram::link(const char * const name) const {
    glLinkProgram(program);
    GLint linked = GL_FALSE;
    GLint valid = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked) {
        glValidateProgram(program);
        glGetProgramiv(program, GL_VALIDATE_STATUS, &valid);
    }
    if (!valid) {
        GLint blen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &blen);
        if (blen > 1) {
            GLchar *compiler_log = (GLchar*) malloc(blen);
            GLsizei slen = 0;
            glGetProgramInfoLog(program, blen, &slen, compiler_log);
            logError("Could not %s %s shader program: %s", linked ? "validate" : "link", name, compiler_log);
            free(compiler_log);
        } else {
            logError("Could not %s %s shader program.", linked ? "validate" : "link", name);
        }
        return false;
    }
    return true;
}

ProgramPano::ProgramPano() {
    hasTesselationShader = Config::getInstance().getParam<bool>("tesselate");

    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/pano/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint geometryShader = loadShader(GL_GEOMETRY_SHADER, DATADIR "/shaders/pano/geometry.shader");
    if (geometryShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/pano/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    GLuint tesselationControlShader;
    GLuint tesselationEvaluationShader;
    if (hasTesselationShader) {
        tesselationControlShader = loadShader(GL_TESS_CONTROL_SHADER,
                DATADIR "/shaders/pano/tesselation-control.shader");
        tesselationEvaluationShader = loadShader(GL_TESS_EVALUATION_SHADER,
                DATADIR "/shaders/pano/tesselation-evaluation.shader");
        if (tesselationControlShader == 0 || tesselationEvaluationShader == 0) {
            return;
        }
    } else {
        tesselationControlShader = 0;
        tesselationEvaluationShader = 0;
    }

    glAttachShader(program, vertexShader);
    if (hasTesselationShader) {
        glAttachShader(program, tesselationControlShader);
        glAttachShader(program, tesselationEvaluationShader);
    }
    glAttachShader(program, geometryShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("pano");
    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    if (hasTesselationShader) {
        glDeleteShader(tesselationControlShader);
        glDeleteShader(tesselationEvaluationShader);
        hasTesselationShader = true;
    } else {
        hasTesselationShader = false;
    }
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locationsMVP.modelMatrix = glGetUniformLocation(program, "model_matrix");
    locationsMVP.projectionMatrix = glGetUniformLocation(program, "projection_matrix");
    locationsLight.lightPosition = glGetUniformLocation(program, "light.position");
    locationsLight.lightAmbient = glGetUniformLocation(program, "light.ambient");
    locationsLight.lightDiffuse = glGetUniformLocation(program, "light.diffuse");
    locationsLight.lightSpecular = glGetUniformLocation(program, "light.specular");
    locationsMaterial.materialAmbient = glGetUniformLocation(program, "material.ambient");
    locationsMaterial.materialDiffuse = glGetUniformLocation(program, "material.diffuse");
    locationsMaterial.materialSpecular = glGetUniformLocation(program, "material.specular");
    locationsMaterial.materialShininess = glGetUniformLocation(program, "material.shininess");
    locationsMaterial.textureUnit = glGetUniformLocation(program, "texture_unit");
    locationsMaterial.hasTexture = glGetUniformLocation(program, "has_texture");
    for (size_t i = 0; i < 6; ++i) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "view_matrix[%zu]", i);
        locationsMVP.viewMatrix[i] = glGetUniformLocation(program, buffer);
        snprintf(buffer, sizeof(buffer), "normal_matrix[%zu]", i);
        locationsMVP.normalMatrix[i] = glGetUniformLocation(program, buffer);
    }

    checkGLError();
    ready = true;
}

ProgramPano::~ProgramPano() {
}

ProgramPanoSemantic::ProgramPanoSemantic() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/pano-semantic/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint geometryShader = loadShader(GL_GEOMETRY_SHADER, DATADIR "/shaders/pano-semantic/geometry.shader");
    if (geometryShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/pano-semantic/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, geometryShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("pano-semantic");
    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locationsMVP.modelMatrix = glGetUniformLocation(program, "model_matrix");
    locationsMVP.projectionMatrix = glGetUniformLocation(program, "projection_matrix");
    locationsMaterial.textureUnit = glGetUniformLocation(program, "texture_unit");
    locationsMaterial.hasTexture = glGetUniformLocation(program, "has_texture");
    for (size_t i = 0; i < 6; ++i) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "view_matrix[%zu]", i);
        locationsMVP.viewMatrix[i] = glGetUniformLocation(program, buffer);
        snprintf(buffer, sizeof(buffer), "normal_matrix[%zu]", i);
        locationsMVP.normalMatrix[i] = glGetUniformLocation(program, buffer);
    }

    checkGLError();
    ready = true;
}

ProgramPanoSemantic::~ProgramPanoSemantic() {
}

ProgramPanoEval::ProgramPanoEval() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/pano-eval/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/pano-eval/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("pano-eval");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    locations.integral = glGetUniformLocation(program, "integral");
    locations.resolution = glGetUniformLocation(program, "resolution");

    checkGLError();
    ready = true;
}

ProgramPanoEval::~ProgramPanoEval() {

}

ProgramVisualTexture::ProgramVisualTexture() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/visual-texture/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/visual-texture/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("visual-texture");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locationsMVP.modelMatrix = glGetUniformLocation(program, "model_matrix");
    locationsMVP.viewMatrix[0] = glGetUniformLocation(program, "view_matrix");
    locationsMVP.projectionMatrix = glGetUniformLocation(program, "projection_matrix");
    locationsMVP.normalMatrix[0] = glGetUniformLocation(program, "normal_matrix");
    locationsLight.lightPosition = glGetUniformLocation(program, "light.position");
    locationsLight.lightAmbient = glGetUniformLocation(program, "light.ambient");
    locationsLight.lightDiffuse = glGetUniformLocation(program, "light.diffuse");
    locationsLight.lightSpecular = glGetUniformLocation(program, "light.specular");
    locationsMaterial.materialAmbient = glGetUniformLocation(program, "material.ambient");
    locationsMaterial.materialDiffuse = glGetUniformLocation(program, "material.diffuse");
    locationsMaterial.materialSpecular = glGetUniformLocation(program, "material.specular");
    locationsMaterial.materialShininess = glGetUniformLocation(program, "material.shininess");
    locationsMaterial.textureUnit = glGetUniformLocation(program, "texture_unit");
    locationsMaterial.hasTexture = glGetUniformLocation(program, "has_texture");
    for (size_t i = 1; i < 6; ++i) {
        locationsMVP.viewMatrix[i] = -1;
        locationsMVP.normalMatrix[i] = -1;
    }

    checkGLError();
    ready = true;
}

ProgramVisualTexture::~ProgramVisualTexture() {
}

ProgramVisualFlat::ProgramVisualFlat() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/visual-flat/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/visual-flat/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("visual-flat");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locationsMVP.modelMatrix = glGetUniformLocation(program, "model_matrix");
    locationsMVP.viewMatrix[0] = glGetUniformLocation(program, "view_matrix");
    locationsMVP.projectionMatrix = glGetUniformLocation(program, "projection_matrix");
    locations.color = glGetUniformLocation(program, "color");

    checkGLError();
    ready = true;
}

ProgramVisualFlat::~ProgramVisualFlat() {
}

ProgramVisualVertexcolor::ProgramVisualVertexcolor() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/visual-vertexcolor/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/visual-vertexcolor/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("visual-vertexcolor");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locationsMVP.modelMatrix = glGetUniformLocation(program, "model_matrix");
    locationsMVP.viewMatrix[0] = glGetUniformLocation(program, "view_matrix");
    locationsMVP.projectionMatrix = glGetUniformLocation(program, "projection_matrix");
    locationsMVP.normalMatrix[0] = -1;

    checkGLError();
    ready = true;
}

AbstractProgramMapProjection::AbstractProgramMapProjection(const char * const name) {
    std::stringstream vertexShaderFilename, fragmentShaderFilename;
    vertexShaderFilename << DATADIR "/shaders/" << name << "/vertex.shader";
    fragmentShaderFilename << DATADIR "/shaders/" << name << "/fragment.shader";
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderFilename.str().c_str());
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderFilename.str().c_str());
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("name");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }
    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    checkGLError();
    ready = true;
}

ProgramVisualVertexcolor::~ProgramVisualVertexcolor() {
}

ProgramFlatProjection2dArray::ProgramFlatProjection2dArray()
        : AbstractProgramMapProjection("flat-projection-2darray") {
}

ProgramFlatProjection2dArray::~ProgramFlatProjection2dArray() {
}

ProgramFlatProjectionCube::ProgramFlatProjectionCube()
        : AbstractProgramMapProjection("flat-projection-cubemap") {
}

ProgramFlatProjectionCube::~ProgramFlatProjectionCube() {
}

AbstractProgramMapProjection::~AbstractProgramMapProjection() {
}

ProgramEquirectangular::ProgramEquirectangular()
        : AbstractProgramMapProjection("equirectangular") {
}

ProgramEquirectangular::~ProgramEquirectangular() {
}

ProgramCylindrical::ProgramCylindrical()
        : AbstractProgramMapProjection("cylindrical") {
}

ProgramCylindrical::~ProgramCylindrical() {
}

ProgramDistanceMapJFA::ProgramDistanceMapJFA() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/distance-map-jfa/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/distance-map-jfa/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("distance-map-jfa");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    locations.stepSize = glGetUniformLocation(program, "step_size");
    locations.resolution = glGetUniformLocation(program, "resolution");

    checkGLError();
    ready = true;
}

ProgramDistanceMapJFA::~ProgramDistanceMapJFA() {
}

ProgramOccupancyMap::ProgramOccupancyMap() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/occupancy-map/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/occupancy-map/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("occupancy-map");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locationsMVP.modelMatrix = glGetUniformLocation(program, "model_matrix");
    locationsMVP.viewMatrix[0] = glGetUniformLocation(program, "view_matrix");
    locationsMVP.projectionMatrix = glGetUniformLocation(program, "projection_matrix");
    locationsMVP.normalMatrix[0] = -1;
    checkGLError();
    ready = true;
}

ProgramDistanceMapSeed::~ProgramDistanceMapSeed() {
}

ProgramDistanceMapSeed::ProgramDistanceMapSeed() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/distance-map-seed/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/distance-map-seed/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("distance-map-seed");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locationsMVP.modelMatrix = glGetUniformLocation(program, "model_matrix");
    locationsMVP.viewMatrix[0] = glGetUniformLocation(program, "view_matrix");
    locationsMVP.projectionMatrix = glGetUniformLocation(program, "projection_matrix");
    locationsMVP.normalMatrix[0] = -1;
    locations.resolution = glGetUniformLocation(program, "resolution");
    checkGLError();
    ready = true;
}

ProgramOccupancyMap::~ProgramOccupancyMap() {
}

ProgramDistanceMap::ProgramDistanceMap() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/distance-map/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/distance-map/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("distance-map");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    locations.resolution = glGetUniformLocation(program, "resolution");

    checkGLError();
    ready = true;
}

ProgramDistanceMap::~ProgramDistanceMap() {
}

ProgramCostMap::ProgramCostMap() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/costmap/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/costmap/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("costmap");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    locations.resolution = glGetUniformLocation(program, "resolution");

    checkGLError();
    ready = true;
}

ProgramCostMapVisual::ProgramCostMapVisual() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/costmap-visual/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/costmap-visual/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("costmap-visual");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    locations.resolution = glGetUniformLocation(program, "resolution");

    checkGLError();
    ready = true;
}

ProgramCostMapVisual::~ProgramCostMapVisual() {

}


ProgramCostMap::~ProgramCostMap() {
}

ProgramBellmanFordInit::ProgramBellmanFordInit() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/bellman-ford-init/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/bellman-ford-init/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("bellman-ford-init");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.resolution = glGetUniformLocation(program, "resolution");
    locations.costmapTextureUnit = glGetUniformLocation(program, "costmap_texture_unit");
    locations.robotPixel = glGetUniformLocation(program, "robot_pixel");
    /*locationsMVP.modelMatrix              = glGetUniformLocation(program, "model_matrix");
     locationsMVP.viewMatrix[0]            = glGetUniformLocation(program, "view_matrix");
     locationsMVP.projectionMatrix         = glGetUniformLocation(program, "projection_matrix");
     locationsMVP.normalMatrix[0]          = -1;*/

    checkGLError();
    ready = true;
}

ProgramBellmanFordInit::~ProgramBellmanFordInit() {
}

ProgramBellmanFordStep::ProgramBellmanFordStep() {
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/bellman-ford-step/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/bellman-ford-step/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("bellman-ford-step");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.resolution = glGetUniformLocation(program, "resolution");
    locations.costmapTextureUnit = glGetUniformLocation(program, "costmap_texture_unit");
    locations.inputTextureUnit = glGetUniformLocation(program, "input_texture_unit");

    checkGLError();
    ready = true;
}

ProgramBellmanFordStep::~ProgramBellmanFordStep() {
}

ProgramVisibility::ProgramVisibility() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/visibility/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/visibility/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("visibility");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.resolution = glGetUniformLocation(program, "resolution");
    locations.textureUnit = glGetUniformLocation(program, "texture_unit");

    locationsMaterial.materialAmbient = -1;
    locationsMaterial.materialDiffuse = -1;
    locationsMaterial.materialSpecular = -1;
    locationsMaterial.materialShininess = -1;
    locationsMaterial.hasTexture = -1;
    locationsMaterial.textureUnit = glGetUniformLocation(program, "texture_unit");

    locationsMVP.modelMatrix = glGetUniformLocation(program, "model_matrix");
    locationsMVP.viewMatrix[0] = glGetUniformLocation(program, "view_matrix");
    locationsMVP.projectionMatrix = glGetUniformLocation(program, "projection_matrix");

    checkGLError();
    ready = true;
}

ProgramVisibility::~ProgramVisibility() {
}

ProgramShowTexture::ProgramShowTexture() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/show-texture/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/show-texture/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("show-texture");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.textureUnit = glGetUniformLocation(program, "texture_unit");

    checkGLError();
    ready = true;
}

ProgramShowTexture::~ProgramShowTexture() {

}

ProgramPixelCounter::ProgramPixelCounter() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/pixel-counter/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/pixel-counter/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("pixel-counter");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.textureUnit = glGetUniformLocation(program, "input_texture_unit");

    checkGLError();
    ready = true;
}

ProgramPixelCounter::~ProgramPixelCounter() {

}

ProgramCounterToFB::ProgramCounterToFB() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/counter-to-fb/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/counter-to-fb/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("counter-to-fb");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    checkGLError();
    ready = true;
}

ProgramCounterToFB::~ProgramCounterToFB() {
}

ProgramVisualizeIntTexture::ProgramVisualizeIntTexture() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/visualize-int-texture/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER,
            DATADIR "/shaders/visualize-int-texture/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("visualize-int-texture");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }
    locations.textureUnit = glGetUniformLocation(program, "input_texture_unit");
    locations.minDist = glGetUniformLocation(program, "min_dist");
    locations.maxDist = glGetUniformLocation(program, "max_dist");

    checkGLError();
    ready = true;
}

ProgramVisualizeIntTexture::~ProgramVisualizeIntTexture() {
}

ProgramTLEdge::ProgramTLEdge() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/tl-edge/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER,
            DATADIR "/shaders/tl-edge/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("tl-edge");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }
    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    locations.width = glGetUniformLocation(program, "width");
    locations.height = glGetUniformLocation(program, "height");

    checkGLError();
    ready = true;
}

ProgramTLEdge::~ProgramTLEdge() {
}

ProgramTLStep::ProgramTLStep() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/tl-step/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER,
            DATADIR "/shaders/tl-step/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("tl-step");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }
    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    locations.width = glGetUniformLocation(program, "width");
    locations.height = glGetUniformLocation(program, "height");

    checkGLError();
    ready = true;
}

ProgramTLStep::~ProgramTLStep() {
}

ProgramBellmanFordXfbInit::ProgramBellmanFordXfbInit() {
	checkGLError();
	const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/test-init/vertex.shader");
	if (vertexShader == 0) {
		return;
	}
	const GLuint geometryShader = loadShader(GL_GEOMETRY_SHADER, DATADIR "/shaders/test-init/geometry.shader");
	if (geometryShader == 0) {
		return;
	}
	const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER,
			DATADIR "/shaders/test-init/fragment.shader");
	if (fragmentShader == 0) {
		return;
	}

	glAttachShader(program, vertexShader);
	glAttachShader(program, geometryShader);
	glAttachShader(program, fragmentShader);
	const GLchar *feedbackVaryings[] = { "position" };
	glTransformFeedbackVaryings(program, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
	const bool isLinked = link("test-init");
	glDeleteShader(vertexShader);
	glDeleteShader(geometryShader);
	glDeleteShader(fragmentShader);
	if (!isLinked) {
		return;
	}

    locations.resolution = glGetUniformLocation(program, "resolution");
    locations.robotPosition = glGetUniformLocation(program, "robot_position");

	checkGLError();
	ready = true;
}

ProgramBellmanFordXfbInit::~ProgramBellmanFordXfbInit() {

}

ProgramBellmanFordXfbStep::ProgramBellmanFordXfbStep() {
	checkGLError();
#ifdef __ANDROID__
	const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/test-step/vertex-android.shader");
#else
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/test-step/vertex.shader");
#endif
	if (vertexShader == 0) {
		return;
	}

#ifdef __ANDROID__
	const GLuint geometryShader = loadShader(GL_GEOMETRY_SHADER, DATADIR "/shaders/test-step/geometry-android.shader");
#else
    const GLuint geometryShader = loadShader(GL_GEOMETRY_SHADER, DATADIR "/shaders/test-step/geometry.shader");
#endif
	if (geometryShader == 0) {
		return;
	}
	const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, DATADIR "/shaders/test-step/fragment.shader");
	if (fragmentShader == 0) {
		return;
	}

	glAttachShader(program, vertexShader);
	glAttachShader(program, geometryShader);
	glAttachShader(program, fragmentShader);
	const GLchar *feedbackVaryings[] = { "position" };
	glTransformFeedbackVaryings(program, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
	const bool isLinked = link("test-step");
	glDeleteShader(vertexShader);
	glDeleteShader(geometryShader);
	glDeleteShader(fragmentShader);
	if (!isLinked) {
		return;
	}

    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    locations.resolution = glGetUniformLocation(program, "resolution");

	checkGLError();
	ready = true;
}

ProgramBellmanFordXfbStep::~ProgramBellmanFordXfbStep() {

}

ProgramCostmapIndex::ProgramCostmapIndex() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/costmap-index/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER,
            DATADIR "/shaders/costmap-index/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("costmap-index");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.textureUnit = glGetUniformLocation(program, "texture_unit");
    locations.width = glGetUniformLocation(program, "width");
    locations.height = glGetUniformLocation(program, "height");

    checkGLError();
    ready = true;
}

ProgramCostmapIndex::~ProgramCostmapIndex() {

}

ProgramUtility1::ProgramUtility1() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/utility1/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER,
            DATADIR "/shaders/utility1/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("utility1");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.utilityUnit = glGetUniformLocation(program, "utility_unit");
    locations.gain1Unit = glGetUniformLocation(program, "gain1_unit");

    checkGLError();
    ready = true;
}

ProgramUtility1::~ProgramUtility1() {

}

ProgramUtility2::ProgramUtility2() {
    checkGLError();
    const GLuint vertexShader = loadShader(GL_VERTEX_SHADER, DATADIR "/shaders/utility2/vertex.shader");
    if (vertexShader == 0) {
        return;
    }
    const GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER,
            DATADIR "/shaders/utility2/fragment.shader");
    if (fragmentShader == 0) {
        return;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    const bool isLinked = link("utility2");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (!isLinked) {
        return;
    }

    locations.utilityUnit = glGetUniformLocation(program, "utility_unit");
    locations.gain1Unit = glGetUniformLocation(program, "gain1_unit");
    locations.gain2Unit = glGetUniformLocation(program, "gain2_unit");

    checkGLError();
    ready = true;
}

ProgramUtility2::~ProgramUtility2() {

}

} /* namespace gpu_coverage */
