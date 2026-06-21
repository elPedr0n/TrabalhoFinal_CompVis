#include <glad/glad.h>  
#include <GLFW/glfw3.h>
#include "globals.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include "animation.h"
#include "breakables.h"
#include "particles.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

void UpdatePosition(bool can_move, bool can_rotate = false) {

    float input_x = 0.0f;
    float input_z = 0.0f;

    if (can_move || can_rotate) {
        if (keys[GLFW_KEY_A]) input_x -= 1.0f;
        if (keys[GLFW_KEY_D]) input_x += 1.0f;
        if (keys[GLFW_KEY_S]) input_z += 1.0f;
        if (keys[GLFW_KEY_W]) input_z -= 1.0f;
    }

    // 2. Variáveis para o movimento final rotacionado
    float move_x = 0.0f;
    float move_z = 0.0f;

    if (input_x != 0.0f || input_z != 0.0f) {
        
        // Normaliza o vetor 
        float length = sqrt(input_x * input_x + input_z * input_z);
        input_x /= length;
        input_z /= length;

        // Rotacionar o vetor de input pelo ângulo da câmera
        move_x =  input_x * cos(g_CameraTheta) + input_z * sin(g_CameraTheta);
        move_z = -input_x * sin(g_CameraTheta) + input_z * cos(g_CameraTheta);

        if (can_move || can_rotate) {
            // Calcular a orientação (rotação) do modelo 3D do personagem
            float target_angle = atan2(move_x, move_z);
        
        // Calcula a diferença entre o ângulo alvo e o ângulo atual
        float diff = target_angle - player.rotate;

        // Normaliza a diferença para ficar no intervalo de -π a π.
        // Isso impede que o personagem dê "uma volta completa" ao passar do ângulo -π para π.
        while (diff < -M_PI) diff += (2.0f * M_PI);
        while (diff >  M_PI) diff -= (2.0f * M_PI);

        // Velocidade de rotação do personagem (aumente para virar mais rápido)
        float turn_speed = 10.0f; 

        // Calcula a quantidade exata de rotação para este frame
        float step = diff * turn_speed * delta_t;

        if (std::abs(step) >= std::abs(diff)) {
            // Se o passo for maior que a distância que falta, crava no alvo
            player.rotate = target_angle;
        } else {
            // Caso contrário, gira suavemente
            player.rotate += step;
        }

        // Mantém o ângulo final dentro de -π a π para evitar estourar o limite numérico do float com o tempo
            while (player.rotate <= -M_PI) player.rotate += (2.0f * M_PI);
            while (player.rotate >  M_PI) player.rotate -= (2.0f * M_PI);
        }
    }
    
    if (!can_move) {
        move_x = 0.0f;
        move_z = 0.0f;
    }
    
    
    if (can_move) {
        if (keys[GLFW_KEY_SPACE] and player.jumping and player.active_character == 0 and player.double_jump_available) { // Aq q entra o double jump
            keys[GLFW_KEY_SPACE] = false;
            player.double_jump_available = false;
            player.speed.y = player.characters[player.active_character].jump_speed;

        }

        if (keys[GLFW_KEY_SPACE] and !player.jumping){
            keys[GLFW_KEY_SPACE] = false;
            player.jumping = true;
            player.speed.y = player.characters[player.active_character].jump_speed;
            player.double_jump_available = true;
        }
    }
    
    player.speed.y += gravidade * delta_t;

    // 1. Calcula o quanto o jogador QUER se mover neste frame (Equivalente ao m_velocity inicial)
    float move_vector_x = move_x * player.speed.x * delta_t;
    float move_vector_y = player.speed.y * delta_t; // O pulo/gravidade já definiu player.speed.y
    float move_vector_z = move_z * player.speed.z * delta_t;

    // Guarda os valores originais para verificar se houve colisão depois (Equivalente ao originalVector)
    float original_x = move_vector_x;
    float original_y = move_vector_y;
    float original_z = move_vector_z;

    // Referência rápida para a BBox do personagem ativo
    auto& player_bbox = player.characters[player.active_character].bbox;

    // === EIXO Y ===
    for (const auto& item : map) {
        // Reduz o move_vector_y se colidir com algo
        move_vector_y = player_bbox.GetClipY(item.bbox, move_vector_y);
        // printf("DeltaY: %f\n", move_vector_y);
        
    }
    for (int i = 0; i < MAX_BREAKABLES; ++i) {
        if (g_breakables[i].active) {
            move_vector_y = player_bbox.GetClipY(g_breakables[i].bbox, move_vector_y);
        }
    }
    // Move o jogador APENAS no Y antes de checar os outros eixos
    player.position.y += move_vector_y;
    // player_bbox.Move(0.0f, move_vector_y, 0.0f); 

    // === EIXO X ===
    for (const auto& item : map) {
        move_vector_x = player_bbox.GetClipX(item.bbox, move_vector_x);
        // printf("DeltaX: %f\n", move_vector_x);
    }
    for (int i = 0; i < MAX_BREAKABLES; ++i) {
        if (g_breakables[i].active) {
            move_vector_x = player_bbox.GetClipX(g_breakables[i].bbox, move_vector_x);
        }
    }
    player.position.x += move_vector_x;
    // player_bbox.Move(move_vector_x, 0.0f, 0.0f);

    // === EIXO Z ===
    for (const auto& item : map) {
        move_vector_z = player_bbox.GetClipZ(item.bbox, move_vector_z);
        // printf("DeltaZ: %f\n", move_vector_z);
    }
    for (int i = 0; i < MAX_BREAKABLES; ++i) {
        if (g_breakables[i].active) {
            move_vector_z = player_bbox.GetClipZ(g_breakables[i].bbox, move_vector_z);
        }
    }
    player.position.z += move_vector_z;
    // player_bbox.Move(0.0f, 0.0f, move_vector_z);


    // === ATUALIZAÇÃO DE ESTADOS PÓS-COLISÃO ===
    
    bool colidiu_com_chao = false;

    // 1. Checagem do "Chão de Segurança" (Hardcode)
    if (player.position.y <= -10.0f) {
        player.position.y = -10.0f;
        player.speed.y = 0.0f;
        colidiu_com_chao = true;
    }

    // 2. Zerar velocidades em caso de colisão (física real)
    if (move_vector_x != original_x) {
        // Bateu numa parede no eixo X
        // player.position.x -= (original_x - move_vector_x); // Reverte o movimento que não aconteceu
    }

    if (move_vector_y != original_y) {
        // Bateu no teto ou no chão
        player.speed.y = 0.0f; 
        
        // Se a tentativa original era cair (y negativo) e foi alterada, é porque bateu no chão
        if (original_y < 0.0f) {
            colidiu_com_chao = true;
        }
    }

    if (move_vector_z != original_z) {
        // Bateu numa parede no eixo Z
        // player.position.z -= (original_z - move_vector_z); // Reverte o movimento que não aconteceu
    }

    // 3. Atualiza as variáveis de estado baseadas na colisão final
    if (colidiu_com_chao) {
        player.jumping = false;
        player.double_jump_available = true; // Recarrega o pulo duplo
    } else {
        // Se ele não colidiu com o chão nem ativou o hardcode (-1.0f), está caindo
        player.jumping = true; 
    }

    glm::vec3 size = player.active_character == 0 ? bigchill_size 
               : (player.active_character == 1 ? swampfire_size : bentennyson_size);
    player_bbox = makeAABBFromGround(player.position, size);
}

void ProcessMeleeHitboxes(const SwampfireAnimResult& animRes, SwampfireAnimState& state, int restore_object_id) 
{
    if (!animRes.punch1_active && !animRes.punch2_active) return;

    glm::vec3 forward = glm::vec3(sin(player.rotate), 0.0f, cos(player.rotate));
    glm::vec3 hitbox_size = glm::vec3(0.8f, 0.5f, 0.8f);  // tune these
    float reach = 0.35f;
    float height = 0.5f;

    if (animRes.punch1_active) {
        glm::vec3 center = player.position + forward * reach + glm::vec3(0.0f, height, 0.0f);
        AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);
        DrawBoundingBox(punch_box, restore_object_id);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].visible) continue;
            if (g_enemies[i].is_dead) continue;
            if (state.punch1_hit_enemies.count(i)) continue;
            if (punch_box.Intersects(g_enemies[i].bbox)) {
                printf("Punch 1 hit enemy %d!\n", i);
                state.punch1_hit_enemies.insert(i);
                ApplyDamageToEnemy(i, 20.0f);
                glm::vec3 overlap_min = glm::max(punch_box.min, g_enemies[i].bbox.min);
                glm::vec3 overlap_max = glm::min(punch_box.max, g_enemies[i].bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts; popts.color = HexToRgb("#ffffff"); popts.life=0.3f; popts.scale=0.1f; popts.speed=3.0f; popts.count=15;
                Particles_Spawn(contact, popts);
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (state.punch1_hit_enemies.count(1000 + i)) continue; // offset to reuse set
            if (punch_box.Intersects(g_breakables[i].bbox)) {
                state.punch1_hit_enemies.insert(1000 + i);
                ApplyDamageToBreakable(i, 20.0f);
            }
        }
    }

    if (animRes.punch2_active) {
        glm::vec3 center = player.position + forward * reach + glm::vec3(0.0f, height, 0.0f);
        AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);
        DrawBoundingBox(punch_box, restore_object_id);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].visible) continue;
            if (g_enemies[i].is_dead) continue;
            if (state.punch2_hit_enemies.count(i)) continue;
            if (punch_box.Intersects(g_enemies[i].bbox)) {
                printf("Punch 2 hit enemy %d!\n", i);
                state.punch2_hit_enemies.insert(i);
                ApplyDamageToEnemy(i, 20.0f);
                glm::vec3 overlap_min = glm::max(punch_box.min, g_enemies[i].bbox.min);
                glm::vec3 overlap_max = glm::min(punch_box.max, g_enemies[i].bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts; popts.color = HexToRgb("#ffffff"); popts.life=0.3f; popts.scale=0.1f; popts.speed=3.0f; popts.count=15;
                Particles_Spawn(contact, popts);
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (state.punch2_hit_enemies.count(1000 + i)) continue;
            if (punch_box.Intersects(g_breakables[i].bbox)) {
                state.punch2_hit_enemies.insert(1000 + i);
                ApplyDamageToBreakable(i, 20.0f);
            }
        }
    }
}

void ProcessBigChillMeleeHitboxes(const BigChillAnimResult& animRes, BigChillAnimState& state, int restore_object_id) {
    if (!animRes.punch_active && !animRes.magic_active) return;

    glm::vec3 forward(sin(player.rotate), 0.0f, cos(player.rotate));
    float reach = 0.4f;
    float height = 0.5f;

    if (animRes.punch_active) {
        glm::vec3 center = player.position + forward * reach + glm::vec3(0.0f, height, 0.0f);
        glm::vec3 hitbox_size = glm::vec3(0.8f, 0.5f, 0.8f);
        AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);
        DrawBoundingBox(punch_box, restore_object_id);

        for (int i = 0; i < 20; i++) {
            if (!g_enemies[i].visible || g_enemies[i].is_dead) continue;
            if (state.punch_hit_enemies.count(i)) continue;

            if (punch_box.Intersects(g_enemies[i].bbox)) {
                state.punch_hit_enemies.insert(i);
                ApplyDamageToEnemy(i, 20.0f);
                glm::vec3 overlap_min = glm::max(punch_box.min, g_enemies[i].bbox.min);
                glm::vec3 overlap_max = glm::min(punch_box.max, g_enemies[i].bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts; popts.color = HexToRgb("#ffffff"); popts.life=0.3f; popts.scale=0.1f; popts.speed=3.0f; popts.count=15;
                Particles_Spawn(contact, popts);
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (state.punch_hit_enemies.count(1000 + i)) continue;
            if (punch_box.Intersects(g_breakables[i].bbox)) {
                state.punch_hit_enemies.insert(1000 + i);
                ApplyDamageToBreakable(i, 20.0f);
            }
        }
    }

    if (animRes.magic_active) {
        // Lower the center
        glm::vec3 center = player.position + forward * 0.8f + glm::vec3(0.0f, 0.5f, 0.0f);
        
        // Calculate dynamic AABB extents based on rotation
        float local_x = 0.4f; // Half of 0.8 width
        float local_y = 0.5f; // Half of 1.0 height
        float local_z = 0.75f; // Half of 1.5 length
        
        float abs_sin = std::abs(sin(player.rotate));
        float abs_cos = std::abs(cos(player.rotate));
        
        float world_x = local_x * abs_cos + local_z * abs_sin;
        float world_z = local_x * abs_sin + local_z * abs_cos;
        
        glm::vec3 dynamic_hitbox_size(world_x * 2.0f, local_y * 2.0f, world_z * 2.0f);
        
        AABB magic_box = MakeAABBFromCenterSize(center, dynamic_hitbox_size);
        DrawBoundingBox(magic_box, restore_object_id);

        for (int i = 0; i < 20; i++) {
            if (!g_enemies[i].visible || g_enemies[i].is_dead) continue;
            if (magic_box.Intersects(g_enemies[i].bbox)) {
                g_enemies[i].is_frozen = true;
                g_enemies[i].frozen_timer = 3.0f;
                // continuous low damage without triggering flinch
                ApplyDamageToEnemy(i, 15.0f * delta_t, false); 
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (magic_box.Intersects(g_breakables[i].bbox)) {
                ApplyDamageToBreakable(i, 15.0f * delta_t); 
            }
        }
    }
}

void ResolvePlayerMapCollisions() {
    auto& bbox = player.characters[player.active_character].bbox;
    for (const auto& item : map) {
        if (bbox.Intersects(item.bbox)) {
            glm::vec3 centerP = (bbox.min + bbox.max) * 0.5f;
            glm::vec3 centerM = (item.bbox.min + item.bbox.max) * 0.5f;
            glm::vec3 extP = (bbox.max - bbox.min) * 0.5f;
            glm::vec3 extM = (item.bbox.max - item.bbox.min) * 0.5f;
            
            float dx = centerP.x - centerM.x;
            float dz = centerP.z - centerM.z;
            
            float px = (extP.x + extM.x) - std::abs(dx);
            float pz = (extP.z + extM.z) - std::abs(dz);
            
            if (px < pz) {
                if (dx > 0) player.position.x += px + 0.001f;
                else        player.position.x -= px + 0.001f;
            } else {
                if (dz > 0) player.position.z += pz + 0.001f;
                else        player.position.z -= pz + 0.001f;
            }
            
            glm::vec3 size = player.active_character == 0 ? bigchill_size : (player.active_character == 1 ? swampfire_size : bentennyson_size);
            bbox = makeAABBFromGround(player.position, size);
        }
    }
}

void ProcessBenMeleeHitboxes(const BenAnimResult& animRes, BenAnimState& state, int restore_object_id) 
{
    if (!animRes.punch_active) return;

    glm::vec3 forward = glm::vec3(sin(player.rotate), 0.0f, cos(player.rotate));
    glm::vec3 hitbox_size = glm::vec3(0.8f, 0.5f, 0.8f);  // tune these
    float reach = 0.35f;
    float height = 0.5f;

    glm::vec3 center = player.position + forward * reach + glm::vec3(0.0f, height, 0.0f);
    AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);
    DrawBoundingBox(punch_box, restore_object_id);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) continue;
        if (g_enemies[i].is_dead) continue;
        if (state.punch_hit_enemies.count(i)) continue;
        if (punch_box.Intersects(g_enemies[i].bbox)) {
            printf("Ben punch hit enemy %d!\n", i);
            state.punch_hit_enemies.insert(i);
            ApplyDamageToEnemy(i, 5.0f);
            glm::vec3 overlap_min = glm::max(punch_box.min, g_enemies[i].bbox.min);
            glm::vec3 overlap_max = glm::min(punch_box.max, g_enemies[i].bbox.max);
            glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
            ParticleOptions popts; popts.color = HexToRgb("#ffffff"); popts.life=0.3f; popts.scale=0.1f; popts.speed=3.0f; popts.count=10;
            Particles_Spawn(contact, popts);
            state.attack_speed_multiplier *= 2.0f; // Increase speed on hit
            if (state.attack_speed_multiplier > 6.0f) {
                state.attack_speed_multiplier = 6.0f;
            }
        }
    }
    for (int i = 0; i < MAX_BREAKABLES; i++) {
        if (!g_breakables[i].active) continue;
        if (state.punch_hit_enemies.count(1000 + i)) continue;
        if (punch_box.Intersects(g_breakables[i].bbox)) {
            state.punch_hit_enemies.insert(1000 + i);
            ApplyDamageToBreakable(i, 5.0f);
        }
    }
}