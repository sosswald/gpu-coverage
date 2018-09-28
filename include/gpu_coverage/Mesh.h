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

#ifndef MESH_H_
#define MESH_H_

#include GL_INCLUDE
#include <gpu_coverage/Bone.h>
#include <gpu_coverage/Programs.h>

#include <assimp/scene.h>
#include <assimp/mesh.h>

#include <vector>

namespace gpu_coverage {

// Forward declarations
struct LocationsCommonRender;
class Material;

/**
 * @brief Class representing a mesh, corresponding to Assimp's aiMesh.
 */
class Mesh {
public:
    /**
     * @brief Buffer semantics.
     *
     * This enum is also used by other classes that render mesh-like visible objects,
     * e.g. CoordinateAxes and Dot.
     */
    enum BUFFERS {
        VERTEX_BUFFER, COLOR_BUFFER, TEXCOORD_BUFFER, NORMAL_BUFFER, INDEX_BUFFER
    };
    typedef std::vector<Bone*> Bones;    ///< Vector of bones attached to this mesh

    /**
     * @brief Constructor.
     * @param mesh The Assimp aiMesh for creating this mesh.
     * @param id Unique ID of this mesh.
     * @param materials Vector of materials loaded beforehand.
     */
    Mesh(const aiMesh * const mesh, const size_t id, const std::vector<Material*>& materials);

    /**
     * @brief Destructor.
     */
    ~Mesh();

    /**
     * @brief Renders the mesh.
     * @param locations Location variables of the material variables in the current shader.
     * @param hasTesselationShader True if a tesselation shader is active.
     */
    void render(const LocationsMaterial * const locations, const bool hasTesselationShader) const;

    /**
     * @brief Write Graphviz %Dot node representing this mesh to file for debugging.
     * @param[in] dot Output %Dot file.
     */
    void toDot(FILE *dot) const;

    /**
     * @brief Returns the unique ID of this mesh.
     * @return ID.
     */
    inline size_t getId() {
        return id;
    }

    /**
     * @brief Returns the name of this mesh for logging.
     * @return Name of this mesh.
     *
     * If available, the name included in the input file read by Assimp is used,
     * otherwise a generic string is constructed.
     */
    inline const std::string& getName() {
        return name;
    }

    /**
     * Returns the bones attached to this mesh.
     * @return Vector of bones.
     */
    inline Bones& getBones() {
        return bones;
    }

    /**
     * Returns the material associated with this mesh if applicable, otherwise NULL
     * @return Material or NULL.
     */
    inline Material *getMaterial() {
        return material;
    }

protected:
    const size_t id;             ///< Unique ID, see getId().
    const std::string name;      ///< Name of this mesh, see getName().
    Material *material;          ///< Material associated with this mesh, see getMaterial().
    GLuint vao;                  ///< Vertex array object.
    GLuint vbo[5];               ///< Vertex buffers (vertex position, color, texture coordinate, normal, index)

    unsigned int elementCount;   ///< Number of indices used in the primitives to draw
    Bones bones;                 ///< Bones associated with this mesh.
};

}

#endif
