// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <glm/glm.hpp>
#include <string>


#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

#define MAX_PLATFORMS 150
#define MAX_ENEMIES 50
#define MAX_CHARACTERS 3
#define MAX_COLLECTIBLES 100
#define MAX_SPAWN_POINTS 50

extern float bigchill_jump_speed;
extern float swampfire_jump_speed;
extern float bentennyson_jump_speed;

#include "structs.h"
#include "animation.h"

extern struct Enemy g_enemies[MAX_ENEMIES]; 

extern struct SpawnPoint g_spawn_points[MAX_SPAWN_POINTS];
extern int g_num_spawn_points;

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

extern float g_MovementTheta;
extern bool g_UseFixedCameras;
extern bool g_CameraJustSwitched;
extern bool g_IsMovementBuffered; 

extern glm::vec3 bigchill_size;
extern glm::vec3 swampfire_size;
extern glm::vec3 bentennyson_size;

void ApplyDamageToEnemy(int enemy_id, float damage, bool cause_flinch = true);
void ApplyDamageToPlayer(float base_damage, glm::vec3 damage_source_pos);
void SpawnEnemy(glm::vec3 pos, int spawner_id = -1);
void SpawnRangedEnemy(glm::vec3 pos, int spawner_id = -1);
void DrawBoundingBox(struct AABB& aabb, int restore_object_id);

void ProcessEnemyMeleeHitboxes();
bool ProcessSwampfireMeleeHitboxes(const SwampfireAnimResult& animRes, SwampfireAnimState& state, int restore_object_id, bool just_triggered);
bool ProcessBigChillMeleeHitboxes(const BigChillAnimResult& animRes, BigChillAnimState& state, int restore_object_id, bool just_triggered);
bool ProcessBenMeleeHitboxes(const BenAnimResult& animRes, BenAnimState& state, int restore_object_id, bool just_triggered);

void ResolvePlayerMapCollisions();
float CheckMapCollisionX(AABB bbox, float move_x);
float CheckMapCollisionY(AABB bbox, float move_y);
float CheckMapCollisionZ(AABB bbox, float move_z);
float CheckBreakablesCollisionX(AABB bbox, float move_x, int ignore_id = -1);
float CheckBreakablesCollisionY(AABB bbox, float move_y, int ignore_id = -1);
float CheckBreakablesCollisionZ(AABB bbox, float move_z, int ignore_id = -1);

#endif
