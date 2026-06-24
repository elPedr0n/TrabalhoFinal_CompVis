#include "breakables.h"
#include "fragments.h"
#include "sceneobject.h"
#include "matrices.h"
#include "particles.h"
#include "sound.h"
#include <iostream>

Breakable g_breakables[MAX_BREAKABLES];

void SpawnBreakable(glm::vec3 pos, float scale, const std::string& model_name, float health, float bbox_w, float bbox_h, float bbox_d, glm::vec3 frag_color, float frag_size, int frag_count, float rot_y) {
    for (int i = 0; i < MAX_BREAKABLES; ++i) {
        if (!g_breakables[i].active) {
            g_breakables[i].active = true;
            g_breakables[i].position = pos;
            g_breakables[i].scale = scale;
            g_breakables[i].rotation_x = 0.0f;
            g_breakables[i].rotation_y = rot_y;
            g_breakables[i].rotation_z = 0.0f;
            if (model_name == "the_teddy_bear") {
                g_breakables[i].rotation_x = -3.14159265359f / 2.0f;
            } else if (model_name == "the_park_bench") {
                float bench_rot_y = rot_y - (3.14159265359f / 2.0f);
                float snapped_rot_y = round(bench_rot_y / (3.14159265359f / 2.0f)) * (3.14159265359f / 2.0f);
                g_breakables[i].rotation_y = snapped_rot_y;
                
                int octant = (int)round(snapped_rot_y / (3.14159265359f / 2.0f));
                if (octant % 2 != 0) { // 90 or 270 degrees
                    float temp = bbox_w;
                    bbox_w = bbox_d;
                    bbox_d = temp;
                }
            }
            g_breakables[i].health = health;
            g_breakables[i].max_health = health;
            g_breakables[i].model_name = model_name;
            g_breakables[i].fragment_color = frag_color;
            g_breakables[i].fragment_size = frag_size;
            g_breakables[i].fragment_count = frag_count;
            g_breakables[i].bbox_w = bbox_w;
            g_breakables[i].bbox_h = bbox_h;
            g_breakables[i].bbox_d = bbox_d;
            
            g_breakables[i].bbox = makeAABBFromGround(pos, glm::vec3(bbox_w * scale, bbox_h * scale, bbox_d * scale));
            g_breakables[i].is_flinching = false;
            g_breakables[i].flinch_timer = 0.0f;
            g_breakables[i].velocity_y = 0.0f;
            
            std::cout << "Spawned " << model_name << " at (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
            
            break;
        }
    }
}

void ApplyDamageToBreakable(int id, float damage) {
    if (id < 0 || id >= MAX_BREAKABLES || !g_breakables[id].active) return;
    
    g_breakables[id].health -= damage;
    g_breakables[id].is_flinching = true;
    g_breakables[id].flinch_timer = 0.0f;
    
    if (g_breakables[id].health <= 0.0f) {
        if (g_breakables[id].model_name == "the_teddy_bear") {
            PlaySoundEffect("../../data/sounds/break_soft.wav");
        } else {
            PlaySoundEffect("../../data/sounds/break_hard.wav");
        }
        g_breakables[id].active = false;
        
        // Spawn fragments
        SpawnFragments(g_breakables[id].position + glm::vec3(0.0f, g_breakables[id].scale * 0.5f, 0.0f), 
                       g_breakables[id].fragment_count, 
                       g_breakables[id].fragment_size, 
                       g_breakables[id].fragment_color);

        // Spawn particles
        ParticleOptions popts;
        popts.color = g_breakables[id].fragment_color;
        popts.life = 0.8f;
        popts.scale = 0.05f;
        popts.speed = 3.0f;
        popts.count = 15;
        popts.additive = false; // Make dust particles solid instead of glowing
        Particles_Spawn(g_breakables[id].position + glm::vec3(0.0f, g_breakables[id].scale * 0.5f, 0.0f), popts);
                       
        // Drop collectible
        int collectible_type = -1;
        int r = rand() % 100;
        if (g_breakables[id].model_name == "the_park_bench") {
            collectible_type = (r < 75) ? 2 : 1; // 75% yellow(2), 25% green(1)
        } else if (g_breakables[id].model_name == "the_wooden_box") {
            collectible_type = (r < 25) ? 2 : 1; // 25% yellow(2), 75% green(1)
        } else if (g_breakables[id].model_name == "the_teddy_bear") {
            collectible_type = (r < 25) ? 0 : 2; // 25% red(0), 75% yellow(2)
        } else {
            collectible_type = rand() % 3;
        }
        SpawnCollectibles(g_breakables[id].position + glm::vec3(0.0f, g_breakables[id].scale, 0.0f), 1, collectible_type);
        
        player.objects_destroyed++;
    } else {
        PlaySoundEffect("../../data/sounds/break_hit.wav");
    }
}

void UpdateBreakables() {
    for (int i = 0; i < MAX_BREAKABLES; ++i) {
        if (!g_breakables[i].active) continue;
        
        if (g_breakables[i].is_flinching) {
            g_breakables[i].flinch_timer += delta_t;
            if (g_breakables[i].flinch_timer > 0.2f) { // Flinch duration
                g_breakables[i].is_flinching = false;
            }
        }
        
        // Gravity
        g_breakables[i].velocity_y += gravidade * delta_t;
        float move_y = g_breakables[i].velocity_y * delta_t;
        
        // Map collision
        for (int j = 0; j < g_num_platforms; ++j) {
            move_y = g_breakables[i].bbox.GetClipY(map[j].bbox, move_y);
        }

        // Breakables collision
        for (int j = 0; j < MAX_BREAKABLES; ++j) {
            if (i != j && g_breakables[j].active) {
                move_y = g_breakables[i].bbox.GetClipY(g_breakables[j].bbox, move_y);
            }
        }
        
        g_breakables[i].position.y += move_y;
        
        if (move_y == 0.0f) {
            g_breakables[i].velocity_y = 0.0f; // hit ground
        }
        
        if (g_breakables[i].position.y < -50.0f) {
            g_breakables[i].active = false; // Fall safe
        }
        
        // Update bbox
        g_breakables[i].bbox = makeAABBFromGround(g_breakables[i].position, glm::vec3(g_breakables[i].bbox_w * g_breakables[i].scale, g_breakables[i].bbox_h * g_breakables[i].scale, g_breakables[i].bbox_d * g_breakables[i].scale));
    }
}

void DrawBreakables(GLint model_uniform, GLint object_id_uniform, GLint override_kd_uniform, GLint use_override_kd_uniform) {
    for (int i = 0; i < MAX_BREAKABLES; ++i) {
        if (!g_breakables[i].active) continue;
        
        float y_offset = (g_breakables[i].bbox_h * g_breakables[i].scale) / 2.0f;
        if (g_breakables[i].model_name == "the_teddy_bear") {
            y_offset = 0.0f;
        } else if (g_breakables[i].model_name == "the_park_bench") {
            y_offset = ((g_breakables[i].bbox_h * g_breakables[i].scale) / 4.0f) + 0.1f;
        }
        glm::mat4 model = Matrix_Translate(g_breakables[i].position.x, g_breakables[i].position.y + y_offset, g_breakables[i].position.z)
                        * Matrix_Rotate_Y(g_breakables[i].rotation_y)
                        * Matrix_Rotate_X(g_breakables[i].rotation_x)
                        * Matrix_Rotate_Z(g_breakables[i].rotation_z)
                        * Matrix_Scale(g_breakables[i].scale, g_breakables[i].scale, g_breakables[i].scale);
        
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(object_id_uniform, 60); // BREAKABLE
        
        if (g_breakables[i].is_flinching) {
            glUniform1i(use_override_kd_uniform, 2); // special blend mode
            glUniform3f(override_kd_uniform, 1.0f, 1.0f, 1.0f);
        }
        
        void DrawVirtualObject(const char* object_name);
        for (const auto& pair : g_VirtualScene) {
            if (pair.first.find(g_breakables[i].model_name + "_") == 0 || pair.first == g_breakables[i].model_name) {
                if (pair.second.texture_id != 0) {
                    glActiveTexture(GL_TEXTURE29);
                    glBindTexture(GL_TEXTURE_2D, pair.second.texture_id);
                    extern GLuint g_GpuProgramID;
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage29"), 29);
                }
                DrawVirtualObject(pair.first.c_str());
            }
        }
        
        if (g_breakables[i].is_flinching) {
            glUniform1i(use_override_kd_uniform, 0);
        }
        
        extern void DrawBoundingBox(AABB& bbox, int original_object_id);
        DrawBoundingBox(g_breakables[i].bbox, 0); // use generic id for bbox lines
    }
}
