#include <glad/glad.h>  
#include <GLFW/glfw3.h>
#include "globals.h"
#include <bits/stdc++.h>

bool CheckCollisionAABB(glm::vec3 posA, glm::vec3 scaleA, glm::vec3 posB, glm::vec3 scaleB);

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
        float diff = target_angle - player_rotate;

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
            player_rotate = target_angle;
        } else {
            // Caso contrário, gira suavemente
            player_rotate += step;
        }

        // Mantém o ângulo final dentro de -π a π para evitar estourar o limite numérico do float com o tempo
        while (player_rotate <= -M_PI) player_rotate += (2.0f * M_PI);
        while (player_rotate >  M_PI) player_rotate -= (2.0f * M_PI);
    }
    
    
    if (keys[GLFW_KEY_SPACE] and jumping and g_active_character == 0 and double_jump_available) { // Aq q entra o double jump
        keys[GLFW_KEY_SPACE] = false;
        double_jump_available = false;
        player_speed[AXIS_Y] = jump_speed;

    }

    if (keys[GLFW_KEY_SPACE] and !jumping){
        keys[GLFW_KEY_SPACE] = false;
        jumping = true;
        player_speed[AXIS_Y] = jump_speed;
        double_jump_available = true;
    }


    if (jumping) player_speed[AXIS_Y] += gravidade;

    player_pos[AXIS_Y] += player_speed[AXIS_Y] * delta_t;
    player_pos[AXIS_X] += move_x * player_speed[AXIS_X] * delta_t;
    player_pos[AXIS_Z] += move_z * player_speed[AXIS_Z] * delta_t;

    // === INÍCIO DO SISTEMA DE COLISÃO ===
    
    bool colidiu_com_chao = false; // Flag para saber se podemos pular

    // 1. Checagem do "Chão de Segurança" (O que você já tinha)
    if (player_pos[AXIS_Y] <= -1.0f) {
        player_pos[AXIS_Y] = -1.0f;
        player_speed[AXIS_Y] = 0.0f;
        colidiu_com_chao = true;
    }

    // 2. Checagem das Plataformas
    // Crie as caixas para o teste (substitua p_scale pelos valores da bbox do seu modelo)
    glm::vec3 p_pos(player_pos[AXIS_X], player_pos[AXIS_Y], player_pos[AXIS_Z]);
    glm::vec3 p_scale(g_characters[g_active_character].bbox[0], g_characters[g_active_character].bbox[1], g_characters[g_active_character].bbox[2]);

    // p_pos.y -= 0.9;
    // Supondo que você tem um vetor global std::vector<Platform> level_platforms;
    for (const auto& plat : g_platforms) {
        if (CheckCollisionAABB(p_pos, p_scale, plat.position, plat.scale)) {
            
            // std::cout << "Colidiu com a plataforma!" << std::endl;
            // O jogador só "pisa" na plataforma se ele estiver CAINDO.
            // Se ele estiver subindo (speed > 0), ele passa direto (ajuda na fluidez do pulo)
            if (player_speed[AXIS_Y] <= 0.0f) {
                
                // std::cout << player_speed[AXIS_Y] <<" Colidiu com a plataforma e esta descendo!" << std::endl;
                // Trava o Y do jogador EXATAMENTE no topo da plataforma
                // Topo da plataforma = Centro Y dela + Metade da Altura dela
                player_pos[AXIS_Y] = plat.position.y + (plat.scale.y); // Ajusta para o topo da plataforma
                
                player_speed[AXIS_Y] = 0.0f; // Zera a velocidade de queda
                colidiu_com_chao = true;     // Avisa que o jogador está pisando em algo
            }
        }
    }

    // 3. Atualiza as suas variáveis de estado baseadas na colisão
    if (colidiu_com_chao) {
        jumping = false;
        double_jump_available = true; // Recarrega o pulo duplo
    } else {
        // Se ele não colidiu com o chão nem com plataforma, ele está caindo
        // (por exemplo, se ele andou pra fora da borda de uma plataforma sem pular)
        jumping = true; 
    }
    
}