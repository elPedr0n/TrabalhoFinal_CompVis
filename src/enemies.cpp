#include "globals.h"
#include <iostream>

void UpdateEnemies() {

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) continue;

        // Movimentação, persegue o jogador
        glm::vec3 direction_to_player = glm::vec3(
            player_pos[AXIS_X] - g_enemies[i].position.x,
            0.0f, // Ignora diferença vertical para manter no chão
            player_pos[AXIS_Z] - g_enemies[i].position.z
        );

        if (glm::distance(g_enemies[i].position, glm::vec3(player_pos[AXIS_X], g_enemies[i].position.y, player_pos[AXIS_Z])) > 1.0f) {
            direction_to_player = glm::normalize(direction_to_player);
            g_enemies[i].position.x += direction_to_player.x * g_enemies[i].speed * delta_t;
            g_enemies[i].position.z += direction_to_player.z * g_enemies[i].speed * delta_t;
            // std::cout << "Inimigo " << i << " esta no range do jogador" << std::endl;
        } 

    }

}