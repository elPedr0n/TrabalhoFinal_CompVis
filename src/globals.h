// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <string>

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

// Character struct — extend this for new properties
struct Character {
	std::string model_name;
	float pos[3];
	float rotate;
	float scale;
	bool  visible;

	Character(const char* name, float px, float py, float pz, float rot, float sc, bool vis)
		: model_name(name), rotate(rot), scale(sc), visible(vis)
	{
		pos[0] = px; pos[1] = py; pos[2] = pz;
	}
};

extern Character g_characters[2];  // 0 = BigChill, 1 = Swampfire
extern int       g_active_character;

extern bool keys[1024];
extern bool jumping;

extern float player_pos[3];
extern float player_speed[3];
extern float player_rotate;
extern float player_scalling;
extern float jump_speed;

extern float gravidade;

extern float delta_t;

#endif
