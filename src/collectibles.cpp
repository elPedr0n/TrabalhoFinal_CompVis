#include "globals.h"
#include <iostream>
#include <cmath>
#include <cstdlib>

Collectible g_collectibles[MAX_COLLECTIBLES];

void SpawnCollectibles(glm::vec3 pos, int count) {
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < MAX_COLLECTIBLES; j++) {
            if (!g_collectibles[j].active) {
                g_collectibles[j].active = true;
                g_collectibles[j].position = pos;
                float angle = (rand() % 360) * (M_PI / 180.0f);
                float speed = 0.1f + (rand() % 100) / 100.0f * 0.2f; 
                g_collectibles[j].velocity = glm::vec3(cos(angle) * speed, 1.0f + (rand()%100)/100.0f * 0.5f, sin(angle) * speed);
                g_collectibles[j].timer = 0.0f;
                g_collectibles[j].duration = 10.0f;
                g_collectibles[j].blink_time = 7.0f;
                g_collectibles[j].scale = 0.08f;
                g_collectibles[j].visible_this_frame = true;
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

        // Blinking
        if (g_collectibles[i].timer >= g_collectibles[i].blink_time) {
            float blink_rate = 10.0f + (g_collectibles[i].timer - g_collectibles[i].blink_time) * 10.0f;
            g_collectibles[i].visible_this_frame = (sin(g_collectibles[i].timer * blink_rate) > 0.0f);
        } else {
            g_collectibles[i].visible_this_frame = true;
        }

        // Physics
        g_collectibles[i].velocity.y += gravidade * delta_t;
        g_collectibles[i].position += g_collectibles[i].velocity * delta_t;

        // Ground collision
        if (g_collectibles[i].position.y < -0.9f) {
            g_collectibles[i].position.y = -0.9f;
            g_collectibles[i].velocity.y = -g_collectibles[i].velocity.y * 0.3f; // bounce
            g_collectibles[i].velocity.x *= 0.5f; // friction
            g_collectibles[i].velocity.z *= 0.5f;
        }

        float dist = glm::distance(g_collectibles[i].position, player.position);
        
        // Prevent instant pickup if enemy dies too close
        if (g_collectibles[i].timer > 0.5f) {
            // Magnetic effect
            if (dist < 3.0f && player.health < player.max_health) {
                glm::vec3 dir = glm::normalize(player.position - g_collectibles[i].position);
                g_collectibles[i].velocity += dir * 15.0f * delta_t; // accelerate towards player
            }

            // Player pickup
            if (dist < 1.0f) {
                if (player.health < player.max_health) {
                    player.health += 10.0f;
                    if (player.health > player.max_health) player.health = player.max_health;
                    g_collectibles[i].active = false;
                }
            }
        }
    }
}
