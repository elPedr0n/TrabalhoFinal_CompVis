#include "projectiles.h"
#include "particles.h"
#include "globals.h"

#include <vector>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "matrices.h"

extern void DrawBoundingBox(AABB& aabb, int restore_object_id);

namespace {
    struct Projectile { 
        std::string model_name;
        glm::vec3 pos; 
        glm::vec3 vel; 
        float age=0.0f; 
        float life=3.0f; 
        float scale=1.0f;
        bool active=true; 
        AABB bbox;
    };
    std::vector<Projectile> s_projectiles;

    constexpr float PROJECTILE_BASE_SPEED = 6.0f;
    constexpr float PROJECTILE_BASE_LIFE = 3.0f;
}

void Projectiles_Spawn(const std::string &modelBaseName, float strength, const glm::vec3 &player_pos, float player_rotate)
{
    Projectile p;
    glm::vec3 forward = glm::vec3(sin(player_rotate), 0.0f, cos(player_rotate));
    p.model_name = modelBaseName;
    p.pos = player_pos + forward * 1.0f + glm::vec3(0.0f, 0.25f, 0.0f);
    p.vel = forward * PROJECTILE_BASE_SPEED * (1.0f + 0.8f * strength);
    p.age = 0.0f;
    p.life = PROJECTILE_BASE_LIFE * (1.0f + 0.5f * strength);
    p.scale = 0.4f + 1.2f * strength;
    p.bbox = MakeAABBFromCenterSize(p.pos, glm::vec3(p.scale));
    p.active = true;
    s_projectiles.push_back(p);
}

// Backwards-compatible spawn (assumes the_fireball)
void Projectiles_Spawn(float strength, const glm::vec3 &player_pos, float player_rotate)
{
    Projectiles_Spawn(std::string("the_fireball"), strength, player_pos, player_rotate);
}

void Projectiles_Update(float delta_t)
{
    for (auto &p : s_projectiles) {
        if (!p.active) continue;
        p.pos += p.vel * delta_t;
        p.bbox = MakeAABBFromCenterSize(p.pos, glm::vec3(p.scale));

        // Hit map platforms
        for (int i = 1; i < MAX_PLATFORMS; i++) {
            if (p.bbox.Intersects(map[i].bbox)) {
                printf("Projectile hit platform %d\n", i);
                p.active = false;
                break;
            }
        }
        // Hit enemies
        if (p.active) {
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!g_enemies[i].visible) continue;
                if (g_enemies[i].is_dead) continue;
                if (p.bbox.Intersects(g_enemies[i].bbox)) {
                    // Apply damage based on scale
                    float damage = 20.0f + (p.scale - 0.4f) * 25.0f;
                    ApplyDamageToEnemy(i, damage);
                    p.active = false;
                    break;
                }
            }
        }

        p.age += delta_t;
        if (p.age >= p.life) p.active = false;
        // Emit a stronger particle trail from projectile position (via options)
        ParticleOptions popts;
        popts.color = HexToRgb("#ff3c00");
        popts.life = 0.1f + 0.15f * (p.scale * 3.0f);
        popts.scale = 0.01f + 0.01f * (p.scale * 3.0f);
        popts.speed = 0.5f + 0.2f * (p.scale * 3.0f);
        popts.count = std::max(1, (int)std::round(8.0f * (p.scale * 3.0f)));
        Particles_Spawn(p.pos - glm::normalize(p.vel) * 0.1f, popts);
    }
    s_projectiles.erase(std::remove_if(s_projectiles.begin(), s_projectiles.end(), [](const Projectile &f){ return !f.active; }), s_projectiles.end());
}

void Projectiles_Draw(const tinygltf::Model &model,
                      GltfAnimator &animator,
                      GLuint gpuProgramID,
                      GLint modelUniform,
                      GLint boneMatricesUniform,
                      GLint objectIdUniform,
                      const std::map<std::string, SceneObject> &scene,
                      const std::string &modelBaseName,
                      float player_scale,
                      int objectIdValue)
{
    for (const auto &p : s_projectiles) {
        if (p.model_name != modelBaseName) continue;

        // If a GLTF model with meshes is provided, update animator and draw its meshes.
        if (!model.meshes.empty()) {
            animator.update(model, 0, p.age);
            const auto &bones = animator.getBoneMatrices();
            if (boneMatricesUniform >= 0 && !bones.empty()) {
                glUniformMatrix4fv(boneMatricesUniform, (GLsizei)bones.size(), GL_FALSE, (const GLfloat*)bones.data());
            }

            for (size_t mi = 0; mi < model.meshes.size(); ++mi) {
                std::string name = modelBaseName + "_" + std::to_string(mi);
                auto it = scene.find(name);
                if (it == scene.end()) continue;

                float rot = atan2(p.vel.x, p.vel.z);
                glm::mat4 modelMat = Matrix_Translate(p.pos.x, p.pos.y, p.pos.z)
                                * Matrix_Scale(player_scale * p.scale, player_scale * p.scale, player_scale * p.scale)
                                * Matrix_Rotate_Y(rot);
                glUniformMatrix4fv(modelUniform, 1, GL_FALSE, glm::value_ptr(modelMat));

                if (it->second.texture_id != 0) {
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, it->second.texture_id);
                    glUniform1i(glGetUniformLocation(gpuProgramID, "TextureImage4"), 4);
                }

                glUniform1i(objectIdUniform, objectIdValue);
                glBindVertexArray(it->second.vertex_array_object_id);
                glDrawElements(it->second.rendering_mode, it->second.num_indices, GL_UNSIGNED_INT, (void*)(it->second.first_index * sizeof(GLuint)));
                glBindVertexArray(0);
                AABB bbox_copy = p.bbox;
                DrawBoundingBox(bbox_copy, objectIdValue);
            }
        }
        else {
            // No GLTF model provided: fall back to drawing a static mesh from the virtual scene.
            auto it = scene.find(modelBaseName);
            if (it == scene.end()) continue;

            float rot = atan2(p.vel.x, p.vel.z);
            glm::mat4 modelMat = Matrix_Translate(p.pos.x, p.pos.y, p.pos.z)
                            * Matrix_Scale(player_scale * p.scale, player_scale * p.scale, player_scale * p.scale)
                            * Matrix_Rotate_Y(rot);
            glUniformMatrix4fv(modelUniform, 1, GL_FALSE, glm::value_ptr(modelMat));

            if (it->second.texture_id != 0) {
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, it->second.texture_id);
                glUniform1i(glGetUniformLocation(gpuProgramID, "TextureImage4"), 4);
            }

            glUniform1i(objectIdUniform, objectIdValue);
            glBindVertexArray(it->second.vertex_array_object_id);
            glDrawElements(it->second.rendering_mode, it->second.num_indices, GL_UNSIGNED_INT, (void*)(it->second.first_index * sizeof(GLuint)));
            glBindVertexArray(0);
            AABB bbox_copy = p.bbox;
            DrawBoundingBox(bbox_copy, objectIdValue);
        }
    }
}

// Backwards-compatible draw (assumes the_fireball)
void Projectiles_Draw(const tinygltf::Model &model,
                      GltfAnimator &animator,
                      GLuint gpuProgramID,
                      GLint modelUniform,
                      GLint boneMatricesUniform,
                      GLint objectIdUniform,
                      const std::map<std::string, SceneObject> &scene,
                      float player_scale,
                      int objectIdValue)
{
    Projectiles_Draw(model, animator, gpuProgramID, modelUniform, boneMatricesUniform, objectIdUniform, scene, std::string("the_fireball"), player_scale, objectIdValue);
}
