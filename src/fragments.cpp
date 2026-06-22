#include "fragments.h"
#include "matrices.h"
#include "globals.h"
#include <cstdlib>
#include <cstdlib>

Fragment g_fragments[MAX_FRAGMENTS];

void SpawnFragments(glm::vec3 pos, int count, float scale, glm::vec3 color) {
    int spawned = 0;
    for (int i = 0; i < MAX_FRAGMENTS && spawned < count; ++i) {
        if (!g_fragments[i].active) {
            g_fragments[i].active = true;
            g_fragments[i].position = pos + glm::vec3(
                ((rand() % 100) / 100.0f - 0.5f) * scale,
                ((rand() % 100) / 100.0f - 0.5f) * scale,
                ((rand() % 100) / 100.0f - 0.5f) * scale
            );
            
            float angle_xz = (rand() % 360) * (M_PI / 180.0f);
            float speed_xz = 0.5f + (rand() % 100) / 100.0f * 1.5f;
            float speed_y = 1.0f + (rand() % 100) / 100.0f * 2.0f;
            
            g_fragments[i].velocity = glm::vec3(cos(angle_xz) * speed_xz, speed_y, sin(angle_xz) * speed_xz);
            
            g_fragments[i].rotation = glm::vec3(
                (rand() % 360) * (M_PI / 180.0f),
                (rand() % 360) * (M_PI / 180.0f),
                (rand() % 360) * (M_PI / 180.0f)
            );
            
            g_fragments[i].rotation_speed = glm::vec3(
                ((rand() % 100) / 100.0f - 0.5f) * 10.0f,
                ((rand() % 100) / 100.0f - 0.5f) * 10.0f,
                ((rand() % 100) / 100.0f - 0.5f) * 10.0f
            );
            
            g_fragments[i].scale = scale * (0.5f + (rand() % 100) / 200.0f); // Variable sizes
            g_fragments[i].color = color;
            g_fragments[i].timer = 0.0f;
            g_fragments[i].duration = 3.0f + (rand() % 100) / 100.0f * 2.0f; // 3 to 5 seconds
            
            spawned++;
        }
    }
}

void UpdateFragments() {
    for (int i = 0; i < MAX_FRAGMENTS; ++i) {
        if (!g_fragments[i].active) continue;
        
        g_fragments[i].timer += delta_t;
        if (g_fragments[i].timer >= g_fragments[i].duration) {
            g_fragments[i].active = false;
            continue;
        }
        
        g_fragments[i].velocity.y += gravidade * delta_t;
        g_fragments[i].position.x += g_fragments[i].velocity.x * delta_t;
        g_fragments[i].position.z += g_fragments[i].velocity.z * delta_t;
        g_fragments[i].rotation += g_fragments[i].rotation_speed * delta_t;
        
        float next_y = g_fragments[i].position.y + g_fragments[i].velocity.y * delta_t;
        bool ground_hit = false;
        
        for (int j = 0; j < g_num_platforms; ++j) {
            if (g_fragments[i].position.x >= map[j].bbox.min.x && g_fragments[i].position.x <= map[j].bbox.max.x &&
                g_fragments[i].position.z >= map[j].bbox.min.z && g_fragments[i].position.z <= map[j].bbox.max.z) {
                
                if (g_fragments[i].velocity.y < 0.0f && g_fragments[i].position.y >= map[j].bbox.max.y && next_y <= map[j].bbox.max.y) {
                    next_y = map[j].bbox.max.y;
                    ground_hit = true;
                    break;
                }
            }
        }
        
        if (next_y < -0.9f) {
            next_y = -0.9f;
            ground_hit = true;
        }

        g_fragments[i].position.y = next_y;
        
        if (ground_hit) {
            g_fragments[i].velocity.y = -g_fragments[i].velocity.y * 0.4f; // Bounce
            g_fragments[i].velocity.x *= 0.6f; // Friction
            g_fragments[i].velocity.z *= 0.6f;
            g_fragments[i].rotation_speed *= 0.6f;
        }
    }
}

void DrawFragments(GLint model_uniform, GLint object_id_uniform, GLint override_kd_uniform, GLint use_override_kd_uniform) {
    for (int i = 0; i < MAX_FRAGMENTS; ++i) {
        if (!g_fragments[i].active) continue;
        
        // Fade out at the end of duration
        float alpha = 1.0f;
        float remaining = g_fragments[i].duration - g_fragments[i].timer;
        if (remaining < 1.0f) {
            alpha = remaining;
            if (alpha < 0.0f) alpha = 0.0f;
        }
        
        if (alpha < 1.0f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }
        
        // Pass alpha to shader using the existing enemy_alpha uniform
        extern GLuint g_GpuProgramID;
        glUniform1f(glGetUniformLocation(g_GpuProgramID, "enemy_alpha"), alpha);
        
        glm::mat4 model = Matrix_Translate(g_fragments[i].position.x, g_fragments[i].position.y, g_fragments[i].position.z)
                        * Matrix_Rotate_X(g_fragments[i].rotation.x)
                        * Matrix_Rotate_Y(g_fragments[i].rotation.y)
                        * Matrix_Rotate_Z(g_fragments[i].rotation.z)
                        * Matrix_Scale(g_fragments[i].scale, g_fragments[i].scale * 0.05f, g_fragments[i].scale); // Very thin Y axis to look like a flat polygon/fragment
        
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        
        glUniform1i(object_id_uniform, 0); // generic object ID
        glUniform1i(use_override_kd_uniform, 1);
        glUniform3f(override_kd_uniform, g_fragments[i].color.x, g_fragments[i].color.y, g_fragments[i].color.z);
        
        void DrawVirtualObject(const char* object_name);
        DrawVirtualObject("the_plane"); // Draw as a flattened polygon
        
        glUniform1i(use_override_kd_uniform, 0); // Reset
        glUniform1f(glGetUniformLocation(g_GpuProgramID, "enemy_alpha"), 1.0f); // Reset alpha
        
        if (alpha < 1.0f) {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }
    }
}
