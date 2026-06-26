#include <glad/glad.h>  
#include <GLFW/glfw3.h>
#include "globals.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include "animation.h"
#include "breakables.h"
#include "particles.h"
#include "sound.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

void ResolvePlayerMapCollisions();


void UpdatePosition(bool can_move, bool can_rotate = false) {

    float input_x = 0.0f;
    float input_z = 0.0f;

    if (can_move || can_rotate) {
        if (keys[GLFW_KEY_A]) input_x -= 1.0f;
        if (keys[GLFW_KEY_D]) input_x += 1.0f;
        if (keys[GLFW_KEY_S]) input_z += 1.0f;
        if (keys[GLFW_KEY_W]) input_z -= 1.0f;
    }

    g_IsMovementBuffered = (input_x != 0.0f || input_z != 0.0f);

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
            PlayJumpSound("../../data/sounds/jump.wav");
        }

        if (keys[GLFW_KEY_SPACE] and !player.jumping){
            keys[GLFW_KEY_SPACE] = false;
            player.jumping = true;
            player.speed.y = player.characters[player.active_character].jump_speed;
            player.double_jump_available = true;
            PlayJumpSound("../../data/sounds/jump.wav");
            if (player.active_character == 0) {
                PlaySoundEffect("../../data/sounds/big_chill_cloak.wav");
            }
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
    move_vector_y = CheckMapCollisionY(player_bbox, move_vector_y);
    move_vector_y = CheckBreakablesCollisionY(player_bbox, move_vector_y);
    // Move o jogador APENAS no Y antes de checar os outros eixos
    player.position.y += move_vector_y;
    // player_bbox.Move(0.0f, move_vector_y, 0.0f); 

    // === EIXO X ===
    move_vector_x = CheckMapCollisionX(player_bbox, move_vector_x);
    move_vector_x = CheckBreakablesCollisionX(player_bbox, move_vector_x);
    player.position.x += move_vector_x;
    // player_bbox.Move(move_vector_x, 0.0f, 0.0f);

    // === EIXO Z ===
    move_vector_z = CheckMapCollisionZ(player_bbox, move_vector_z);
    move_vector_z = CheckBreakablesCollisionZ(player_bbox, move_vector_z);
    player.position.z += move_vector_z;
    // player_bbox.Move(0.0f, 0.0f, move_vector_z);


    // === ATUALIZAÇÃO DE ESTADOS PÓS-COLISÃO ===
    
    bool colidiu_com_chao = false;

    // 1. Checagem do "Chão de Segurança" (Hardcode)
    if (player.position.y <= -2.0f) {
        player.position.y = -2.0f;
        player.speed.y = 0.0f;
        colidiu_com_chao = true;
        
        if (!player.is_dead) {
            player.health = 0.0f;
            player.is_dead = true;
            player.death_timer = 0.0f;
            
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
        }
    }

    if (move_vector_y != original_y) {
        // Bateu no teto ou no chão
        player.speed.y = 0.0f; 
        
        // Se a tentativa original era cair (y negativo) e foi alterada, é porque bateu no chão
        if (original_y < 0.0f) {
            colidiu_com_chao = true;
        }
    }

    // 3. Atualiza as variáveis de estado baseadas na colisão final
    if (colidiu_com_chao) {
        if (player.jumping) {
            PlaySoundEffect("../../data/sounds/step.wav");
            if (player.active_character == 0) {
                PlaySoundEffect("../../data/sounds/big_chill_cloak.wav");
            }
        }
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

void ApplyDamageToPlayer(float base_damage, glm::vec3 damage_source_pos) {
    if (player.is_dead) return;

    glm::vec3 knockback_dir = player.position - damage_source_pos;
    knockback_dir.y = 0.0f;
    if (glm::length(knockback_dir) > 0.001f) {
        knockback_dir = glm::normalize(knockback_dir);
    } else {
        knockback_dir = glm::vec3(0,0,1);
    }
    player.position += knockback_dir * 0.05f; // small knockback
    ResolvePlayerMapCollisions();

    // Rotate player to face the damage source
    glm::vec3 look_dir = damage_source_pos - player.position;
    player.rotate = atan2(look_dir.x, look_dir.z);

    float defense = player.active_character == 0 ? 0.5f : (player.active_character == 1 ? 0.4f : 1.0f);
    float actual_damage = base_damage * defense;
    player.health -= actual_damage;
    
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