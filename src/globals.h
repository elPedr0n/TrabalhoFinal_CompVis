// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <string>

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

#define MAX_PLATFORMS 3

// Character struct — extend this for new properties
struct Character {
	std::string model_name;
	float pos[3];
	float rotate;
	float scale;
	bool  visible;
    float bbox[3]; // Bounding box dimensions (width, height, depth)

	Character(const char* name, float px, float py, float pz, float rot, float sc, bool vis, float bbox_w, float bbox_h, float bbox_d)
		: model_name(name), rotate(rot), scale(sc), visible(vis)
	{
		pos[0] = px; pos[1] = py; pos[2] = pz;
		bbox[0] = bbox_w;
		bbox[1] = bbox_h;
		bbox[2] = bbox_d;
	}
};

// BIGCHILL Dimensoes -> Largura(X): 0.695, Altura(Y): 0.985, Profund(Z): 0.225

extern Character g_characters[2];  // 0 = BigChill, 1 = Swampfire
extern int       g_active_character;

struct Platform {
    glm::vec3 position; // Centro da plataforma
    glm::vec3 scale;    // Tamanho (Largura, Altura, Profundidade)
};

extern Platform g_platforms[MAX_PLATFORMS]; // Array com as plataformas atuais 


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
