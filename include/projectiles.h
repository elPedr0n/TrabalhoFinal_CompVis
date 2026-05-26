#pragma once

#include <glm/glm.hpp>
#include <tiny_gltf.h>
#include <map>
#include <string>
#include <glad/glad.h>

#include "sceneobject.h"
#include "animation.h"

// Spawn a charged projectile for a given model base name (e.g. "the_fireball").
// strength: 0..1 (normalized charge)
void Projectiles_Spawn(const std::string &modelBaseName, float strength, const glm::vec3 &player_pos, float player_rotate);

// Update projectile physics (delta seconds)
void Projectiles_Update(float delta_t);

// Draw active projectiles for the specified model base name. Pass renderer state and scene map.
// objectIdValue is the integer id used by the fragment shader to select rendering behavior.
void Projectiles_Draw(const tinygltf::Model &model,
                      GltfAnimator &animator,
                      GLuint gpuProgramID,
                      GLint modelUniform,
                      GLint boneMatricesUniform,
                      GLint objectIdUniform,
                      const std::map<std::string, SceneObject> &scene,
                      const std::string &modelBaseName,
                      float player_scale,
                      int objectIdValue);

// Backwards-compatible wrappers (old signatures) kept for convenience
void Projectiles_Spawn(float strength, const glm::vec3 &player_pos, float player_rotate);
void Projectiles_Draw(const tinygltf::Model &model,
                      GltfAnimator &animator,
                      GLuint gpuProgramID,
                      GLint modelUniform,
                      GLint boneMatricesUniform,
                      GLint objectIdUniform,
                      const std::map<std::string, SceneObject> &scene,
                      float player_scale,
                      int objectIdValue);
