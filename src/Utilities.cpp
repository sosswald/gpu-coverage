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

#include <gpu_coverage/Utilities.h>
#ifdef GLU_INCLUDE
#include GLU_INCLUDE
#endif

namespace gpu_coverage {

GLException::GLException(const GLenum code, const char * const file, const int line)
        : code(code) {
#ifdef __ANDROID__
    ((void)__android_log_print(ANDROID_LOG_ERROR, "articulation", "%s:%d: GL error code %u", (strrchr(file, '/') ? strrchr(file, '/') + 1 : file), line, code));
#else
#ifdef GLU_INCLUDE
    fprintf(stderr, "[ERR ] %s:%d: GL error code %u: %s\n", (strrchr(file, '/') ? strrchr(file, '/') + 1 : file), line, code, gluErrorString(code));
#else
    fprintf(stderr, "[ERR ] %s:%d: GL error code %u\n", (strrchr(file, '/') ? strrchr(file, '/') + 1 : file), line, code);
#endif
#endif
}

GLException::~GLException() throw () {
}

const char* GLException::what() const throw () {
    return text.c_str();
}

void doCheckGLError(const char * const file, const int line) {
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        throw new GLException(error, file, line);
    }
}

} /* namespace gpu_coverage */
