#pragma once

#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <set>

class GltfAnimator {
public:
    // Construtor: Prepara a hierarquia de nós e as matrizes de repouso (Inverse Bind Matrices)
    GltfAnimator(const tinygltf::Model& model);

    // Update: Calcula a interpolação do frame atual e atualiza as matrizes dos ossos
    void update(const tinygltf::Model& model, int anim_index, float current_time, bool loop = true);

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
    std::set<int> punch1_hit_enemies;
    std::set<int> punch2_hit_enemies;
  
    bool is_e_attacking = false;
    float e_attack_timer = 0.0f;
    bool e_key_was_down = false;

};

// Result of the Swampfire animation computation for this frame.
struct SwampfireAnimResult {
    int current_anim_index = 0;
    float anim_time_to_pass = 0.0f;
    bool is_attacking = false;
    // If >0, spawn a fireball with this normalized strength (0..1)
    float spawn_fireball_strength = 0.0f;
    bool punch1_active = false;
    bool punch2_active = false;
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

// Ben Tennyson animation state and result
struct BenAnimState {
    int last_applied_anim_index = -1;
    float anim_start_time = 0.0f;
    float jump_timer = 0.0f;
    float attack_timer = 0.0f;
    bool is_attacking = false;
    bool in_fighting_stance = false;
    float time_since_last_punch = 0.0f;
    bool e_key_was_down = false;
    std::set<int> punch_hit_enemies;  // same as swampfire's punch1_hit_enemies
    float attack_speed_multiplier = 1.0f;
};

struct BenAnimResult {
    int current_anim_index = 7;
    float anim_time_to_pass = 0.0f;
    bool is_attacking = false;
    bool punch_active = false;
};

BenAnimResult computeBenAnimation(const tinygltf::Model& model,
                                 const bool keys[1024],
                                 bool jumping,
                                 float delta_t,
                                 float agora,
                                 BenAnimState& state);

// Big Chill animation state and result
struct BigChillAnimState {
    int last_applied_anim_index = -1;
    float anim_start_time = 0.0f;
    float jump_timer = 0.0f;
    float attack_timer = 0.0f;
    bool is_attacking = false;
    bool in_fighting_stance = false;
    bool e_key_was_down = false;
    bool q_key_was_down = false;
    bool is_q_attacking = false;
    float q_attack_timer = 0.0f;
    std::set<int> punch_hit_enemies;
    std::set<int> magic_hit_enemies;
};

struct BigChillAnimResult {
    int current_anim_index = 8; // Idle_9
    float anim_time_to_pass = 0.0f;
    bool is_attacking = false;
    bool punch_active = false;
    bool magic_active = false;
};

BigChillAnimResult computeBigChillAnimation(const tinygltf::Model& model,
                                 const bool keys[1024],
                                 bool jumping,
                                 float delta_t,
                                 float agora,
                                 BigChillAnimState& state);
