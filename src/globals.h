// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#define X 0
#define Y 1
#define Z 2

extern bool keys[1024];
extern bool jumping;

extern float player_pos[3];
extern float player_speed[3];
extern float player_rotate;
extern float jump_speed;

extern float gravidade;

extern float delta_t;

extern float g_CameraTheta; 
extern float g_CameraPhi;   
extern float g_CameraDistance; 

#endif
