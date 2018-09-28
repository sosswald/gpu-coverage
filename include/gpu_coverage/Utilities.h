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

#ifndef INCLUDE_ARTICULATION_UTILITIES_H_
#define INCLUDE_ARTICULATION_UTILITIES_H_

#include <cstdio>
#include <exception>
#include <cstring>
#include <string>
#include GL_INCLUDE

#if HAS_GLES
#define glPolygonMode(...)
#define glPushDebugGroup(...)
#define glPopDebugGroup(...)
#define GL_FILL 0x1B02
#endif

#define GET_API(FRAMEWORK) FRAMEWORK##_##OPENGL_API

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#ifdef __ANDROID__
#include <android/log.h>
#define logInfo(FORMAT, ...)  ((void)__android_log_print(ANDROID_LOG_INFO, "articulation", "%s:%d: " FORMAT, __FILENAME__, __LINE__, ##__VA_ARGS__))
#define logWarn(FORMAT, ...)  ((void)__android_log_print(ANDROID_LOG_WARN, "articulation", "%s:%d: " FORMAT, __FILENAME__, __LINE__, ##__VA_ARGS__))
#define logError(FORMAT, ...) ((void)__android_log_print(ANDROID_LOG_ERROR, "articulation", "%s:%d: " FORMAT, __FILENAME__, __LINE__, ##__VA_ARGS__))
#else
#define logInfo(FORMAT, ...)  fprintf(stdout, "[INFO] %s:%d: " FORMAT "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)
#define logWarn(FORMAT, ...)  fprintf(stderr, "[WARN] %s:%d: " FORMAT "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)
#define logError(FORMAT, ...) fprintf(stderr, "[ERR ] %s:%d: " FORMAT "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)
#endif

namespace gpu_coverage {

/**
 * @brief Custom OpenGL exception
 */
class GLException: public std::exception {
public:
    /**
     * @brief Constructor.
     * @param code OpenGL error code.
     * @param file File name.
     * @param line Line where the exception was thrown.
     */
    GLException(const GLenum code, const char * const file, const int line);
    /**
     * @brief Destructor.
     */
    virtual ~GLException() throw ();
    /**
     * @brief Returns the human-readable description of the error.
     * @return Error description.
     */
    const char* what() const throw ();

protected:
    const GLenum code;       ///< OpenGL error code.
    std::string text;        ///< Description string.
};

/**
 * @brief Check for OpenGL error.
 * @param file File name.
 * @param line Line number.
 * @exception GLException An OpenGL exception was detected.
 *
 * Use the checkGLError() macro to auto-fill the file name and line number.
 */
void doCheckGLError(const char * const file, const int line);
#ifndef checkGLError
#ifdef DEBUG
#define checkGLError() doCheckGLError(__FILE__, __LINE__)
#else
#define checkGLError()
#endif
#endif

} /* namespace gpu_coverage */

#endif /* INCLUDE_ARTICULATION_UTILITIES_H_ */
