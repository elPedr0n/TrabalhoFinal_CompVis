// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <string>

#include "structs.h"

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

#define MAX_PLATFORMS 3

#define MAX_ENEMIES 1

// BIGCHILL Dimensoes -> Largura(X): 0.695, Altura(Y): 0.985, Profund(Z): 0.225

extern Character g_characters[2];  // 0 = BigChill, 1 = Swampfire
extern int       g_active_character;

extern Enemy g_enemies[MAX_ENEMIES]; // Array with the current enemies

extern bool keys[1024];
extern bool jumping;
extern bool double_jump_available;

extern float player_pos[3];
extern float player_speed[3];
extern float player_rotate;
extern float player_scalling;
extern float jump_speed;

extern float gravidade;

extern float delta_t;

extern float g_CameraTheta; 
extern float g_CameraPhi;   
extern float g_CameraDistance; 

#endif
