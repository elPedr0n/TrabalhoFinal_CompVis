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
