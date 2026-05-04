#include <glad/glad.h>  
#include <GLFW/glfw3.h>
#include "globals.h"
#include <bits/stdc++.h>

#define X 0
#define Y 1
#define Z 2

void UpdatePosition() {

    //Inputs do jogador
    if (keys[GLFW_KEY_A]) {
        player_pos[X] -= player_speed_h * delta_t;
    }
    if (keys[GLFW_KEY_D]) {
        player_pos[X] += player_speed_h * delta_t;
    }
    if (keys[GLFW_KEY_S]) {
        player_pos[Z] += player_speed_h * delta_t;
    }
    if (keys[GLFW_KEY_W]) {
        player_pos[Z] -= player_speed_h * delta_t;
    }
    if (keys[GLFW_KEY_SPACE] and !jumping){
        keys[GLFW_KEY_SPACE] = false;
        std::cout << "entremo aq\n";
        jumping = true;
        player_speed_v = jump_speed;
    }

    player_speed_v += gravidade;
    player_pos[Y] += player_speed_v * delta_t;


    if (player_pos[Y] < -1) { //Aqui defini o chao como 1
        player_pos[Y] = -1;
        player_speed_v = 0;
        jumping = false;
    }
    
}