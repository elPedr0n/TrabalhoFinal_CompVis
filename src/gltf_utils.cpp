#include "gltf_utils.h"

#include <iostream>
#include <unordered_map>

#include <glm/glm.hpp>

// Extract triangles from GLTF and add to virtual scene
#include <glad/glad.h>
#include <map>
#include <string>

#include "sceneobject.h"

#include "stb_image.h"

void buildTrianglesAndAddToVirtualSceneFromGLTF(const tinygltf::Model &model) {
    int mesh_count = 0;
    for (const auto &mesh : model.meshes) {
        for (const auto &primitive : mesh.primitives) {
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                continue;

            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> texcoords;
            std::vector<uint32_t> indices;

            // POSITIONS
            if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                int accessorIdx = primitive.attributes.at("POSITION");
                const auto &accessor = model.accessors[accessorIdx];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[bufferView.buffer];
                const float *positions_ptr = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i) {
                    positions.emplace_back(
                        positions_ptr[3 * i + 0],
                        positions_ptr[3 * i + 1],
                        positions_ptr[3 * i + 2]
                    );
                }
            }

            // NORMALS
            if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                int accessorIdx = primitive.attributes.at("NORMAL");
                const auto &accessor = model.accessors[accessorIdx];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[bufferView.buffer];
                const float *normals_ptr = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i) {
                    normals.emplace_back(
                        normals_ptr[3 * i + 0],
                        normals_ptr[3 * i + 1],
                        normals_ptr[3 * i + 2]
                    );
                }
            }

            // TEXCOORDS
            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                int accessorIdx = primitive.attributes.at("TEXCOORD_0");
                const auto &accessor = model.accessors[accessorIdx];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[bufferView.buffer];
                const float *texcoords_ptr = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i) {
                    texcoords.emplace_back(
                        texcoords_ptr[2 * i + 0],
                        texcoords_ptr[2 * i + 1]
                    );
                }
            }

            // INDICES
            if (primitive.indices >= 0) {
                const auto &accessor = model.accessors[primitive.indices];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[bufferView.buffer];
                const unsigned char *dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                for (size_t i = 0; i < accessor.count; ++i) {
                    uint32_t idx = 0;
                    switch (accessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            idx = ((const uint8_t*)dataPtr)[i]; break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            idx = ((const uint16_t*)dataPtr)[i]; break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                            idx = ((const uint32_t*)dataPtr)[i]; break;
                        default: break;
                    }
                    indices.push_back(idx);
                }
            }

            // OpenGL: Create VAO, VBOs, EBO
            GLuint vao, vbo_pos, vbo_norm, vbo_tex, vbo_mat, ebo;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            // Positions — convert vec3 to vec4 (W=1)
            {
                std::vector<glm::vec4> positions4;
                positions4.reserve(positions.size());
                for (const auto& p : positions)
                    positions4.emplace_back(p.x, p.y, p.z, 1.0f);

                glGenBuffers(1, &vbo_pos);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
                glBufferData(GL_ARRAY_BUFFER, positions4.size() * sizeof(glm::vec4), positions4.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(0);
            }

            // Normals — convert vec3 to vec4 (W=0)
            if (!normals.empty()) {
                std::vector<glm::vec4> normals4;
                normals4.reserve(normals.size());
                for (const auto& n : normals)
                    normals4.emplace_back(n.x, n.y, n.z, 0.0f);

                glGenBuffers(1, &vbo_norm);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_norm);
                glBufferData(GL_ARRAY_BUFFER, normals4.size() * sizeof(glm::vec4), normals4.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(1);
            }

            // Texcoords
            if (!texcoords.empty()) {
                glGenBuffers(1, &vbo_tex);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);
                glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(glm::vec2), texcoords.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(2);
            }

            // Material selector (location 3) — constant 4.0 for SWAMPFIRE
            {
                std::vector<float> mat_sel(positions.size(), 4.0f);
                glGenBuffers(1, &vbo_mat);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_mat);
                glBufferData(GL_ARRAY_BUFFER, mat_sel.size() * sizeof(float), mat_sel.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(3);
            }

            // Indices
            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

            glBindVertexArray(0);

            // --- TEXTURE ---
            GLuint texture_id = 0;
            if (primitive.material >= 0 && primitive.material < (int)model.materials.size()) {
                const auto& mat = model.materials[primitive.material];
                if (mat.values.find("baseColorTexture") != mat.values.end()) {
                    int texIndex = mat.values.at("baseColorTexture").TextureIndex();
                    if (texIndex >= 0 && texIndex < (int)model.textures.size()) {
                        const auto& tex = model.textures[texIndex];
                        int imgIdx = tex.source;
                        if (imgIdx >= 0 && imgIdx < (int)model.images.size()) {
                            const auto& img = model.images[imgIdx];
                            glGenTextures(1, &texture_id);
                            glBindTexture(GL_TEXTURE_2D, texture_id);
                            GLenum format = GL_RGBA;
                            if (img.component == 1) format = GL_RED;
                            else if (img.component == 2) format = GL_RG;
                            else if (img.component == 3) format = GL_RGB;
                            else if (img.component == 4) format = GL_RGBA;
                            glTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.image.data());
                            glGenerateMipmap(GL_TEXTURE_2D);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                            glBindTexture(GL_TEXTURE_2D, 0);
                        }
                    }
                }
            }

            // SceneObject
            SceneObject obj;
            obj.name = "the_swampfire_" + std::to_string(mesh_count);
            obj.first_index = 0;
            obj.num_indices = indices.size();
            obj.rendering_mode = GL_TRIANGLES;
            obj.vertex_array_object_id = vao;

            int width, height, channels;
            unsigned char* img_data = stbi_load("../../data/swampfire__ben_10_alien_force/textures/SwampFire_baseColor.png", &width, &height, &channels, 4);
            if (img_data) {
                GLuint swampfire_tex_id;
                glGenTextures(1, &swampfire_tex_id);
                glBindTexture(GL_TEXTURE_2D, swampfire_tex_id);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);
                obj.texture_id = swampfire_tex_id;
                stbi_image_free(img_data);
            } else {
                obj.texture_id = texture_id;
            }

            // Compute bbox
            if (!positions.empty()) {
                obj.bbox_min = positions[0];
                obj.bbox_max = positions[0];
                for (const auto& v : positions) {
                    obj.bbox_min = glm::min(obj.bbox_min, v);
                    obj.bbox_max = glm::max(obj.bbox_max, v);
                }
            }
            g_VirtualScene[obj.name] = obj;
            mesh_count++;
        }
    }
}