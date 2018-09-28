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

#include <gpu_coverage/Light.h>
#include <gpu_coverage/Node.h>
#include <cstdio>

namespace gpu_coverage {

static const char * TYPE_STR[4] = {
        "undefined",
        "directional",
        "point",
        "spot"
};

Light::Light(const aiLight * const light, const size_t id, Node * const node)
        : id(id), name(light->mName.C_Str()), node(node), type(light->mType),
                diffuse(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b),
                ambient(light->mColorAmbient.r, light->mColorAmbient.g, light->mColorAmbient.b),
                specular(light->mColorSpecular.r, light->mColorSpecular.g, light->mColorSpecular.b) {
}

Light::~Light() {
}

void Light::toDot(FILE *dot) {
    fprintf(dot, "  l%zu [label=\"{%s|{Type|%s}}\", shape=\"record\"];\n", id, name.c_str(), TYPE_STR[type]);
    if (node) {
        fprintf(dot, "  n%zu -> l%zu;\n", node->getId(), id);
    }
}

} /* namespace gpu_coverage */
