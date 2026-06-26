#include "globals.h"
#include "breakables.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include "particles.h"
#include "projectiles.h"
#include "sound.h"
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define BUNNY 1


void ResolvePlayerMapCollisions();

void SpawnEnemy(glm::vec3 pos, int spawner_id) {
    if (rand() % 3 == 0) {
        SpawnRangedEnemy(pos, spawner_id);
        return;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) {
            g_enemies[i].visible = true;
            pos.y -= 0.5f;
            g_enemies[i].position = pos;
            g_enemies[i].rotate = 0.0f;
            g_enemies[i].scale = 1.0f;
            g_enemies[i].max_health = 100.0f;
            g_enemies[i].health = 100.0f;
            g_enemies[i].speed = 1.5f;
            g_enemies[i].attack_range = 0.9f;
            g_enemies[i].is_attacking = false;
            g_enemies[i].attack_timer = 0.0f;
            g_enemies[i].attack_cooldown = 0.0f;
            g_enemies[i].is_flinching = false;
            g_enemies[i].flinch_timer = 0.0f;
            g_enemies[i].is_dead = false;
            g_enemies[i].death_timer = 0.0f;
            g_enemies[i].is_flashing = false;
            g_enemies[i].flash_timer = 0.0f;
            g_enemies[i].punch_active = false;
            g_enemies[i].has_hit_player = false;
            g_enemies[i].type = 0; // Melee
            g_enemies[i].is_spawning = true;
            g_enemies[i].spawn_timer = 0.0f;
            g_enemies[i].spawn_duration = 2.0f;
            g_enemies[i].spawner_id = spawner_id;
            g_enemies[i].bbox = MakeAABBFromCenterSize(g_enemies[i].position + glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
            if (spawner_id != -1) {
                g_spawn_points[spawner_id].active_enemy_id = i;
                g_spawn_points[spawner_id].enemies_spawned++;
            }
            if ((rand() % 4 == 0) && glm::distance(pos, player.position) < 8.0f) {
                PlaySoundEffect("../../data/sounds/knight_laugh.wav");
            }
            break;
        }
    }
}

void SpawnRangedEnemy(glm::vec3 pos, int spawner_id) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) {
            g_enemies[i].visible = true;
            pos.y -= 0.5f;
            g_enemies[i].position = pos;
            g_enemies[i].rotate = 0.0f;
            g_enemies[i].scale = 1.0f;
            g_enemies[i].max_health = 80.0f; // Less health for ranged
            g_enemies[i].health = 80.0f;
            g_enemies[i].speed = 1.2f; // Slower
            g_enemies[i].attack_range = 6.0f; // Far away attack
            g_enemies[i].is_attacking = false;
            g_enemies[i].attack_timer = 0.0f;
            g_enemies[i].attack_cooldown = 0.0f;
            g_enemies[i].is_flinching = false;
            g_enemies[i].flinch_timer = 0.0f;
            g_enemies[i].is_dead = false;
            g_enemies[i].death_timer = 0.0f;
            g_enemies[i].is_flashing = false;
            g_enemies[i].flash_timer = 0.0f;
            g_enemies[i].punch_active = false;
            g_enemies[i].has_hit_player = false;
            g_enemies[i].type = 1; // Ranged
            g_enemies[i].attack_phase = 0;
            g_enemies[i].is_spawning = true;
            g_enemies[i].spawn_timer = 0.0f;
            g_enemies[i].spawn_duration = 2.0f;
            g_enemies[i].spawner_id = spawner_id;
            g_enemies[i].bbox = MakeAABBFromCenterSize(g_enemies[i].position + glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
            if (spawner_id != -1) {
                g_spawn_points[spawner_id].active_enemy_id = i;
                g_spawn_points[spawner_id].enemies_spawned++;
            }
            if ((rand() % 4 == 0) && glm::distance(pos, player.position) < 8.0f) {
                PlaySoundEffect("../../data/sounds/knight_laugh.wav");
            }
            break;
        }
    }
}

void UpdateEnemies() {

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) continue;

        // Reset per-frame flag
        g_enemies[i].punch_active = false;

        if (g_enemies[i].is_spawning) {
            g_enemies[i].spawn_timer += delta_t;
            if (g_enemies[i].spawn_timer >= g_enemies[i].spawn_duration) {
                g_enemies[i].is_spawning = false;
            }
            continue; // Skip movement, attacks, gravity, etc
        }

        // ===== TEMPO E CONGELAMENTO =====
        float time_scale = 1.0f;
        if (g_enemies[i].is_frozen) {
            g_enemies[i].frozen_timer -= delta_t;
            if (g_enemies[i].frozen_timer <= 0.0f) {
                if (g_enemies[i].is_frozen) {
                    PlaySoundEffect("../../data/sounds/ice_break.wav");
                }
                g_enemies[i].is_frozen = false;
            } else {
                time_scale = 0.3f;
            }
        }

        // ===== GRAVITY =====
        float fall_y = gravidade * delta_t; 
        fall_y = CheckMapCollisionY(g_enemies[i].bbox, fall_y);
        fall_y = CheckBreakablesCollisionY(g_enemies[i].bbox, fall_y);
        g_enemies[i].position.y += fall_y;
        g_enemies[i].bbox = MakeAABBFromCenterSize(
            g_enemies[i].position + glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));

        // Safety net for enemies
        if (g_enemies[i].position.y < -5.0f) {
            if (g_enemies[i].health > 0.0f) {
                PlaySoundEffect("../../data/sounds/knight_death_water.wav");
                g_enemies[i].health = 0.0f;
                g_enemies[i].is_dead = true;
                g_enemies[i].death_timer = 0.0f;
                g_enemies[i].death_anim_duration = 2.66f; 
                g_enemies[i].is_attacking = false;
                g_enemies[i].punch_active = false;

                if (g_enemies[i].spawner_id != -1) {
                    int sid = g_enemies[i].spawner_id;
                    g_spawn_points[sid].enemies_killed++;
                    g_spawn_points[sid].active_enemy_id = -1;

                    // Check win condition
                    bool all_i_spawners_dead = true;
                    bool has_i_spawners = false;
                    for(int k=0; k<g_num_spawn_points; k++) {
                        if (g_spawn_points[k].type == 1) { // Special 'i'
                            has_i_spawners = true;
                            if (g_spawn_points[k].enemies_killed < g_spawn_points[k].max_enemies) {
                                all_i_spawners_dead = false;
                                break;
                            }
                        }
                    }
                    if (has_i_spawners && all_i_spawners_dead) {
                        player.has_won = true;
                        player.final_time = (float)glfwGetTime();
                        printf("You won! All special enemies defeated!\n");
                    }
                }
            }
        }

        if (!g_enemies[i].is_dead && glm::distance(player.position, g_enemies[i].position) < 8.0f) {
            if ((rand() % 10000) < 5) { 
                PlaySoundEffect("../../data/sounds/knight_laugh.wav");
            }
        }

        // Timer for attacks
        if (g_enemies[i].attack_cooldown > 0.0f) {
            g_enemies[i].attack_cooldown -= delta_t * time_scale;
        }

        // ===== DEATH STATE MACHINE =====
        if (g_enemies[i].is_dead) {
            g_enemies[i].death_timer += delta_t;
            if (g_enemies[i].death_timer >= g_enemies[i].death_anim_duration) {
                g_enemies[i].is_flashing = true;
                g_enemies[i].flash_timer += delta_t;
                if (g_enemies[i].flash_timer >= 1.0f) {
                    g_enemies[i].visible = false;
                }
            }
            continue; // Skip movement and attacking
        }

        // ===== FLINCH STATE MACHINE =====
        if (g_enemies[i].is_flinching) {
            g_enemies[i].flinch_timer += delta_t * time_scale;
            if (g_enemies[i].flinch_timer >= g_enemies[i].flinch_duration) {
                g_enemies[i].is_flinching = false;
            }
            continue; // Skip movement and attacking
        }

        // ===== ATTACK STATE MACHINE =====
        if (g_enemies[i].is_attacking) {
            g_enemies[i].attack_timer += delta_t * time_scale;

            if (g_enemies[i].type == 0) { // MELEE
                // Hitbox active window (tune these) - Ativa apenas nos 80% centrais da animação de 1.25s
                float elapsed = g_enemies[i].attack_timer;
                if (elapsed >= 0.425f && elapsed <= 0.825f) {
                    g_enemies[i].punch_active = true;
                }

                // Attack finished
                if (g_enemies[i].attack_timer >= g_enemies[i].attack_duration) {
                    g_enemies[i].is_attacking = false;
                    g_enemies[i].attack_cooldown = 1.5f; // cooldown before next attack
                }
            } else if (g_enemies[i].type == 1) { // RANGED
                if (g_enemies[i].attack_phase == 0) { // Prepare (Anim 22)
                    if (g_enemies[i].attack_timer >= 1.5f) {
                        g_enemies[i].attack_phase = 1;
                        g_enemies[i].attack_timer = 0.0f; // Reset for next phase
                    }
                } else if (g_enemies[i].attack_phase == 1) { // Idle attack mode (Anim 23)
                    g_enemies[i].phase_timer += delta_t * time_scale;
                    if (g_enemies[i].phase_timer >= 2.0f) { // wait 2s between shots
                        g_enemies[i].attack_phase = 2;
                        g_enemies[i].attack_timer = 0.0f; // Reset for anim 25
                        glm::vec3 forward = glm::vec3(sin(g_enemies[i].rotate), 0.0f, cos(g_enemies[i].rotate));
                        glm::vec3 spawn_pos = g_enemies[i].position + glm::vec3(0.0f, 1.0f, 0.0f) + forward * 1.5f;
                        PlaySoundEffect("../../data/sounds/knight_shot.wav");
                        // Spawn bezier projectile!
                        Projectiles_SpawnBezier(std::string("the_sphere"), spawn_pos, player.position + glm::vec3(0.0f, 0.5f, 0.0f), true);
                    }
                } else if (g_enemies[i].attack_phase == 2) { // Shooting (Anim 25)
                    if (g_enemies[i].attack_timer >= 1.0f) { // approx anim duration
                        g_enemies[i].attack_phase = 1;
                        g_enemies[i].phase_timer = 0.0f; // Reset wait timer
                        g_enemies[i].attack_timer = 100.0f; // Jump to end of anim 23 so it doesn't replay
                    }
                }

                // Check if player moved out of range
                float dist_to_player = glm::distance(
                    glm::vec3(g_enemies[i].position.x, 0.0f, g_enemies[i].position.z),
                    glm::vec3(player.position.x, 0.0f, player.position.z));
                    
                if (dist_to_player > g_enemies[i].attack_range && g_enemies[i].attack_phase == 1) {
                    g_enemies[i].is_attacking = false;
                    g_enemies[i].attack_phase = 0;
                    g_enemies[i].attack_timer = 0.0f;
                    g_enemies[i].phase_timer = 0.0f;
                } else {
                    // Ranged enemies keep facing player while attacking
                    glm::vec3 direction_to_player = glm::normalize(glm::vec3(
                        player.position.x - g_enemies[i].position.x,
                        0.0f,
                        player.position.z - g_enemies[i].position.z
                    ));
                    g_enemies[i].rotate = atan2(direction_to_player.x, direction_to_player.z);
                }
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
            if (g_enemies[i].type == 1) {
                g_enemies[i].attack_phase = 0; // Start preparation
            }
            // Face the player when attacking
            g_enemies[i].rotate = atan2(
                player.position.x - g_enemies[i].position.x,
                player.position.z - g_enemies[i].position.z);
            continue;
        }

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
            float move_x = direction_to_player.x * g_enemies[i].speed * (delta_t * time_scale);
            float move_z = direction_to_player.z * g_enemies[i].speed * (delta_t * time_scale);

            // Clip against map platforms (same pattern as player.cpp)
            move_x = CheckMapCollisionX(g_enemies[i].bbox, move_x);
            move_x = CheckBreakablesCollisionX(g_enemies[i].bbox, move_x);
            g_enemies[i].position.x += move_x;
            g_enemies[i].bbox = MakeAABBFromCenterSize(
                g_enemies[i].position + glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));

            move_z = CheckMapCollisionZ(g_enemies[i].bbox, move_z);
            move_z = CheckBreakablesCollisionZ(g_enemies[i].bbox, move_z);
            g_enemies[i].position.z += move_z;
        }

        // Update AABB
        g_enemies[i].bbox = MakeAABBFromCenterSize(
            g_enemies[i].position + glm::vec3(0.0f, -0.1f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    }
}

void ApplyDamageToEnemy(int enemy_id, float damage, bool cause_flinch) {
    if (g_enemies[enemy_id].is_dead || g_enemies[enemy_id].is_spawning) return;
    
    g_enemies[enemy_id].health -= damage;
    if (g_enemies[enemy_id].health <= 0.0f) {
        PlaySoundEffect("../../data/sounds/knight_death.wav");
        g_enemies[enemy_id].health = 0.0f;
        g_enemies[enemy_id].is_dead = true;
        g_enemies[enemy_id].death_timer = 0.0f;
        g_enemies[enemy_id].death_anim_duration = 2.66f; 
        g_enemies[enemy_id].is_attacking = false;
        g_enemies[enemy_id].punch_active = false;
        
        player.enemies_slain++;

        if (g_enemies[enemy_id].spawner_id != -1) {
            int sid = g_enemies[enemy_id].spawner_id;
            g_spawn_points[sid].enemies_killed++;
            g_spawn_points[sid].active_enemy_id = -1;

            // Check win condition
            bool all_i_spawners_dead = true;
            bool has_i_spawners = false;
            for(int k=0; k<g_num_spawn_points; k++) {
                if (g_spawn_points[k].type == 1) { // Special 'i'
                    has_i_spawners = true;
                    if (g_spawn_points[k].enemies_killed < g_spawn_points[k].max_enemies) {
                        all_i_spawners_dead = false;
                        break;
                    }
                }
            }
            if (has_i_spawners && all_i_spawners_dead) {
                player.has_won = true;
                player.final_time = (float)glfwGetTime();
                printf("You won! All special enemies defeated!\n");
            }
        }

        SpawnCollectibles(g_enemies[enemy_id].position, 1, 0);

    } else if (cause_flinch) {
        if (rand() % 2 == 0) PlaySoundEffect("../../data/sounds/knight_hurt1.wav");
        else PlaySoundEffect("../../data/sounds/knight_hurt2.wav");
        // Flinch
        g_enemies[enemy_id].is_flinching = true;
        g_enemies[enemy_id].flinch_timer = 0.0f;
        g_enemies[enemy_id].is_attacking = false; // cancel attack
        g_enemies[enemy_id].punch_active = false;
        
        if (rand() % 2 == 0) {
            g_enemies[enemy_id].flinch_anim = 14;
            g_enemies[enemy_id].flinch_duration = 0.54f;
        } else {
            g_enemies[enemy_id].flinch_anim = 15;
            g_enemies[enemy_id].flinch_duration = 0.29f;
        }
        g_enemies[enemy_id].attack_cooldown = g_enemies[enemy_id].flinch_duration + 0.4f;
    }
}

