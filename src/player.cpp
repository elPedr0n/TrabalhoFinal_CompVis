#include <glad/glad.h>  
#include <GLFW/glfw3.h>
#include "globals.h"
#include <bits/stdc++.h>

void UpdatePosition() {

    float move_x = 0.0f;
    float move_z = 0.0f;

    //Inputs do jogador
    if (keys[GLFW_KEY_A]) move_x -= 1.0f;
    if (keys[GLFW_KEY_D]) move_x += 1.0f;
    if (keys[GLFW_KEY_S]) move_z += 1.0f;
    if (keys[GLFW_KEY_W]) move_z -= 1.0f;

    if (move_x != 0.0f || move_z != 0.0f) {
        float target_angle = atan2(move_x, move_z);

        // Converte para graus se seu motor/matriz usar graus
        player_rotate = target_angle * (180.0f / M_PI);
    }
    
    
    if (keys[GLFW_KEY_SPACE] and !jumping){
        keys[GLFW_KEY_SPACE] = false;
        jumping = true;
        player_speed[Y] = jump_speed;
    }
    
    player_speed[Y] += gravidade;
    player_pos[Y] += player_speed[Y] * delta_t;
    
    player_pos[X] += move_x * player_speed[X] * delta_t;
    player_pos[Z] += move_z * player_speed[Z] * delta_t;

    if (player_pos[Y] < -1) { //Aqui defini o chao como -1
        player_pos[Y] = -1;
        player_speed[Y] = 0;
        jumping = false;
    }
    
}