#include <glad/glad.h>  
#include <GLFW/glfw3.h>
#include "globals.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

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
    
    
    if (player.jumping) player.speed.y += gravidade;
    
    player.position.y += player.speed.y * delta_t;
    player.position.x += move_x * player.speed.x * delta_t;
    player.position.z += move_z * player.speed.z * delta_t;
    
    
    // === INÍCIO DO SISTEMA DE COLISÃO ===
    
    bool colidiu_com_chao = false; // Flag para saber se podemos pular

    // 1. Checagem do "Chão de Segurança" (O que você já tinha)
    if (player.position.y <= -1.0f) {
        player.position.y = -1.0f;
        player.speed.y = 0.0f;
        colidiu_com_chao = true;
    }

    for (auto item : map) {

    }



    // 3. Atualiza as suas variáveis de estado baseadas na colisão
    if (colidiu_com_chao) {
        player.jumping = false;
        player.double_jump_available = true; // Recarrega o pulo duplo
    } else {
        // Se ele não colidiu com o chão nem com plataforma, ele está caindo
        // (por exemplo, se ele andou pra fora da borda de uma plataforma sem pular)
        player.jumping = true; 
    }
    
}