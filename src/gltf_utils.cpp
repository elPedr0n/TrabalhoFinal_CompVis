#include "gltf_utils.h"

#include <iostream>
#include <unordered_map>

#include <glm/glm.hpp>

// Extract triangles from GLTF and add to virtual scene
#include <glad/glad.h>
#include <map>
#include <string>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>

#include "structs.h"
#include "sceneobject.h"

// texture loading should come from GLTF images; do not hardcode stbi_load here

void buildTrianglesAndAddToVirtualSceneFromGLTF(const tinygltf::Model &model, const std::string &base_name) {
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

            // JOINTS_0 (vec4 of unsigned ints) & WEIGHTS_0 (vec4 of floats)
            std::vector<glm::uvec4> joints; // will store as uvec4 (uint32) for GPU upload
            std::vector<glm::vec4> weights;

            if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
                int accessorIdx = primitive.attributes.at("JOINTS_0");
                const auto &accessor = model.accessors[accessorIdx];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[bufferView.buffer];
                const unsigned char *dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

                joints.reserve(accessor.count);
                for (size_t i = 0; i < accessor.count; ++i) {
                    glm::uvec4 j(0);
                    switch (accessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                            const uint8_t *src = reinterpret_cast<const uint8_t*>(dataPtr);
                            j.x = src[4 * i + 0]; j.y = src[4 * i + 1]; j.z = src[4 * i + 2]; j.w = src[4 * i + 3];
                        } break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                            const uint16_t *src = reinterpret_cast<const uint16_t*>(dataPtr);
                            j.x = src[4 * i + 0]; j.y = src[4 * i + 1]; j.z = src[4 * i + 2]; j.w = src[4 * i + 3];
                        } break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                            const uint32_t *src = reinterpret_cast<const uint32_t*>(dataPtr);
                            j.x = src[4 * i + 0]; j.y = src[4 * i + 1]; j.z = src[4 * i + 2]; j.w = src[4 * i + 3];
                        } break;
                        default:
                            break;
                    }
                    joints.push_back(j);
                }
            }

            if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
                int accessorIdx = primitive.attributes.at("WEIGHTS_0");
                const auto &accessor = model.accessors[accessorIdx];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[bufferView.buffer];
                const float *weights_ptr = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

                weights.reserve(accessor.count);
                for (size_t i = 0; i < accessor.count; ++i) {
                    weights.emplace_back(
                        weights_ptr[4 * i + 0],
                        weights_ptr[4 * i + 1],
                        weights_ptr[4 * i + 2],
                        weights_ptr[4 * i + 3]
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
            } else {
                // Se não houver índices definidos (malha não-indexada), geramos eles sequencialmente!
                for (uint32_t i = 0; i < positions.size(); ++i) {
                    indices.push_back(i);
                }
            }

            // COMPUTE NORMALS IF MISSING
            if (normals.empty() && !positions.empty() && indices.size() >= 3) {
                normals.resize(positions.size(), glm::vec3(0.0f));
                for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                    uint32_t i0 = indices[i];
                    uint32_t i1 = indices[i+1];
                    uint32_t i2 = indices[i+2];
                    glm::vec3 v0 = positions[i0];
                    glm::vec3 v1 = positions[i1];
                    glm::vec3 v2 = positions[i2];
                    glm::vec3 n = glm::cross(v1 - v0, v2 - v0);
                    normals[i0] += n;
                    normals[i1] += n;
                    normals[i2] += n;
                }
                for (auto& n : normals) {
                    if (glm::length(n) > 0.000001f) {
                        n = glm::normalize(n);
                    } else {
                        n = glm::vec3(0.0f, 1.0f, 0.0f);
                    }
                }
            }

            // OpenGL: Create VAO, VBOs, EBO
            GLuint vao, vbo_pos, vbo_norm, vbo_tex, vbo_mat, vbo_joints, vbo_weights, ebo;
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
                if (base_name == "the_bigchill_cloaked") {
                    std::ifstream infile("../../data/big_chill_cloaked_mats.txt");
                    if (infile.is_open()) {
                        float m;
                        int idx = 0;
                        while (infile >> m && idx < (int)mat_sel.size()) {
                            mat_sel[idx++] = m;
                        }
                    }
                }
                glGenBuffers(1, &vbo_mat);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_mat);
                glBufferData(GL_ARRAY_BUFFER, mat_sel.size() * sizeof(float), mat_sel.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(3);
            }

            // Joints (location 4) — upload as unsigned ints (vec4)
            if (!joints.empty()) {
                glGenBuffers(1, &vbo_joints);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_joints);
                glBufferData(GL_ARRAY_BUFFER, joints.size() * sizeof(glm::uvec4), joints.data(), GL_STATIC_DRAW);
                // integer attribute requires glVertexAttribIPointer
                glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, 0, 0);
                glEnableVertexAttribArray(4);
            }

            // Weights (location 5) — upload as vec4 floats
            if (!weights.empty()) {
                glGenBuffers(1, &vbo_weights);
                glBindBuffer(GL_ARRAY_BUFFER, vbo_weights);
                glBufferData(GL_ARRAY_BUFFER, weights.size() * sizeof(glm::vec4), weights.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 0, 0);
                glEnableVertexAttribArray(5);
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
            // Use provided base_name so multiple models don't clobber each other
            obj.name = base_name + "_" + std::to_string(mesh_count);
            obj.first_index = 0;
            obj.num_indices = indices.size();
            obj.rendering_mode = GL_TRIANGLES;
            obj.vertex_array_object_id = vao;

            // Use texture_id loaded from the GLTF material (if any).
            // Do not hardcode loading from the repository; fix GLTF image paths instead.
            obj.texture_id = texture_id;

            // Compute AABB
            if (!positions.empty()) {
                obj.aabb.min = positions[0];
                obj.aabb.max = positions[0];
                for (const auto& v : positions) {
                    obj.aabb.min = glm::min(obj.aabb.min, v);
                    obj.aabb.max = glm::max(obj.aabb.max, v);
                }
            }
            g_VirtualScene[obj.name] = obj;
            mesh_count++;
        }
    }
}

tinygltf::Model loadGltfModelAndBuildScene(const std::string &path, const std::string &base_name)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    bool is_glb = (path.length() >= 4 && path.substr(path.length() - 4) == ".glb");
    bool ret = false;
    if (is_glb) {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    } else {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    }

    if (!warn.empty())
        fprintf(stderr, "glTF warning: %s\n", warn.c_str());

    if (!err.empty())
        fprintf(stderr, "glTF error: %s\n", err.c_str());

    if (!ret) {
        std::string msg = "Failed to load glTF file '" + path + "'";
        if (!err.empty()) msg += std::string(": ") + err;
        if (!warn.empty()) msg += std::string("\nWarnings: ") + warn;
        throw std::runtime_error(msg);
    }

    // Compute normals if missing and build GPU resources
    computeNormalsForGLTF<uint16_t>(model);
    // If caller did not provide a base_name, derive one from the filename.
    std::string final_base = base_name;
    if (final_base.empty()) {
        size_t slash = path.find_last_of("/\\");
        std::string fname = (slash == std::string::npos) ? path : path.substr(slash + 1);
        size_t dot = fname.find_last_of('.');
        final_base = (dot == std::string::npos) ? fname : fname.substr(0, dot);
        for (auto &c : final_base) if (!isalnum((unsigned char)c)) c = '_';
    }
    buildTrianglesAndAddToVirtualSceneFromGLTF(model, final_base);

    return model;
}

std::vector<AABB> parseColliders(const std::string& filepath) {                                                                                                                        
    std::vector<AABB> colliders;                                                                                                                                                       
    std::ifstream file(filepath);                                                                                                                                                      
                                                                                                                                                                                        
    if (!file.is_open()) {                                                                                                                                                             
        std::cerr << "Erro ao abrir o arquivo de colisores: " << filepath << std::endl;                                                                                                
        return colliders;                                                                                                                                                              
    }                                                                                                                                                                                  
                                                                                                                                                                                        
    std::string line;                                                                                                                                                                  
    // Percorre cada linha do arquivo                                                                                                                                                  
    while (std::getline(file, line)) {                                                                                                                                                 
        if (line.empty()) continue; // Ignora linhas vazias                                                                                                                            
                                                                                                                                                                                        
        std::stringstream ss(line);                                                                                                                                                    
        float minX, minY, minZ, maxX, maxY, maxZ;                                                                                                                                      
                                                                                                                                                                                        
        // Extrai os 6 floats separados por espaço                                                                                                                                     
        if (ss >> minX >> minY >> minZ >> maxX >> maxY >> maxZ) {                                                                                                                      
            glm::vec3 min_point(minX, minY, minZ);                                                                                                                                     
            glm::vec3 max_point(maxX, maxY, maxZ);                                                                                                                                     
                                                                                                                                                                                        
            // Adiciona a bounding box na lista usando o construtor do seu AABB                                                                                                        
            colliders.push_back(AABB(min_point, max_point));                                                                                                                           
        }                                                                                                                                                                              
    }                                                                                                                                                                                  
                                                                                                                                                                                        
    file.close();                                                                                                                                                                      
    return colliders;                                                                                                                                                                  
} 