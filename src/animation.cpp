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
    if (player.is_dead) {
        if (jumping) {
            current_anim_index = 0; // jump
            anim_time_to_pass = 0.5f;
        } else {
            current_anim_index = 5; // death
            anim_time_to_pass = player.death_timer;
        }
        is_attacking = false;
        state.is_e_attacking = false;
        state.q_state = 0;
    } else if (player.is_flinching) {
        if (jumping) {
            current_anim_index = 0; // jump
            anim_time_to_pass = 0.5f;
        } else {
            current_anim_index = 4; // damage
            anim_time_to_pass = player.flinch_timer;
        }
        is_attacking = false;
        state.is_e_attacking = false;
        state.q_state = 0;
    } else {
        if (jumping) {
            // Cancel any ongoing holds when jumping
            state.q_state = 0;
            state.is_e_attacking = false;
            // If jumping, we skip attack handling and keep pending fireball
        } else {
        bool e_is_down = key_down(GLFW_KEY_E);
        bool e_just_pressed = e_is_down && !state.e_key_was_down;
        state.e_key_was_down = e_is_down;

        // Allow continuous punching if E is held down
        if (player.active_character == 1 && e_is_down && !state.is_e_attacking && state.q_state == 0) {
            state.is_e_attacking = true;
            state.e_attack_timer = 0.0f;
        }

        if (state.is_e_attacking) {
            current_anim_index = 1;
            is_attacking = true;
            state.e_attack_timer += delta_t;
            
            float max_attack_time = 0.6f; // reasonable fallback
            if (model.animations.size() > 1) {
                const tinygltf::Animation &anim = model.animations[1];
                float max_t = 0.0f;
                for (const auto &samp : anim.samplers) {
                    if (samp.input < 0) continue;
                    const tinygltf::Accessor &acc = model.accessors[samp.input];
                    const tinygltf::BufferView &bv = model.bufferViews[acc.bufferView];
                    const tinygltf::Buffer &buf = model.buffers[bv.buffer];
                    const float *times = reinterpret_cast<const float*>(&(buf.data[bv.byteOffset + acc.byteOffset]));
                    if (acc.count > 0) max_t = std::max(max_t, times[acc.count - 1]);
                }
                if (max_t > 0.0f) max_attack_time = max_t;
            }

            if (state.e_attack_timer >= max_attack_time) {
                state.is_e_attacking = false;
                state.last_applied_anim_index = -1; // Reset animation to cycle hitboxes
            }
        }
        // Attack with Q (hold = 3, release = 2) - support charging
        else {
            if (player.active_character == 1 && key_down(GLFW_KEY_Q) && player.special_energy >= 10.0f) {
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
                player.special_energy -= 10.0f; // Deduct energy on fireball release

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
        if (current_anim_index == 1) {
            state.punch1_hit_enemies.clear();
            state.punch2_hit_enemies.clear();
        }
    }

    if (!player.is_dead && !player.is_flinching) {
        anim_time_to_pass = agora - state.anim_start_time;
    }

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

    if (current_anim_index == 1) {
        float elapsed = anim_time_to_pass;
        // Punch 1 window — tune these by watching the animation
        if (elapsed >= 0.25f && elapsed <= 0.4f)
            res.punch1_active = true;
        // Punch 2 window
        if (elapsed >= 0.5f && elapsed <= 0.75f)
            res.punch2_active = true;
    }

    res.current_anim_index = current_anim_index;
    res.anim_time_to_pass = anim_time_to_pass;
    res.is_attacking = is_attacking;
    return res;
}

BenAnimResult computeBenAnimation(const tinygltf::Model& model,
                                 const bool keys[1024],
                                 bool jumping,
                                 float delta_t,
                                 float agora,
                                 BenAnimState& state)
{
    BenAnimResult res;
    auto key_down = [&](int k){ return k >= 0 && k < 1024 ? keys[k] : false; };
    bool is_moving = key_down(GLFW_KEY_W) || key_down(GLFW_KEY_A) ||
                     key_down(GLFW_KEY_S) || key_down(GLFW_KEY_D) ||
                     key_down(GLFW_KEY_UP) || key_down(GLFW_KEY_DOWN) ||
                     key_down(GLFW_KEY_LEFT) || key_down(GLFW_KEY_RIGHT);

    int current_anim_index = 7; // Idle_Loop default
    float anim_time_to_pass = 0.0f;

    // Handle Fighting Stance and Attack (E = 4)
    if (player.is_dead) {
        if (jumping) {
            current_anim_index = 9; // Jump_Loop
            anim_time_to_pass = 0.5f;
        } else {
            current_anim_index = 2; // Death01
            anim_time_to_pass = player.death_timer;
        }
        state.is_attacking = false;
        state.in_fighting_stance = false;
    } else if (player.is_flinching) {
        if (jumping) {
            current_anim_index = 9; // Jump_Loop
            anim_time_to_pass = 0.5f;
        } else {
            current_anim_index = 18; // Hit_Chest
            anim_time_to_pass = player.flinch_timer;
        }
        state.is_attacking = false;
    } else {
    bool e_is_down = key_down(GLFW_KEY_E);
    bool e_just_pressed = e_is_down && !state.e_key_was_down;
    state.e_key_was_down = e_is_down;

    if (!e_is_down) {
        state.in_fighting_stance = false;
        state.attack_speed_multiplier = 1.0f;
    }

    bool q_is_down = key_down(GLFW_KEY_Q);
    bool q_just_pressed = q_is_down && !state.q_key_was_down;
    state.q_key_was_down = q_is_down;

    // Allow continuous punching if E is held down
    if (player.active_character == 2 && e_is_down && !jumping && !state.is_attacking && !state.is_q_attacking) {
        state.is_attacking = true;
        state.attack_timer = 0.0f;
        state.in_fighting_stance = true;
        state.time_since_last_punch = 0.0f;
    }

    bool g_is_down = key_down(GLFW_KEY_G);
    state.is_dancing = g_is_down && !jumping && !state.is_attacking && !state.is_q_attacking;

    float big_slap_cost = player.max_special_energy * 0.10f;
    // Big Slap (Overhand Throw) on Q
    if (player.active_character == 2 && q_is_down && !jumping && !state.is_attacking && !state.is_q_attacking && player.special_energy >= big_slap_cost) {
        player.special_energy -= big_slap_cost;
        state.is_q_attacking = true;
        state.q_attack_timer = 0.0f;
        state.in_fighting_stance = true;
        printf("Big slap!\n"); // History log per the user's request
    }

    if (state.is_attacking) {
        current_anim_index = 4; // Fighting Left Jab
        state.attack_timer += delta_t * 1.5f; // Normal punch speed
        
        // Find attack animation duration
        float max_attack_time = 0.6f; // reasonable fallback
        if (model.animations.size() > 4) {
            const tinygltf::Animation &anim = model.animations[4];
            float max_t = 0.0f;
            for (const auto &samp : anim.samplers) {
                if (samp.input < 0) continue;
                const tinygltf::Accessor &acc = model.accessors[samp.input];
                const tinygltf::BufferView &bv = model.bufferViews[acc.bufferView];
                const tinygltf::Buffer &buf = model.buffers[bv.buffer];
                const float *times = reinterpret_cast<const float*>(&(buf.data[bv.byteOffset + acc.byteOffset]));
                if (acc.count > 0) max_t = std::max(max_t, times[acc.count - 1]);
            }
            if (max_t > 0.0f) max_attack_time = max_t;
        }

        if (state.attack_timer >= max_attack_time) {
            state.is_attacking = false;
        }
    } else if (state.is_q_attacking) {
        current_anim_index = 21; // OverhandThrow
        state.q_attack_timer += delta_t * 1.2f; // adjust speed if needed
        
        float max_attack_time = 1.0f;
        if (model.animations.size() > 21) {
            const tinygltf::Animation &anim = model.animations[21];
            float max_t = 0.0f;
            for (const auto &samp : anim.samplers) {
                if (samp.input < 0) continue;
                const tinygltf::Accessor &acc = model.accessors[samp.input];
                const tinygltf::BufferView &bv = model.bufferViews[acc.bufferView];
                const tinygltf::Buffer &buf = model.buffers[bv.buffer];
                const float *times = reinterpret_cast<const float*>(&(buf.data[bv.byteOffset + acc.byteOffset]));
                if (acc.count > 0) max_t = std::max(max_t, times[acc.count - 1]);
            }
            if (max_t > 0.0f) max_attack_time = max_t;
        }

        if (state.q_attack_timer >= max_attack_time) {
            state.is_q_attacking = false;
        }
    }

    if (!state.is_attacking && !state.is_q_attacking) {
        if (state.is_dancing) {
            current_anim_index = 1; // Dance_Loop
        } else if (jumping) {
            // Always use Jump_Loop (9) while in the air as requested
            current_anim_index = 9; 
        } else if (is_moving && player.active_character == 2) {
            current_anim_index = 16; // Sprint_Loop
        } else {
            current_anim_index = state.in_fighting_stance ? 3 : 7; // Fighting Idle (3) or Idle_Loop (7)
        }
    }
    }

    if (current_anim_index != state.last_applied_anim_index) {
        state.anim_start_time = agora;
        state.last_applied_anim_index = current_anim_index;
        if (jumping && current_anim_index == 9) state.jump_timer = 0.0f;
        if (current_anim_index == 4) state.punch_hit_enemies.clear();
        if (current_anim_index == 21) state.big_slap_hit_enemies.clear();
    }

    if (jumping && current_anim_index == 9) {
        state.jump_timer += delta_t;
    }

    if (!player.is_dead && !player.is_flinching) {
        anim_time_to_pass = agora - state.anim_start_time;
    }

    if (current_anim_index == 4) {
        anim_time_to_pass = state.attack_timer;
        float elapsed = anim_time_to_pass;
        // Tune these values by watching Ben's jab animation
        if (elapsed >= 0.25f && elapsed <= 0.45f)
            res.punch_active = true;
    } else if (current_anim_index == 21) {
        anim_time_to_pass = state.q_attack_timer;
        float elapsed = anim_time_to_pass;
        // Overhand Throw active window (tune this if needed, ~0.4 to 0.7 for an overhand)
        if (elapsed >= 0.4f && elapsed <= 0.7f)
            res.big_slap_active = true;
    }

    // Adjust speed for Sprint_Loop if needed, otherwise normal
    if (current_anim_index == 16) {
        anim_time_to_pass *= 1.5f; // Adjust run animation speed slightly if needed
    }

    res.current_anim_index = current_anim_index;
    res.anim_time_to_pass = anim_time_to_pass;
    res.is_attacking = state.is_attacking || state.is_q_attacking;
    res.is_dancing = state.is_dancing;
    
    return res;
}

void GltfAnimator::update(const tinygltf::Model& model, int anim_index, float current_time, bool loop) {
    if (model.skins.empty()) return;
    const tinygltf::Skin &skin = model.skins[0];

    // 1) Extrai T, R, S base originais para TODOS os ossos
    size_t num_nodes = model.nodes.size();
    if (node_T.size() != num_nodes) {
        node_T.resize(num_nodes);
        node_R.resize(num_nodes);
        node_S.resize(num_nodes);
        node_has_matrix.resize(num_nodes);
        local_matrix.resize(num_nodes);
        global_matrix.resize(num_nodes);
        matrix_computed.resize(num_nodes);
    }
    
    std::fill(node_T.begin(), node_T.end(), glm::vec3(0.0f));
    std::fill(node_R.begin(), node_R.end(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    std::fill(node_S.begin(), node_S.end(), glm::vec3(1.0f));
    std::fill(node_has_matrix.begin(), node_has_matrix.end(), false);
    std::fill(local_matrix.begin(), local_matrix.end(), glm::mat4(1.0f));

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
        
        if (cached_anim_index != anim_index) {
            sampler_inputs_cache.resize(anim.samplers.size());
            sampler_output_accessor_cache.assign(anim.samplers.size(), -1);

            for (size_t si = 0; si < anim.samplers.size(); ++si) {
                const auto &samp = anim.samplers[si];
                if (samp.input >= 0) {
                    const tinygltf::Accessor &acc = model.accessors[samp.input];
                    const tinygltf::BufferView &bv = model.bufferViews[acc.bufferView];
                    const tinygltf::Buffer &buf = model.buffers[bv.buffer];
                    const float *times = reinterpret_cast<const float*>(&buf.data[bv.byteOffset + acc.byteOffset]);
                    sampler_inputs_cache[si].assign(times, times + acc.count);
                }
                sampler_output_accessor_cache[si] = samp.output;
            }
            cached_anim_index = anim_index;
        }

        for (size_t si = 0; si < anim.samplers.size(); ++si) {
            if (!sampler_inputs_cache[si].empty()) {
                max_time = std::max(max_time, sampler_inputs_cache[si].back());
            }
        }
        if (max_time > 0.0f) {
            if (loop) {
                anim_time = fmod(current_time, max_time);
            } else {
                anim_time = std::min(current_time, max_time);
            }
        }

        for (const auto &ch : anim.channels) {
            int samp_idx = ch.sampler;
            if (samp_idx < 0 || samp_idx >= (int)anim.samplers.size()) continue;
            const auto &inputs = sampler_inputs_cache[samp_idx];
            if (inputs.empty()) continue;

            size_t k = 0; while (k + 1 < inputs.size() && anim_time > inputs[k+1]) ++k;
            size_t k1 = std::min(k + 1, inputs.size()-1);
            float t0 = inputs[k], t1 = inputs[k1];
            float local_t = (t1 - t0) > 0.0f ? (anim_time - t0) / (t1 - t0) : 0.0f;

            int outAccIdx = sampler_output_accessor_cache[samp_idx];
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
    std::fill(matrix_computed.begin(), matrix_computed.end(), false);

    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        if (matrix_computed[ni]) continue;

        path_cache.clear();
        int curr = ni;
        while (curr != -1 && !matrix_computed[curr]) {
            path_cache.push_back(curr);
            curr = node_parent[curr];
        }

        for (int i = (int)path_cache.size() - 1; i >= 0; --i) {
            int node = path_cache[i];
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

BigChillAnimResult computeBigChillCloakedAnimation(const tinygltf::Model& model,
                                 const bool keys[1024],
                                 bool jumping,
                                 float delta_t,
                                 float agora,
                                 BigChillAnimState& state) {
    BigChillAnimResult res;
    int current_anim_index = 8; // Idle_9
    float anim_time_to_pass = 0.0f;
    bool w_is_down = keys[GLFW_KEY_W];
    bool a_is_down = keys[GLFW_KEY_A];
    bool s_is_down = keys[GLFW_KEY_S];
    bool d_is_down = keys[GLFW_KEY_D];
    bool is_moving = w_is_down || a_is_down || s_is_down || d_is_down;
    bool e_is_down = keys[GLFW_KEY_E];
    bool q_is_down = keys[GLFW_KEY_Q];
    bool g_is_down = keys[GLFW_KEY_G];

    if (!e_is_down) state.e_key_was_down = false;
    if (!q_is_down) state.q_key_was_down = false;
    
    state.is_dancing = g_is_down && !jumping && !state.is_attacking && !state.is_q_attacking;

    if (player.is_dead) {
        current_anim_index = 3; // Dying Backwards_4
        anim_time_to_pass = player.death_timer;
        state.is_attacking = false;
        state.is_q_attacking = false;
    } else if (player.is_flinching) {
        current_anim_index = 2; // Rib Hit_3
        anim_time_to_pass = player.flinch_timer;
        state.is_attacking = false;
        state.is_q_attacking = false;
    } else if (jumping && !state.is_attacking && !state.is_q_attacking) {
        current_anim_index = 1; // Jump_2
        state.jump_timer += delta_t * 1.5f;
    } else {
        state.jump_timer = 0.0f;
        if (state.is_q_attacking) {
            current_anim_index = 0; // Special attack
            state.q_attack_timer += delta_t * 3.0f; // sped up
            float max_attack_time = 0.0f;
            if (current_anim_index < model.animations.size()) {
                const auto& anim = model.animations[current_anim_index];
                float max_t = 0.0f;
                for (const auto& samp : anim.samplers) {
                    const auto& inAcc = model.accessors[samp.input];
                    const auto& inBV = model.bufferViews[inAcc.bufferView];
                    const auto& inBuf = model.buffers[inBV.buffer];
                    const float* times = reinterpret_cast<const float*>(&inBuf.data[inBV.byteOffset + inAcc.byteOffset]);
                    if (inAcc.count > 0 && times[inAcc.count - 1] > max_t) max_t = times[inAcc.count - 1];
                }
                if (max_t > 0.0f) max_attack_time = max_t;
            }
            if (state.q_attack_timer >= max_attack_time) {
                state.is_q_attacking = false;
            }
        } else if (state.is_attacking) {
            current_anim_index = 5; // Punch Combo
            state.attack_timer += delta_t * 2.0f; // Sped up punch
            float max_attack_time = 0.0f;
            if (current_anim_index < model.animations.size()) {
                const auto& anim = model.animations[current_anim_index];
                float max_t = 0.0f;
                for (const auto& samp : anim.samplers) {
                    const auto& inAcc = model.accessors[samp.input];
                    const auto& inBV = model.bufferViews[inAcc.bufferView];
                    const auto& inBuf = model.buffers[inBV.buffer];
                    const float* times = reinterpret_cast<const float*>(&inBuf.data[inBV.byteOffset + inAcc.byteOffset]);
                    if (inAcc.count > 0 && times[inAcc.count - 1] > max_t) max_t = times[inAcc.count - 1];
                }
                if (max_t > 0.0f) max_attack_time = max_t;
            }
            if (state.attack_timer >= max_attack_time) {
                state.is_attacking = false;
                state.punch_hit_enemies.clear();
            }
        } else if (is_moving) {
            current_anim_index = 6; // Running_7
            state.in_fighting_stance = false;
        } else {
            if (state.is_dancing) {
                current_anim_index = 8; // Fallback to idle, big chill cloaked doesn't have dance
            } else if (state.in_fighting_stance) {
                current_anim_index = 7; // Fighting Stance
            } else {
                current_anim_index = 8; // Idle_9
            }
        }

        if (player.active_character == 0 && e_is_down && !jumping && !state.is_attacking && !state.is_q_attacking) {
            state.is_attacking = true;
            state.attack_timer = 0.0f;
            state.punch_segment = 0;
            state.e_key_was_down = true;
            state.in_fighting_stance = true;
            state.punch_hit_enemies.clear();
        }
        
        if (player.active_character == 0 && q_is_down && !jumping && !state.is_attacking && !state.is_q_attacking) {
            if (player.special_energy >= 10.0f) {
                state.is_q_attacking = true;
                state.q_attack_timer = 0.0f;
                state.in_fighting_stance = true;
                player.special_energy -= 10.0f; // Deduct start cost
            }
        }
    }

    if (current_anim_index != state.last_applied_anim_index && current_anim_index != 1 && current_anim_index != 5 && current_anim_index != 0) {
        state.anim_start_time = agora;
    }
    state.last_applied_anim_index = current_anim_index;

    if (current_anim_index != 1 && current_anim_index != 5 && current_anim_index != 0) {
        anim_time_to_pass = (agora - state.anim_start_time) * 1.5f;
    }

    if (current_anim_index == 1 && jumping) {
        anim_time_to_pass = state.jump_timer;
    }

    if (current_anim_index == 5) {
        anim_time_to_pass = state.attack_timer;
        if (anim_time_to_pass >= 0.15f && anim_time_to_pass < 0.25f) res.punch_active = true;
        if (anim_time_to_pass >= 0.30f && anim_time_to_pass < 0.40f) res.punch_active = true;
        if (anim_time_to_pass >= 0.45f && anim_time_to_pass < 0.55f) res.punch_active = true;
        if (anim_time_to_pass >= 0.60f && anim_time_to_pass < 0.70f) res.punch_active = true;

        if (anim_time_to_pass >= 0.15f && state.punch_segment == 0) { state.punch_segment = 1; state.punch_hit_enemies.clear(); res.punch_sound_trigger = true; }
        if (anim_time_to_pass >= 0.30f && state.punch_segment == 1) { state.punch_segment = 2; state.punch_hit_enemies.clear(); res.punch_sound_trigger = true; }
        if (anim_time_to_pass >= 0.45f && state.punch_segment == 2) { state.punch_segment = 3; state.punch_hit_enemies.clear(); res.punch_sound_trigger = true; }
        if (anim_time_to_pass >= 0.60f && state.punch_segment == 3) { state.punch_segment = 4; state.punch_hit_enemies.clear(); res.punch_sound_trigger = true; }
    }
    if (current_anim_index == 0) {
        float frame_48 = 48.0f / 24.0f;
        // Clampa primeiro! E só clampa se não tiver escapado (janela de 0.15s)
        if (q_is_down && state.q_attack_timer >= frame_48 && state.q_attack_timer <= frame_48 + 0.5f) {
            if (player.special_energy > 0.0f) {
                state.q_attack_timer = frame_48; // freeze
                player.special_energy -= 10.0f * delta_t; // Continuous drain
            }
        }
        
        anim_time_to_pass = state.q_attack_timer;
        if (anim_time_to_pass >= (43.0f / 24.0f) && anim_time_to_pass <= frame_48) {
            res.magic_active = true;
        }
    }

    if (current_anim_index == 6) {
        anim_time_to_pass *= 0.8f; 
    }

    res.current_anim_index = current_anim_index;
    res.anim_time_to_pass = anim_time_to_pass;
    res.is_attacking = state.is_attacking || state.is_q_attacking;
    res.is_dancing = state.is_dancing;
    return res;
}

BigChillAnimResult computeBigChillBen10Animation(const tinygltf::Model& model,
                                 const bool keys[1024],
                                 bool jumping,
                                 float delta_t,
                                 float agora,
                                 BigChillAnimState& state) {
    BigChillAnimResult res;
    int current_anim_index = 5; // Idle_Loop
    float anim_time_to_pass = 0.0f;
    bool w_is_down = keys[GLFW_KEY_W];
    bool a_is_down = keys[GLFW_KEY_A];
    bool s_is_down = keys[GLFW_KEY_S];
    bool d_is_down = keys[GLFW_KEY_D];
    bool is_moving = w_is_down || a_is_down || s_is_down || d_is_down;
    bool e_is_down = keys[GLFW_KEY_E];
    bool q_is_down = keys[GLFW_KEY_Q];
    bool g_is_down = keys[GLFW_KEY_G];

    if (!e_is_down) state.e_key_was_down = false;
    if (!q_is_down) state.q_key_was_down = false;
    
    state.is_dancing = g_is_down && !jumping && !state.is_attacking && !state.is_q_attacking;

    if (player.is_dead) {
        current_anim_index = 4; // Use Hit_Chest for now if no dead animation
        anim_time_to_pass = player.death_timer;
        state.is_attacking = false;
        state.is_q_attacking = false;
    } else if (player.is_flinching) {
        current_anim_index = 4; // Hit_Chest
        anim_time_to_pass = player.flinch_timer;
        state.is_attacking = false;
        state.is_q_attacking = false;
    } else if (jumping && !state.is_attacking && !state.is_q_attacking) {
        current_anim_index = 7; // Jump_Loop
        state.jump_timer += delta_t * 1.5f;
    } else {
        state.jump_timer = 0.0f;
        if (state.is_q_attacking) {
            current_anim_index = 9; // Levitate Entrance (Special attack)
            state.q_attack_timer += delta_t * 1.5f;
            float max_attack_time = 0.0f;
            if (current_anim_index < model.animations.size()) {
                const auto& anim = model.animations[current_anim_index];
                float max_t = 0.0f;
                for (const auto& samp : anim.samplers) {
                    const auto& inAcc = model.accessors[samp.input];
                    const auto& inBV = model.bufferViews[inAcc.bufferView];
                    const auto& inBuf = model.buffers[inBV.buffer];
                    const float* times = reinterpret_cast<const float*>(&inBuf.data[inBV.byteOffset + inAcc.byteOffset]);
                    if (inAcc.count > 0 && times[inAcc.count - 1] > max_t) max_t = times[inAcc.count - 1];
                }
                if (max_t > 0.0f) max_attack_time = max_t;
            }
            if (state.q_attack_timer >= max_attack_time) {
                state.is_q_attacking = false;
            }
        } else if (state.is_attacking) {
            current_anim_index = 2; // Fighting Left Jab
            state.attack_timer += delta_t * 1.5f;
            float max_attack_time = 0.0f;
            if (current_anim_index < model.animations.size()) {
                const auto& anim = model.animations[current_anim_index];
                float max_t = 0.0f;
                for (const auto& samp : anim.samplers) {
                    const auto& inAcc = model.accessors[samp.input];
                    const auto& inBV = model.bufferViews[inAcc.bufferView];
                    const auto& inBuf = model.buffers[inBV.buffer];
                    const float* times = reinterpret_cast<const float*>(&inBuf.data[inBV.byteOffset + inAcc.byteOffset]);
                    if (inAcc.count > 0 && times[inAcc.count - 1] > max_t) max_t = times[inAcc.count - 1];
                }
                if (max_t > 0.0f) max_attack_time = max_t;
            }
            if (state.attack_timer >= max_attack_time) {
                state.is_attacking = false;
                state.punch_hit_enemies.clear();
            }
        } else if (is_moving) {
            current_anim_index = 11; // Run Anime
            state.in_fighting_stance = false;
        } else {
            if (state.is_dancing) {
                current_anim_index = 0; // Dance Charleston
            } else if (state.in_fighting_stance) {
                current_anim_index = 1; // Fighting Idle
            } else {
                current_anim_index = 5; // Idle_Loop
            }
        }

        if (player.active_character == 0 && e_is_down && !jumping && !state.is_attacking && !state.is_q_attacking) {
            state.is_attacking = true;
            state.attack_timer = 0.0f;
            state.punch_segment = 0;
            state.e_key_was_down = true;
            state.in_fighting_stance = true;
            state.punch_hit_enemies.clear();
        }
        
        if (player.active_character == 0 && q_is_down && !jumping && !state.is_attacking && !state.is_q_attacking) {
            if (player.special_energy >= 10.0f) {
                state.is_q_attacking = true;
                state.q_attack_timer = 0.0f;
                state.in_fighting_stance = true;
                player.special_energy -= 10.0f; // Deduct start cost
            }
        }
    }

    if (current_anim_index != state.last_applied_anim_index && current_anim_index != 7 && current_anim_index != 2 && current_anim_index != 9 && current_anim_index != 0) {
        state.anim_start_time = agora;
    }
    state.last_applied_anim_index = current_anim_index;

    if (current_anim_index != 7 && current_anim_index != 2 && current_anim_index != 9 && current_anim_index != 0) {
        anim_time_to_pass = (agora - state.anim_start_time) * 1.5f;
    }

    if (current_anim_index == 0) {
        anim_time_to_pass = (agora - state.anim_start_time) * 1.2f;
    }

    if (current_anim_index == 7 && jumping) {
        anim_time_to_pass = state.jump_timer;
    }

    if (current_anim_index == 2) {
        anim_time_to_pass = state.attack_timer;
        if (anim_time_to_pass >= 0.15f && anim_time_to_pass < 0.25f) res.punch_active = true;
        if (anim_time_to_pass >= 0.30f && anim_time_to_pass < 0.40f) res.punch_active = true;
        if (anim_time_to_pass >= 0.45f && anim_time_to_pass < 0.55f) res.punch_active = true;
        if (anim_time_to_pass >= 0.60f && anim_time_to_pass < 0.70f) res.punch_active = true;

        if (anim_time_to_pass >= 0.15f && state.punch_segment == 0) { state.punch_segment = 1; state.punch_hit_enemies.clear(); res.punch_sound_trigger = true; }
        if (anim_time_to_pass >= 0.30f && state.punch_segment == 1) { state.punch_segment = 2; state.punch_hit_enemies.clear(); res.punch_sound_trigger = true; }
        if (anim_time_to_pass >= 0.45f && state.punch_segment == 2) { state.punch_segment = 3; state.punch_hit_enemies.clear(); res.punch_sound_trigger = true; }
        if (anim_time_to_pass >= 0.60f && state.punch_segment == 3) { state.punch_segment = 4; state.punch_hit_enemies.clear(); res.punch_sound_trigger = true; }
    }
    
    if (current_anim_index == 9) {
        float freeze_time = 1.0f; // Approximate freeze point for levitation magic
        if (q_is_down && state.q_attack_timer >= freeze_time && state.q_attack_timer <= freeze_time + 0.5f) {
            if (player.special_energy > 0.0f) {
                state.q_attack_timer = freeze_time; // freeze
                player.special_energy -= 10.0f * delta_t; // Continuous drain
            }
        }
        
        anim_time_to_pass = state.q_attack_timer;
        if (anim_time_to_pass >= freeze_time - 0.2f && anim_time_to_pass <= freeze_time) {
            res.magic_active = true;
        }
    }

    if (current_anim_index == 11) {
        anim_time_to_pass *= 1.2f; // Adjust run speed if needed
    }

    res.current_anim_index = current_anim_index;
    res.anim_time_to_pass = anim_time_to_pass;
    res.is_attacking = state.is_attacking || state.is_q_attacking;
    return res;
}
