#include "globals.h"
#include <iostream>
#include <cmath>

void UpdateEnemies() {

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) continue;

        // Reset per-frame flag
        g_enemies[i].punch_active = false;

        // ===== ATTACK COOLDOWN =====
        if (g_enemies[i].attack_cooldown > 0.0f) {
            g_enemies[i].attack_cooldown -= delta_t;
        }

        // ===== ATTACK STATE MACHINE =====
        if (g_enemies[i].is_attacking) {
            g_enemies[i].attack_timer += delta_t;

            // Hitbox active window (tune these)
            float elapsed = g_enemies[i].attack_timer;
            if (elapsed >= 0.2f && elapsed <= 0.5f) {
                g_enemies[i].punch_active = true;
            }

            // Attack finished
            if (g_enemies[i].attack_timer >= g_enemies[i].attack_duration) {
                g_enemies[i].is_attacking = false;
                g_enemies[i].attack_cooldown = 1.5f; // cooldown before next attack
            }

            // While attacking, don't move — skip movement below
            // Update AABB in place and continue
            g_enemies[i].bbox = MakeAABBFromCenterSize(
                g_enemies[i].position, glm::vec3(1.0f, 0.99f, 0.775f));
            continue;
        }

        // ===== ATTACK TRIGGER (choose one, comment the other) =====
        float dist_to_player = glm::distance(
            glm::vec3(g_enemies[i].position.x, 0.0f, g_enemies[i].position.z),
            glm::vec3(player.position.x, 0.0f, player.position.z));

        // --- OPTION A: Distance-based trigger ---
        if (dist_to_player <= g_enemies[i].attack_range 
            && g_enemies[i].attack_cooldown <= 0.0f) 
        {
            g_enemies[i].is_attacking = true;
            g_enemies[i].attack_timer = 0.0f;
            g_enemies[i].has_hit_player = false;
            // Face the player when attacking
            g_enemies[i].rotate = atan2(
                player.position.x - g_enemies[i].position.x,
                player.position.z - g_enemies[i].position.z);
            continue;
        }

        // --- OPTION B: AABB intersection trigger (comment A, uncomment this) ---
        // if (g_enemies[i].bbox.Intersects(player.characters[player.active_character].bbox)
        //     && g_enemies[i].attack_cooldown <= 0.0f)
        // {
        //     g_enemies[i].is_attacking = true;
        //     g_enemies[i].attack_timer = 0.0f;
        //     g_enemies[i].has_hit_player = false;
        //     g_enemies[i].rotate = atan2(
        //         player.position.x - g_enemies[i].position.x,
        //         player.position.z - g_enemies[i].position.z);
        //     continue;
        // }

        // ===== MOVEMENT (only when not attacking) =====
        if (dist_to_player > g_enemies[i].attack_range) {
            glm::vec3 direction_to_player = glm::normalize(glm::vec3(
                player.position.x - g_enemies[i].position.x,
                0.0f,
                player.position.z - g_enemies[i].position.z
            ));

            // Face the player while walking
            g_enemies[i].rotate = atan2(direction_to_player.x, direction_to_player.z);

            // Compute desired movement
            float move_x = direction_to_player.x * g_enemies[i].speed * delta_t;
            float move_z = direction_to_player.z * g_enemies[i].speed * delta_t;

            // Clip against map platforms (same pattern as player.cpp)
            for (int j = 0; j < MAX_PLATFORMS; j++) {
                move_x = g_enemies[i].bbox.GetClipX(map[j].bbox, move_x);
            }
            g_enemies[i].position.x += move_x;
            // g_enemies[i].bbox.Move(move_x, 0.0f, 0.0f);

            for (int j = 0; j < MAX_PLATFORMS; j++) {
                move_z = g_enemies[i].bbox.GetClipZ(map[j].bbox, move_z);
            }
            g_enemies[i].position.z += move_z;
        }

        // Update AABB
        g_enemies[i].bbox = MakeAABBFromCenterSize(
            g_enemies[i].position, glm::vec3(1.0f, 0.99f, 0.775f));
    }
}