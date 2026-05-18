#include "animation.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

GltfAnimator::GltfAnimator(const tinygltf::Model& model) {
    // 1. Extrair hierarquia de pais (node_parent)
    node_parent.assign(model.nodes.size(), -1);
    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        const tinygltf::Node &n = model.nodes[ni];
        for (int c : n.children) {
            if (c >= 0 && c < (int)model.nodes.size()) {
                node_parent[c] = (int)ni;
            }
        }
    }

    // 2. Extrair Inverse Bind Matrices da Skin 0 (assumindo 1 skin principal)
    if (!model.skins.empty()) {
        const tinygltf::Skin &skin = model.skins[0];
        if (skin.inverseBindMatrices >= 0) {
            const tinygltf::Accessor &acc = model.accessors[skin.inverseBindMatrices];
            const tinygltf::BufferView &bv = model.bufferViews[acc.bufferView];
            const tinygltf::Buffer &buf = model.buffers[bv.buffer];
            const float *mats = reinterpret_cast<const float*>(&buf.data[bv.byteOffset + acc.byteOffset]);

            inverse_bind_matrices.reserve(acc.count);
            for (size_t i = 0; i < acc.count; ++i) {
                inverse_bind_matrices.push_back(glm::make_mat4(&mats[16 * i]));
            }
        }
    }
}

void GltfAnimator::update(const tinygltf::Model& model, int anim_index, float current_time) {
    if (model.skins.empty()) return;
    const tinygltf::Skin &skin = model.skins[0];

    // 1) Extrai T, R, S base originais para TODOS os ossos
    std::vector<glm::vec3> node_T(model.nodes.size(), glm::vec3(0.0f));
    std::vector<glm::quat> node_R(model.nodes.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> node_S(model.nodes.size(), glm::vec3(1.0f));
    std::vector<bool> node_has_matrix(model.nodes.size(), false);
    std::vector<glm::mat4> local_matrix(model.nodes.size(), glm::mat4(1.0f));

    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        const tinygltf::Node &n = model.nodes[ni];
        if (n.matrix.size() == 16) {
            node_has_matrix[ni] = true;
            local_matrix[ni] = glm::make_mat4(n.matrix.data());
        } else {
            if (n.translation.size() == 3) node_T[ni] = glm::vec3(n.translation[0], n.translation[1], n.translation[2]);
            if (n.rotation.size() == 4) node_R[ni] = glm::quat(n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2]);
            if (n.scale.size() == 3) node_S[ni] = glm::vec3(n.scale[0], n.scale[1], n.scale[2]);
        }
    }

    // 2) Aplica a animação
    if (!model.animations.empty() && anim_index >= 0 && anim_index < (int)model.animations.size()) {
        float anim_time = current_time;
        const tinygltf::Animation &anim = model.animations[anim_index];

        float max_time = 0.0f;
        std::vector<std::vector<float>> sampler_inputs(anim.samplers.size());
        std::vector<int> sampler_output_accessor(anim.samplers.size(), -1);

        for (size_t si = 0; si < anim.samplers.size(); ++si) {
            const auto &samp = anim.samplers[si];
            if (samp.input >= 0) {
                const tinygltf::Accessor &acc = model.accessors[samp.input];
                const tinygltf::BufferView &bv = model.bufferViews[acc.bufferView];
                const tinygltf::Buffer &buf = model.buffers[bv.buffer];
                const float *times = reinterpret_cast<const float*>(&buf.data[bv.byteOffset + acc.byteOffset]);
                sampler_inputs[si].assign(times, times + acc.count);
                if (!sampler_inputs[si].empty()) max_time = std::max(max_time, sampler_inputs[si].back());
            }
            sampler_output_accessor[si] = samp.output;
        }
        if (max_time > 0.0f) anim_time = fmod(current_time, max_time);

        for (const auto &ch : anim.channels) {
            int samp_idx = ch.sampler;
            if (samp_idx < 0 || samp_idx >= (int)anim.samplers.size()) continue;
            const auto &inputs = sampler_inputs[samp_idx];
            if (inputs.empty()) continue;

            size_t k = 0; while (k + 1 < inputs.size() && anim_time > inputs[k+1]) ++k;
            size_t k1 = std::min(k + 1, inputs.size()-1);
            float t0 = inputs[k], t1 = inputs[k1];
            float local_t = (t1 - t0) > 0.0f ? (anim_time - t0) / (t1 - t0) : 0.0f;

            int outAccIdx = sampler_output_accessor[samp_idx];
            if (outAccIdx < 0) continue;
            const tinygltf::Accessor &outAcc = model.accessors[outAccIdx];
            const tinygltf::BufferView &outBV = model.bufferViews[outAcc.bufferView];
            const tinygltf::Buffer &outBuf = model.buffers[outBV.buffer];
            const float *outData = reinterpret_cast<const float*>(&outBuf.data[outBV.byteOffset + outAcc.byteOffset]);

            size_t compCount = (outAcc.type == TINYGLTF_TYPE_VEC3) ? 3 : (outAcc.type == TINYGLTF_TYPE_VEC4 ? 4 : 1);
            const float *v0 = &outData[k * compCount];
            const float *v1 = &outData[k1 * compCount];

            int nodeIdx = ch.target_node;
            if (nodeIdx < 0 || nodeIdx >= (int)model.nodes.size()) continue;

            if (ch.target_path == "translation") {
                glm::vec3 t0v(0.0f), t1v(0.0f);
                for (size_t c = 0; c < compCount && c < 3; ++c) { t0v[c] = v0[c]; t1v[c] = v1[c]; }
                node_T[nodeIdx] = glm::mix(t0v, t1v, local_t);
                node_has_matrix[nodeIdx] = false;
            } else if (ch.target_path == "scale") {
                glm::vec3 s0v(1.0f), s1v(1.0f);
                for (size_t c = 0; c < compCount && c < 3; ++c) { s0v[c] = v0[c]; s1v[c] = v1[c]; }
                node_S[nodeIdx] = glm::mix(s0v, s1v, local_t);
                node_has_matrix[nodeIdx] = false;
            } else if (ch.target_path == "rotation") {
                glm::quat q0(1.0f,0.0f,0.0f,0.0f), q1(1.0f,0.0f,0.0f,0.0f);
                if (compCount >= 4) { q0 = glm::quat(v0[3], v0[0], v0[1], v0[2]); q1 = glm::quat(v1[3], v1[0], v1[1], v1[2]); }
                node_R[nodeIdx] = glm::normalize(glm::slerp(q0, q1, local_t));
                node_has_matrix[nodeIdx] = false;
            }
        }
    }

    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        if (!node_has_matrix[ni]) {
            local_matrix[ni] = glm::translate(glm::mat4(1.0f), node_T[ni]) 
                             * glm::mat4_cast(node_R[ni]) 
                             * glm::scale(glm::mat4(1.0f), node_S[ni]);
        }
    }

    // 3) Ordem Topológica e Matrizes Globais
    std::vector<glm::mat4> global_matrix(model.nodes.size(), glm::mat4(1.0f));
    std::vector<bool> matrix_computed(model.nodes.size(), false);

    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        if (matrix_computed[ni]) continue;

        std::vector<int> path;
        int curr = ni;
        while (curr != -1 && !matrix_computed[curr]) {
            path.push_back(curr);
            curr = node_parent[curr];
        }

        for (int i = (int)path.size() - 1; i >= 0; --i) {
            int node = path[i];
            int p = node_parent[node];
            if (p == -1) {
                global_matrix[node] = local_matrix[node];
            } else {
                global_matrix[node] = global_matrix[p] * local_matrix[node];
            }
            matrix_computed[node] = true;
        }
    }

    // 4) Matrizes finais (Bone Matrices)
    size_t jointCount = skin.joints.size();
    size_t uploadCount = std::min<size_t>(jointCount, 100);
    boneMatrices.assign(uploadCount, glm::mat4(1.0f));
    
    for (size_t j = 0; j < uploadCount; ++j) {
        int nodeIdx = skin.joints[j];
        glm::mat4 invBind(1.0f);
        if (j < inverse_bind_matrices.size()) invBind = inverse_bind_matrices[j];
        boneMatrices[j] = global_matrix[nodeIdx] * invBind;
    }
}
