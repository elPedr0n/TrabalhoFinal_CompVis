#include "globals.h"
#include "breakables.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include "particles.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define BUNNY 1


void ResolvePlayerMapCollisions();

void SpawnEnemy(glm::vec3 pos) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) {
            g_enemies[i].visible = true;
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
            g_enemies[i].bbox = MakeAABBFromCenterSize(g_enemies[i].position + glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.5f, 1.0f, 0.5f));
            break;
        }
    }
}

void UpdateEnemies() {

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) continue;

        // Reset per-frame flag
        g_enemies[i].punch_active = false;

// ===== TEMPO E CONGELAMENTO =====
        float time_scale = 1.0f;
        if (g_enemies[i].is_frozen) {
            g_enemies[i].frozen_timer -= delta_t;
            if (g_enemies[i].frozen_timer <= 0.0f) {
                g_enemies[i].is_frozen = false;
            } else {
                time_scale = 0.3f;
            }
        } // <-- ESSA É A CHAVE QUE O GIT ENGOLIU!

        // ===== GRAVITY =====
        float fall_y = gravidade * delta_t; 
        for (int j = 0; j < MAX_PLATFORMS; j++) {
            fall_y = g_enemies[i].bbox.GetClipY(map[j].bbox, fall_y);
        }
        for (int j = 0; j < MAX_BREAKABLES; j++) {
            if (g_breakables[j].active) {
                fall_y = g_enemies[i].bbox.GetClipY(g_breakables[j].bbox, fall_y);
            }
        }
        g_enemies[i].position.y += fall_y;

        // Safety net for enemies
        if (g_enemies[i].position.y <= -10.0f) {
            g_enemies[i].position.y = -10.0f;
        }
        

        // ===== ATTACK COOLDOWN =====
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
            float move_x = direction_to_player.x * g_enemies[i].speed * (delta_t * time_scale);
            float move_z = direction_to_player.z * g_enemies[i].speed * (delta_t * time_scale);

            // Clip against map platforms (same pattern as player.cpp)
            for (int j = 0; j < MAX_PLATFORMS; j++) {
                move_x = g_enemies[i].bbox.GetClipX(map[j].bbox, move_x);
            }
            for (int j = 0; j < MAX_BREAKABLES; j++) {
                if (g_breakables[j].active) {
                    move_x = g_enemies[i].bbox.GetClipX(g_breakables[j].bbox, move_x);
                }
            }
            g_enemies[i].position.x += move_x;
            // g_enemies[i].bbox.Move(move_x, 0.0f, 0.0f);

            for (int j = 0; j < MAX_PLATFORMS; j++) {
                move_z = g_enemies[i].bbox.GetClipZ(map[j].bbox, move_z);
            }
            for (int j = 0; j < MAX_BREAKABLES; j++) {
                if (g_breakables[j].active) {
                    move_z = g_enemies[i].bbox.GetClipZ(g_breakables[j].bbox, move_z);
                }
            }
            g_enemies[i].position.z += move_z;
        }

        // Update AABB
        g_enemies[i].bbox = MakeAABBFromCenterSize(
            g_enemies[i].position, glm::vec3(1.0f, 0.99f, 0.775f));
    }
}

void ApplyDamageToEnemy(int enemy_id, float damage, bool cause_flinch) {
    if (g_enemies[enemy_id].is_dead) return;
    
    g_enemies[enemy_id].health -= damage;
    if (g_enemies[enemy_id].health <= 0.0f) {
        g_enemies[enemy_id].health = 0.0f;
        g_enemies[enemy_id].is_dead = true;
        g_enemies[enemy_id].death_timer = 0.0f;
        g_enemies[enemy_id].death_anim_duration = 2.66f; 
        g_enemies[enemy_id].is_attacking = false;
        g_enemies[enemy_id].punch_active = false;
        
        player.enemies_slain++;

        SpawnCollectibles(g_enemies[enemy_id].position, 1, 0);

        for(int k=0; k<2; ++k) {
            float angle = (rand() % 360) * (M_PI / 180.0f);
            float distance = 2.0f + (rand() % 100) / 10.0f; // 10 to 20 units away
            glm::vec3 spawn_pos;
            spawn_pos.x = player.position.x + cos(angle) * distance;
            spawn_pos.y = 2.0f;
            spawn_pos.z = player.position.z + std::abs(sin(angle) * distance);
            SpawnEnemy(spawn_pos);
        }
    } else if (cause_flinch) {
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

void ProcessEnemyMeleeHitboxes()
{
    auto& player_bbox = player.characters[player.active_character].bbox;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) continue;
        if (!g_enemies[i].punch_active) continue;
        glm::vec3 forward = glm::vec3(
            sin(g_enemies[i].rotate), 0.0f, cos(g_enemies[i].rotate));
        
        // Ajustando tamanho e alcance para casar com o ataque
        glm::vec3 hitbox_size = glm::vec3(0.8f, 0.6f, 0.8f);
        float reach = 0.8f;
        float height = 0.3f;

        glm::vec3 center = g_enemies[i].position 
                         + forward * reach 
                         + glm::vec3(0.0f, height, 0.0f);
        AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);
        
        // Desenhamos a hitbox SEMPRE que punch_active for true
        DrawBoundingBox(punch_box, BUNNY);

        // Somente depois de desenhar, verificamos se já bateu no player para não contar dano duplo
        if (g_enemies[i].has_hit_player) continue;

        if (punch_box.Intersects(player_bbox)) {
            printf("Enemy %d hit the player!\n", i);
            g_enemies[i].has_hit_player = true;
            if (!player.is_dead) {
                glm::vec3 knockback_dir = player.position - g_enemies[i].position;
                knockback_dir.y = 0.0f;
                if (glm::length(knockback_dir) > 0.001f) {
                    knockback_dir = glm::normalize(knockback_dir);
                } else {
                    knockback_dir = forward;
                }
                player.position += knockback_dir * 0.05f; // small knockback
                ResolvePlayerMapCollisions();

                // Rotate player to face the enemy
                glm::vec3 look_dir = g_enemies[i].position - player.position;
                player.rotate = atan2(look_dir.x, look_dir.z);

                float defense = player.active_character == 0 ? 0.5f : (player.active_character == 1 ? 0.4f : 1.0f);
                float damage = 50.0f * defense;
                player.health -= damage;
                
                glm::vec3 overlap_min = glm::max(punch_box.min, player_bbox.min);
                glm::vec3 overlap_max = glm::min(punch_box.max, player_bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts;
                popts.color = HexToRgb("#ffffff");
                popts.life = 0.3f;
                popts.scale = 0.15f;
                popts.speed = 2.0f;
                popts.count = 15;
                Particles_Spawn(contact, popts);

                if (player.health <= 0.0f) {
                    player.health = 0.0f;
                    player.is_dead = true;
                    player.death_timer = 0.0f;
                    if (player.speed.y > 0.0f) player.speed.y = 0.0f;
                    
                    if (player.active_character != 2) {
                        player.active_character = 2; // Ben
                        player.characters[2].bbox = makeAABBFromGround(player.position, bentennyson_size);
                        ResolvePlayerMapCollisions();
                        
                        ParticleOptions popts;
                        popts.color = HexToRgb("#ff0000"); // Red flash on forced revert (damage)
                        popts.life = 0.25f + 0.15f * 1.0f;
                        popts.scale = 0.15f + 0.01f * 6.0f;
                        popts.speed = 0.1f + 0.8f * 3.0f;
                        popts.count = std::max(2, (int)std::round(8.0f * 6.0f));
                        Particles_Spawn(glm::vec3(player.position.x, player.position.y, player.position.z), popts);
                    }
                } else {
                    player.is_flinching = true;
                    player.flinch_timer = 0.0f;
                }
            }
        }
    }
}