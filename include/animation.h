#pragma once

#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

class GltfAnimator {
public:
    // Construtor: Prepara a hierarquia de nós e as matrizes de repouso (Inverse Bind Matrices)
    GltfAnimator(const tinygltf::Model& model);

    // Update: Calcula a interpolação do frame atual e atualiza as matrizes dos ossos
    void update(const tinygltf::Model& model, int anim_index, float current_time);

    // Retorna as matrizes prontas para a GPU
    const std::vector<glm::mat4>& getBoneMatrices() const { return boneMatrices; }

private:
    std::vector<int> node_parent;
    std::vector<glm::mat4> inverse_bind_matrices;
    std::vector<glm::mat4> boneMatrices;
};

// State object used to preserve local timers and flags for the Swampfire
// animation state machine between frames.
struct SwampfireAnimState {
    int q_state = 0;
    float q_release_time = 0.0f;
    float q_press_time = 0.0f;
    float jump_timer = 0.0f;
    int last_applied_anim_index = -1;
    float anim_start_time = 0.0f;
    // Pending fireball strength stored until release animation finishes
    float pending_fireball_strength = 0.0f;
};

// Result of the Swampfire animation computation for this frame.
struct SwampfireAnimResult {
    int current_anim_index = 0;
    float anim_time_to_pass = 0.0f;
    bool is_attacking = false;
    // If >0, spawn a fireball with this normalized strength (0..1)
    float spawn_fireball_strength = 0.0f;
};

// Compute the proper animation index and local animation time for the
// Swampfire character. This encapsulates the state machine previously
// implemented inline in main.cpp. It updates `state` and returns the
// animation index/time to apply.
SwampfireAnimResult computeSwampfireAnimation(const tinygltf::Model& model,
                                             const bool keys[1024],
                                             bool jumping,
                                             float delta_t,
                                             float agora,
                                             SwampfireAnimState& state);
