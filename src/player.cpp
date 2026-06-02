#include <glad/glad.h>  
#include <GLFW/glfw3.h>
#include "globals.h"
#include <cmath>
#include <algorithm>
#include <vector>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

void UpdatePosition() {

    float input_x = 0.0f;
    float input_z = 0.0f;

    if (keys[GLFW_KEY_A]) input_x -= 1.0f;
    if (keys[GLFW_KEY_D]) input_x += 1.0f;
    if (keys[GLFW_KEY_S]) input_z += 1.0f;
    if (keys[GLFW_KEY_W]) input_z -= 1.0f;

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
    
    
    if (keys[GLFW_KEY_SPACE] and player.jumping and player.active_character == 0 and player.double_jump_available) { // Aq q entra o double jump
        keys[GLFW_KEY_SPACE] = false;
        player.double_jump_available = false;
        player.speed.y = player.jump_speed;

    }

    if (keys[GLFW_KEY_SPACE] and !player.jumping){
        keys[GLFW_KEY_SPACE] = false;
        player.jumping = true;
        player.speed.y = player.jump_speed;
        player.double_jump_available = true;
    }
    
    
    // ... [Seu código de rotação e pulo (gravidade) até aqui permanece igual] ...
    
    if (player.jumping) player.speed.y += gravidade;

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
    // Move o jogador APENAS no Y antes de checar os outros eixos
    player.position.y += move_vector_y;
    player_bbox.Move(0.0f, move_vector_y, 0.0f); 

    // === EIXO X ===
    for (const auto& item : map) {
        move_vector_x = player_bbox.GetClipX(item.bbox, move_vector_x);
        // printf("DeltaX: %f\n", move_vector_x);
    }
    player.position.x += move_vector_x;
    player_bbox.Move(move_vector_x, 0.0f, 0.0f);

    // === EIXO Z ===
    for (const auto& item : map) {
        move_vector_z = player_bbox.GetClipZ(item.bbox, move_vector_z);
        // printf("DeltaZ: %f\n", move_vector_z);
    }
    player.position.z += move_vector_z;
    player_bbox.Move(0.0f, 0.0f, move_vector_z);


    // === ATUALIZAÇÃO DE ESTADOS PÓS-COLISÃO ===
    
    bool colidiu_com_chao = false;

    // 1. Checagem do "Chão de Segurança" (Hardcode)
    if (player.position.y <= -1.0f) {
        player.position.y = -1.0f;
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
}