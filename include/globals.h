// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <string>


#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

#define MAX_PLATFORMS 150
#define MAX_ENEMIES 100
#define MAX_CHARACTERS 3
#define MAX_COLLECTIBLES 100

extern float bigchill_jump_speed;
extern float swampfire_jump_speed;
extern float bentennyson_jump_speed;

#include "structs.h"
#include "animation.h"

// BIGCHILL Dimensoes -> Largura(X): 0.695, Altura(Y): 0.985, Profund(Z): 0.225

extern struct Enemy g_enemies[MAX_ENEMIES]; // Array with the current enemies

extern struct MapItem map[MAX_PLATFORMS];
extern int g_num_platforms;

extern struct Collectible g_collectibles[MAX_COLLECTIBLES];
void SpawnCollectibles(glm::vec3 pos, int count, int specific_type = -1);
void UpdateCollectibles();

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
extern glm::vec3 bentennyson_size;

void ApplyDamageToEnemy(int enemy_id, float damage, bool cause_flinch = true);
void ApplyDamageToPlayer(float base_damage, glm::vec3 damage_source_pos);
void SpawnEnemy(glm::vec3 pos);
void SpawnRangedEnemy(glm::vec3 pos);
void DrawBoundingBox(struct AABB& aabb, int restore_object_id);

void ProcessEnemyMeleeHitboxes();
bool ProcessSwampfireMeleeHitboxes(const SwampfireAnimResult& animRes, SwampfireAnimState& state, int restore_object_id, bool just_triggered);
bool ProcessBigChillMeleeHitboxes(const BigChillAnimResult& animRes, BigChillAnimState& state, int restore_object_id, bool just_triggered);
bool ProcessBenMeleeHitboxes(const BenAnimResult& animRes, BenAnimState& state, int restore_object_id, bool just_triggered);

#endif
