#include "globals.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include "sound.h"

Collectible g_collectibles[MAX_COLLECTIBLES];

void SpawnCollectibles(glm::vec3 pos, int count, int specific_type) {
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < MAX_COLLECTIBLES; ++j) {
            if (!g_collectibles[j].active) {
                g_collectibles[j].active = true;
                g_collectibles[j].position = pos;
                float angle = (rand() % 360) * (M_PI / 180.0f);
                float speed = 0.1f + (rand() % 100) / 100.0f * 0.2f; 
                g_collectibles[j].velocity = glm::vec3(cos(angle) * speed, 1.0f + (rand()%100)/100.0f * 0.5f, sin(angle) * speed);
                g_collectibles[j].timer = 0.0f;
                g_collectibles[j].duration = 10.0f;
                g_collectibles[j].blink_time = 7.0f;
                g_collectibles[j].scale = 0.15f;
                g_collectibles[j].visible_this_frame = true;
                if (specific_type != -1) {
                    g_collectibles[j].type = specific_type;
                } else {
                    g_collectibles[j].type = rand() % 3;
                }
                break;
            }
        }
    }
}

void UpdateCollectibles() {
    for (int i = 0; i < MAX_COLLECTIBLES; i++) {
        if (!g_collectibles[i].active) continue;

        g_collectibles[i].timer += delta_t;
        if (g_collectibles[i].timer >= g_collectibles[i].duration) {
            g_collectibles[i].active = false;
            continue;
        }

        // Fading is handled in the renderer
        g_collectibles[i].visible_this_frame = true;

        // Physics
        g_collectibles[i].velocity.y += gravidade * delta_t;
        g_collectibles[i].position.x += g_collectibles[i].velocity.x * delta_t;
        g_collectibles[i].position.z += g_collectibles[i].velocity.z * delta_t;

        float next_y = g_collectibles[i].position.y + g_collectibles[i].velocity.y * delta_t;
        bool ground_hit = false;
        
        for (int j = 0; j < g_num_platforms; ++j) {
            if (g_collectibles[i].position.x >= map[j].bbox.min.x && g_collectibles[i].position.x <= map[j].bbox.max.x &&
                g_collectibles[i].position.z >= map[j].bbox.min.z && g_collectibles[i].position.z <= map[j].bbox.max.z) {
                
                // Falling down
                if (g_collectibles[i].velocity.y < 0.0f && g_collectibles[i].position.y >= map[j].bbox.max.y && next_y <= map[j].bbox.max.y) {
                    next_y = map[j].bbox.max.y;
                    ground_hit = true;
                    break;
                }
            }
        }
        
        // Fallback ground collision
        if (next_y < -0.9f) {
            next_y = -0.9f;
            ground_hit = true;
        }

        g_collectibles[i].position.y = next_y;
        
        if (ground_hit) {
            g_collectibles[i].velocity.y = -g_collectibles[i].velocity.y * 0.3f; // bounce
            g_collectibles[i].velocity.x *= 0.5f; // friction
            g_collectibles[i].velocity.z *= 0.5f;
        }

        glm::vec3 player_center = player.position + glm::vec3(0.0f, 1.0f, 0.0f);
        float dist = glm::distance(g_collectibles[i].position, player_center);
        
        // Prevent instant pickup if enemy dies too close
        if (g_collectibles[i].timer > 0.5f) {
            bool can_collect = false;
            if (g_collectibles[i].type == 0 && player.health < player.max_health) can_collect = true;
            if (g_collectibles[i].type == 1 && player.transform_energy < player.max_transform_energy) can_collect = true;
            if (g_collectibles[i].type == 2 && player.special_energy < player.max_special_energy) can_collect = true;

            // Magnetic effect
            if (dist < 3.0f && can_collect) {
                glm::vec3 dir = glm::normalize(player_center - g_collectibles[i].position);
                g_collectibles[i].velocity += dir * 15.0f * delta_t; // accelerate towards player center
            }

            // Player pickup
            if (dist < 1.0f && can_collect) {
                if (g_collectibles[i].type == 0) {
                    player.health = std::min(player.health + 10.0f, player.max_health);
                } else if (g_collectibles[i].type == 1) {
                    player.transform_energy = std::min(player.transform_energy + 20.0f, player.max_transform_energy);
                } else if (g_collectibles[i].type == 2) {
                    player.special_energy = std::min(player.special_energy + 20.0f, player.max_special_energy);
                }
                PlaySoundEffect("../../data/sounds/absorb.wav");
                g_collectibles[i].active = false;
            }
        }
    }
}
