#include "projectiles.h"
#include "particles.h"
#include "globals.h"
#include "globals.h"
#include "breakables.h"

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
        
        bool is_bezier=false;
        bool is_enemy=false;
        glm::vec3 p0, p1, p2, p3;
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
    p.pos = player_pos + forward * 0.5f + glm::vec3(0.0f, 0.5f, 0.0f);
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

void Projectiles_SpawnBezier(const std::string &modelBaseName, const glm::vec3 &start_pos, const glm::vec3 &target_pos, bool is_enemy)
{
    Projectile p;
    p.model_name = modelBaseName;
    p.pos = start_pos;
    p.vel = glm::vec3(0.0f); // Not used for Bezier
    p.age = 0.0f;
    p.life = 1.5f; // time to reach target
    p.scale = 0.5f;
    p.bbox = MakeAABBFromCenterSize(p.pos, glm::vec3(p.scale));
    p.active = true;
    p.is_bezier = true;
    p.is_enemy = is_enemy;
    p.p0 = start_pos;
    p.p3 = target_pos;
    
    // Parabola effect: control points are higher up
    glm::vec3 mid = (start_pos + target_pos) * 0.5f;
    float dist = glm::distance(start_pos, target_pos);
    float height = std::max(2.0f, dist * 0.3f);
    
    p.p1 = p.p0 + (mid - p.p0) * 0.5f;
    p.p1.y += height;
    p.p2 = mid + (target_pos - mid) * 0.5f;
    p.p2.y += height;
    
    s_projectiles.push_back(p);
}

void Projectiles_Update(float delta_t)
{
    for (auto &p : s_projectiles) {
        if (!p.active) continue;

        if (p.is_bezier) {
            float t = p.age / p.life;
            if (t > 1.0f) t = 1.0f;
            
            float u = 1.0f - t;
            float tt = t * t;
            float uu = u * u;
            float uuu = uu * u;
            float ttt = tt * t;

            glm::vec3 next_pos = uuu * p.p0; // (1-t)^3 * P0
            next_pos += 3 * uu * t * p.p1;   // 3 * (1-t)^2 * t * P1
            next_pos += 3 * u * tt * p.p2;   // 3 * (1-t) * t^2 * P2
            next_pos += ttt * p.p3;          // t^3 * P3
            
            // Derive velocity for rotation (approximate)
            p.vel = next_pos - p.pos;
            if (p.vel != glm::vec3(0.0f)) {
                p.vel = glm::normalize(p.vel) * 2.0f; // Give it some length for rotation atan2
            }
            
            p.pos = next_pos;
            p.bbox = MakeAABBFromCenterSize(p.pos, glm::vec3(p.scale));
            
            // Check collision
            bool exploded = false;
            // Floor collision
            if (p.pos.y <= 0.1f) {
                p.active = false;
                exploded = true;
            }
            // Skip map collision for bezier or implement simple point collision
        } else {
            float move_x = p.vel.x * delta_t;
            float move_z = p.vel.z * delta_t;
            float orig_move_x = move_x;
            float orig_move_z = move_z;

        for (int i = 0; i < MAX_PLATFORMS; i++) {
            // Ignorar colisões horizontais com o chão (onde max.y costuma ser <= 0.1f)
            if (map[i].bbox.max.y <= 0.1f) continue;

            move_x = p.bbox.GetClipX(map[i].bbox, move_x);
            move_z = p.bbox.GetClipZ(map[i].bbox, move_z);
        }

            // Explode if it hits a wall horizontally
            if (move_x != orig_move_x || move_z != orig_move_z) {
                p.active = false;
            }

            p.pos.x += move_x;
            p.pos.y += p.vel.y * delta_t;
            p.pos.z += move_z;
            p.bbox = MakeAABBFromCenterSize(p.pos, glm::vec3(p.scale));
        }

        bool exploded = !p.active; // True if it hit a wall or floor

        // Hit enemies or player
        if (p.active) {
            if (p.is_enemy) {
                if (p.bbox.Intersects(player.characters[player.active_character].bbox)) {
                    p.active = false;
                    exploded = true;
                }
            } else {
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!g_enemies[i].visible) continue;
                    if (g_enemies[i].is_dead) continue;
                    if (p.bbox.Intersects(g_enemies[i].bbox)) {
                        p.active = false;
                        exploded = true;
                        break;
                    }
                }
            }
            if (p.active) {
                for (int i = 0; i < MAX_BREAKABLES; i++) {
                    if (!g_breakables[i].active) continue;
                    if (p.bbox.Intersects(g_breakables[i].bbox)) {
                        p.active = false;
                        exploded = true;
                        break;
                    }
                }
            }
        }

        if (exploded) {
            float damage = 20.0f + (p.scale - 0.4f) * 25.0f;
            float splash_radius = 2.0f + p.scale;
            
            if (p.is_enemy) {
                float dist = glm::distance(p.pos, player.position);
                if (dist <= splash_radius) {
                    ApplyDamageToPlayer(damage, p.pos);
                }
            } else {
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!g_enemies[i].visible || g_enemies[i].is_dead) continue;
                    
                    float dist = glm::distance(p.pos, g_enemies[i].position);
                    if (dist <= splash_radius) {
                        ApplyDamageToEnemy(i, damage);
                    }
                }
            }
            
            for (int i = 0; i < MAX_BREAKABLES; i++) {
                if (!g_breakables[i].active) continue;
                
                float dist = glm::distance(p.pos, g_breakables[i].position);
                if (dist <= splash_radius) {
                    ApplyDamageToBreakable(i, damage);
                }
            }
            
            ParticleOptions explode_opts;
            explode_opts.color = p.is_enemy ? HexToRgb("#ff0000") : HexToRgb("#ff8800");
            explode_opts.life = 0.4f;
            explode_opts.scale = 0.05f * p.scale;
            explode_opts.speed = 3.0f * p.scale;
            explode_opts.count = 40 * (int)std::max(1.0f, p.scale);
            Particles_Spawn(p.pos, explode_opts);
        }

        p.age += delta_t;
        if (p.age >= p.life) p.active = false;
        // Emit a stronger particle trail from projectile position (via options)
        ParticleOptions popts;
        popts.color = p.is_enemy ? HexToRgb("#ff0000") : HexToRgb("#ff3c00");
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
                    glUniform1i(glGetUniformLocation(gpuProgramID, "UseOverrideKd"), 0);
                } else {
                    glUniform1i(glGetUniformLocation(gpuProgramID, "UseOverrideKd"), 0);
                }
                
                int draw_id = p.is_enemy ? 61 : objectIdValue;
                glUniform1i(objectIdUniform, draw_id);
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
                glUniform1i(glGetUniformLocation(gpuProgramID, "UseOverrideKd"), 0);
            } else {
                glUniform1i(glGetUniformLocation(gpuProgramID, "UseOverrideKd"), 0);
            }

            int draw_id = p.is_enemy ? 61 : objectIdValue;
            glUniform1i(objectIdUniform, draw_id);
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
