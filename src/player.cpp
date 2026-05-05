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
        player_pos[X] -= player_speed[X] * delta_t;
    }
    if (keys[GLFW_KEY_D]) {
        player_pos[X] += player_speed[X] * delta_t;
    }
    if (keys[GLFW_KEY_S]) {
        player_pos[Z] += player_speed[Z] * delta_t;
    }
    if (keys[GLFW_KEY_W]) {
        player_pos[Z] -= player_speed[Z]* delta_t;
    }
    if (keys[GLFW_KEY_SPACE] and !jumping){
        keys[GLFW_KEY_SPACE] = false;
        jumping = true;
        player_speed[Y] = jump_speed;
    }
    std::cout << delta_t << "\n";

    player_speed[Y] += gravidade;
    player_pos[Y] += player_speed[Y] * delta_t;


    if (player_pos[Y] < -1) { //Aqui defini o chao como 1
        player_pos[Y] = -1;
        player_speed[Y] = 0;
        jumping = false;
    }
    
}