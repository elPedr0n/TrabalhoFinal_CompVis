// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <string>


#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

#define MAX_PLATFORMS 1
#define MAX_ENEMIES 1
#define MAX_CHARACTERS 2

#include "structs.h"
// BIGCHILL Dimensoes -> Largura(X): 0.695, Altura(Y): 0.985, Profund(Z): 0.225

extern struct Enemy g_enemies[MAX_ENEMIES]; // Array with the current enemies

extern struct MapItem map[MAX_PLATFORMS];

#include "map.h"

extern bool keys[1024];

extern struct Player player;

extern float gravidade;

extern float delta_t;

extern float g_CameraTheta; 
extern float g_CameraPhi;   
extern float g_CameraDistance; 

extern glm::vec3 bigchill_size;
extern glm::vec3 swampfire_size;

#endif
