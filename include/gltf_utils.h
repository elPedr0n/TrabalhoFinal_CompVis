#ifndef GLTF_UTILS_H
#define GLTF_UTILS_H

#include "tiny_gltf.h"

#include <vector>
#include <cstdint>

#include <glm/glm.hpp>

// ============================================================
// MESH DATA
// ============================================================

struct GLTFMeshData
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<uint32_t>  indices;
};

// ============================================================
// HELPER: FACE NORMAL
// ============================================================

inline glm::vec3 computeFaceNormal(
    const glm::vec3 &v0,
    const glm::vec3 &v1,
    const glm::vec3 &v2)
{
    return glm::normalize(glm::cross(v1 - v0, v2 - v0));
}

// ============================================================
// TEMPLATE IMPLEMENTATION
// MUST STAY IN HEADER
// ============================================================

template<typename T>
void computeNormalsForGLTF(tinygltf::Model &model)
{
    for (auto &mesh : model.meshes)
    {
        for (auto &primitive : mesh.primitives)
        {
            // Skip if normals already exist
            if (primitive.attributes.find("NORMAL")
                != primitive.attributes.end())
            {
                continue;
            }

            // Only support triangles
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                continue;

            // Must have indices
            if (primitive.indices < 0)
                continue;

            // ====================================================
            // POSITION ACCESSOR
            // ====================================================

            int posAccessorIdx =
                primitive.attributes["POSITION"];

            const auto &posAccessor =
                model.accessors[posAccessorIdx];

            const auto &posBufferView =
                model.bufferViews[posAccessor.bufferView];

            const auto &posBuffer =
                model.buffers[posBufferView.buffer];

            const float *positions =
                reinterpret_cast<const float*>(
                    &posBuffer.data[
                        posBufferView.byteOffset +
                        posAccessor.byteOffset
                    ]
                );

            // ====================================================
            // INDEX ACCESSOR
            // ====================================================

            const auto &idxAccessor =
                model.accessors[primitive.indices];

            const auto &idxBufferView =
                model.bufferViews[idxAccessor.bufferView];

            const auto &idxBuffer =
                model.buffers[idxBufferView.buffer];

            const T *indices =
                reinterpret_cast<const T*>(
                    &idxBuffer.data[
                        idxBufferView.byteOffset +
                        idxAccessor.byteOffset
                    ]
                );

            size_t vertexCount =
                posAccessor.count;

            size_t indexCount =
                idxAccessor.count;

            std::vector<glm::vec3> tempNormals(
                vertexCount,
                glm::vec3(0.0f)
            );

            // ====================================================
            // ACCUMULATE FACE NORMALS
            // ====================================================

            for (size_t i = 0; i + 2 < indexCount; i += 3)
            {
                uint32_t i0 = indices[i + 0];
                uint32_t i1 = indices[i + 1];
                uint32_t i2 = indices[i + 2];

                glm::vec3 v0(
                    positions[3 * i0 + 0],
                    positions[3 * i0 + 1],
                    positions[3 * i0 + 2]
                );

                glm::vec3 v1(
                    positions[3 * i1 + 0],
                    positions[3 * i1 + 1],
                    positions[3 * i1 + 2]
                );

                glm::vec3 v2(
                    positions[3 * i2 + 0],
                    positions[3 * i2 + 1],
                    positions[3 * i2 + 2]
                );

                glm::vec3 n =
                    computeFaceNormal(v0, v1, v2);

                tempNormals[i0] += n;
                tempNormals[i1] += n;
                tempNormals[i2] += n;
            }

            // ====================================================
            // NORMALIZE
            // ====================================================

            for (auto &n : tempNormals)
            {
                float len = glm::length(n);

                if (len > 0.000001f)
                    n /= len;
            }

            // ====================================================
            // TODO:
            // WRITE NORMALS BACK TO GLTF MODEL
            // ====================================================

            // Currently only computed in memory.
        }
    }
}

// ============================================================
// NON-TEMPLATE FUNCTION DECLARATION
// ============================================================

void buildTrianglesAndAddToVirtualSceneFromGLTF(
    const tinygltf::Model &model
);

#endif // GLTF_UTILS_H