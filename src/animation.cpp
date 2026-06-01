#include "animation.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
// Need GLFW key definitions for the Swampfire state machine
#include <GLFW/glfw3.h>

#include "globals.h"

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

SwampfireAnimResult computeSwampfireAnimation(const tinygltf::Model& model,
                                             const bool keys[1024],
                                             bool jumping,
                                             float delta_t,
                                             float agora,
                                             SwampfireAnimState& state)
{
    SwampfireAnimResult res;

    auto key_down = [&](int k){ return k >= 0 && k < 1024 ? keys[k] : false; };
    bool is_moving = key_down(GLFW_KEY_W) || key_down(GLFW_KEY_A) ||
                     key_down(GLFW_KEY_S) || key_down(GLFW_KEY_D) ||
                     key_down(GLFW_KEY_UP) || key_down(GLFW_KEY_DOWN) ||
                     key_down(GLFW_KEY_LEFT) || key_down(GLFW_KEY_RIGHT);

    int current_anim_index = 6; // Idle by default
    bool is_attacking = false;
    float anim_time_to_pass = 0.0f;

    // 1/2. Attack handling (E and Q). Disabled while jumping.
    if (jumping) {
        // Cancel any ongoing Q hold when jumping
        state.q_state = 0;
        // If jumping, we skip attack handling and keep pending fireball (it should not fire while jumping)
    } else {
        // Attack with E
        if (player.active_character == 1 && key_down(GLFW_KEY_E)) {
            current_anim_index = 1;
            is_attacking = true;
        }
        // Attack with Q (hold = 3, release = 2) - support charging
        else {
            if (player.active_character == 1 && key_down(GLFW_KEY_Q)) {
                // start holding
                if (state.q_state != 1) {
                    state.q_state = 1;
                    state.q_press_time = agora;
                }
                current_anim_index = 3; // hold animation
                is_attacking = true;
            } else if (state.q_state == 1) {
                // release: transition to release animation and report spawn strength
                state.q_state = 2;
                state.q_release_time = agora;
                current_anim_index = 2; // release animation
                is_attacking = true;

                // compute hold duration and normalized strength
                const float max_hold = 1.5f; // seconds
                float hold_time = agora - state.q_press_time;
                float strength = hold_time > 0.0f ? std::min(1.0f, hold_time / max_hold) : 0.0f;
                // store pending strength; actual spawn will be emitted when release animation finishes
                state.pending_fireball_strength = strength;
            } else if (state.q_state == 2) {
                const float release_duration = 0.8f; // total release anim length
                const float lead_time = 0.5f; // spawn this many seconds before animation end
                float since_release = agora - state.q_release_time;

                if (since_release < release_duration) {
                    current_anim_index = 2;
                    is_attacking = true;

                    // If we're within the lead window and haven't emitted yet, emit now
                    if (since_release >= (release_duration - lead_time) && state.pending_fireball_strength > 0.0f) {
                        res.spawn_fireball_strength = state.pending_fireball_strength;
                        state.pending_fireball_strength = 0.0f;
                    }
                } else {
                    state.q_state = 0;
                    // If for some reason pending remained, emit now as fallback
                    if (state.pending_fireball_strength > 0.0f) {
                        res.spawn_fireball_strength = state.pending_fireball_strength;
                        state.pending_fireball_strength = 0.0f;
                    }
                }
            }
        }
    }

    // 3. Jump (animation 0) - jumping animation allowed even if not active
    if (!is_attacking && jumping) {
        current_anim_index = 0;
    }

    // 4. Run / Idle - only start these animations when Swampfire is active
    if (!is_attacking && !jumping) {
        if (player.active_character == 1) {
            if (is_moving) current_anim_index = 8;
            else current_anim_index = 6;
        } else {
            // Not active -> keep default_idle
            current_anim_index = 6;
        }
    }

    // Debug override: numeric keys 0-9
    for (int num = 0; num <= 9; ++num) {
        if (key_down(GLFW_KEY_0 + num)) { current_anim_index = num; break; }
    }

    // Clamp to available animations (use last defined animation if index too large)
    if (!model.animations.empty()) {
        current_anim_index = std::min(current_anim_index, (int)model.animations.size() - 1);
    }

    // Local time management (reset when animation changes)
    if (current_anim_index != state.last_applied_anim_index) {
        state.anim_start_time = agora;
        state.last_applied_anim_index = current_anim_index;
        if (current_anim_index == 0) state.jump_timer = 0.0f;
    }

    anim_time_to_pass = agora - state.anim_start_time;

    // Jump ping-pong handling
    if (current_anim_index == 0) {
        state.jump_timer += delta_t;
        float speed_multiplier = 2.0f;
        anim_time_to_pass = state.jump_timer * speed_multiplier;

        if (model.animations.size() > (size_t)current_anim_index) {
            const tinygltf::Animation &anim = model.animations[current_anim_index];
            auto get_max_time = [&](const tinygltf::Animation &a)->float{
                float max_t = 0.0f;
                for (const auto &samp : a.samplers) {
                    if (samp.input < 0) continue;
                    const tinygltf::Accessor &acc = model.accessors[samp.input];
                    const tinygltf::BufferView &bv = model.bufferViews[acc.bufferView];
                    const tinygltf::Buffer &buf = model.buffers[bv.buffer];
                    const float *times = reinterpret_cast<const float*>(&(buf.data[bv.byteOffset + acc.byteOffset]));
                    if (acc.count > 0) max_t = std::max(max_t, times[acc.count - 1]);
                }
                return max_t;
            };

            float max_time = get_max_time(anim);
            if (max_time > 0.0f && anim_time_to_pass > max_time) {
                float time_after_max = anim_time_to_pass - max_time;
                anim_time_to_pass = max_time - time_after_max;
                if (anim_time_to_pass < 0.0f) anim_time_to_pass = 0.0f;
            }
        }
    }

    res.current_anim_index = current_anim_index;
    res.anim_time_to_pass = anim_time_to_pass;
    res.is_attacking = is_attacking;
    return res;
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
