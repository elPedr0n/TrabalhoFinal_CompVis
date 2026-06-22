#ifndef STRUCTS_H
#define STRUCTS_H

#include "globals.h"

// FONTE: https://medium.com/@andrebluntindie/3d-aabb-collision-detection-and-resolution-for-voxel-games-5fcbfdb8cdb4
struct AABB {
	glm::vec3 min; // Vértice mínimo (canto inferior esquerdo)
	glm::vec3 max; // Vértice máximo (canto superior direito)

	AABB() : min(0.0f), max(0.0f) {}
	AABB(const glm::vec3& min_point, const glm::vec3& max_point)
		: min(min_point), max(max_point) {}

	// Criar uma regiao de colisao a partir dos pontos min e max e uma translacao
	AABB(const glm::vec3& position, const glm::vec3& min_point, const glm::vec3& max_point)
		: min(position + min_point), max(position + max_point) {}

	public:
	bool IntersectsX(AABB other) {
		return min.x < other.max.x && max.x > other.min.x;
	}

	bool IntersectsY(AABB other) {
		return min.y < other.max.y && max.y > other.min.y;
	}

	bool IntersectsZ(AABB other) {
		return min.z < other.max.z && max.z > other.min.z;
	}
	
	bool Intersects(AABB other) {
		return IntersectsX(other) && IntersectsY(other) && IntersectsZ(other);
	}


	void Move(float valueX, float valueY, float valueZ) {
		min.x += valueX;
		max.x += valueX;
		min.y += valueY;
		max.y += valueY;
		min.z += valueZ;
		max.z += valueZ;
	}

	float GetClipX(AABB against, float deltaX) {
		//are we overlapping the other axes?
		//(if we aren't, then an intersection could never actually take place)
		if(IntersectsY(against) && IntersectsZ(against)) {
			//if we are moving right and our right bounds are smaller than
			//or equal to the other left bounds
			if(deltaX > 0 && max.x <= against.min.x + 0.002f) {
				//what is the distance to the other AABB?
				float clip = against.min.x - max.x;
				//if our move delta is larger than the distance to
				//the other AABB, set the move delta that distance
				if (deltaX > clip)
					deltaX = clip;
			}
			//the principle explained in the code above is the same for
			//everything else
			if (deltaX < 0 && min.x >= against.max.x - 0.002f) {
				float clip = against.max.x - min.x;
				if (deltaX < clip)
					deltaX = clip;
			}
			return deltaX;
		}
		return deltaX;
	}

	float GetClipY(AABB against, float deltaY) {
		if (IntersectsX(against) && IntersectsZ(against)) {
			if (deltaY > 0 && max.y <= against.min.y + 0.002f) {
				float clip = against.min.y - max.y;
				if (deltaY > clip)
					deltaY = clip;
			}
			if (deltaY < 0 && min.y >= against.max.y - 0.002f) {
				float clip = against.max.y - min.y;
				if (deltaY < clip)
					deltaY = clip;
			}
			return deltaY;
		}
		return deltaY;
	}

	float GetClipZ(AABB against, float deltaZ) {
		if (IntersectsX(against) && IntersectsY(against)) {
			if (deltaZ > 0 && max.z <= against.min.z + 0.002f) {
				float clip = against.min.z - max.z;
				if (deltaZ > clip)
					deltaZ = clip;
			}
			if (deltaZ < 0 && min.z >= against.max.z - 0.002f) {
				float clip = against.max.z - min.z;
				if (deltaZ < clip)
					deltaZ = clip;
			}
			return deltaZ;
		}
		return deltaZ;
	}

};

inline AABB MakeAABBFromCenterSize(const glm::vec3& center, const glm::vec3& size)
{
	glm::vec3 half = size * 0.5f;
	return AABB(center - half, center + half);
}

inline AABB makeAABBFromGround(const glm::vec3& position, const glm::vec3& size)
{
	glm::vec3 half = size * 0.5f;
	return AABB(glm::vec3(position.x - half.x, position.y, position.z - half.z),
				glm::vec3(position.x + half.x, position.y + size.y, position.z + half.z));
}


// Character struct — extend this for new properties
struct Character {
	std::string model_name;
	float scale;
    AABB bbox;
	float jump_speed;

	Character() : model_name(""), scale(0.5f), bbox(), jump_speed(6.0f) {} 

	Character(const char* name, glm::vec3 pos, float bbox_w, float bbox_h, float bbox_d, float scale, float jump)
		: model_name(name), scale(scale), bbox(makeAABBFromGround(pos, glm::vec3(bbox_w * scale, bbox_h * scale, bbox_d * scale))), jump_speed(jump) {}
};

struct AttackUI {
    std::string text;
    float timer;
    float x_offset;
    bool active;
    AttackUI() : text(""), timer(0.0f), x_offset(0.0f), active(false) {}
};

struct Player {
	int active_character; // 0 = SWAMPFIRE, 1 = BIGCHILL, 2 = BENTENNYSON
	int enemies_slain;
	int objects_destroyed;
	bool has_won;
	float start_time;
	float final_time;

	Character characters[MAX_CHARACTERS];

	glm::vec3 position;
	glm::vec3 speed;
	
	float rotate;
	float scale;

	bool jumping;
	bool double_jump_available;

	float health;
	float max_health;
	bool is_dead;
	float death_timer;
	bool is_flinching;
	float flinch_timer;

	float transform_energy;
	float max_transform_energy;
	float special_energy;
	float max_special_energy;
	int selected_alien;

	AttackUI recent_attack;
	AttackUI previous_attack;

	void pushAttack(const std::string& name) {
		if (recent_attack.active) {
			previous_attack = recent_attack;
		}
		recent_attack.text = name;
		recent_attack.timer = 0.0f;
		recent_attack.x_offset = 0.0f;
		recent_attack.active = true;
	}

	Player()
		: active_character(2), position(1.0f, 0.0f, -7.0f), speed(3.0f, 0.0f, 3.0f), rotate(0.0f), scale(1.0f), jumping(false), double_jump_available(false), health(100.0f), max_health(100.0f), is_dead(false), death_timer(0.0f), is_flinching(false), flinch_timer(0.0f), transform_energy(100.0f), max_transform_energy(100.0f), special_energy(100.0f), max_special_energy(100.0f), selected_alien(0)
	{
		characters[0] = Character("the_bigchill", position, 1.38963f, 1.96548f, 0.454046f, 0.5f, bigchill_jump_speed); // Calafrio pula mais alto
		characters[1] = Character("the_swampfire", position, 3.28f, 3.8f, 2.0f, 0.3f, swampfire_jump_speed); // Fogo Fátuo pula mais baixo
		characters[2] = Character("the_bentennyson", position, 1.38f, 1.5f, 0.27f, 0.6f, bentennyson_jump_speed); // Ben salto médio

		enemies_slain = 0;
		objects_destroyed = 0;
		has_won = false;
		start_time = 0.0f;
		final_time = 0.0f;
	}
};

struct Enemy {
	glm::vec3 position;
	float rotate;
	float scale;
	bool visible;
	AABB bbox;
	float speed; // Velocidade de movimento do inimigo

	// Attack state
	bool is_attacking;
	float attack_timer;
	float attack_cooldown;
	float attack_duration;
	float attack_range;
	bool has_hit_player;     // single-hit per attack
	bool punch_active;       // hitbox active this frame

	// Health and Damage state
	float health;
	float max_health;
	bool is_flinching;
	float flinch_timer;
	float flinch_duration;
	int flinch_anim;

	// Death and Respawn state
	bool is_dead;
	float death_timer;
	float death_anim_duration;
	bool is_flashing;
	float flash_timer;

    // Freeze state
    bool is_frozen;
    float frozen_timer;

	Enemy() : rotate(0.0f), scale(1.0f), visible(false), speed(0.8f), is_attacking(false), attack_timer(0.0f), attack_cooldown(0.0f), attack_duration(1.25f), attack_range(1.0f), has_hit_player(false), punch_active(false), health(100.0f), max_health(100.0f), is_flinching(false), flinch_timer(0.0f), flinch_duration(0.0f), flinch_anim(16), is_dead(false), death_timer(0.0f), death_anim_duration(2.66f), is_flashing(false), flash_timer(0.0f), is_frozen(false), frozen_timer(0.0f) {}

	Enemy(float px, float py, float pz, float rot, float sc, bool vis, float bbox_w, float bbox_h, float bbox_d)
		: rotate(rot), scale(sc), visible(vis), speed(0.8f),
		is_attacking(false), attack_timer(0.0f), attack_cooldown(0.0f),
		attack_duration(1.25f), attack_range(1.0f), has_hit_player(false), punch_active(false),
		health(100.0f), max_health(100.0f), is_flinching(false), flinch_timer(0.0f), flinch_duration(0.0f), flinch_anim(16),
		is_dead(false), death_timer(0.0f), death_anim_duration(2.66f), is_flashing(false), flash_timer(0.0f), is_frozen(false), frozen_timer(0.0f)
	{
		position.x = px; position.y = py; position.z = pz;
		bbox = MakeAABBFromCenterSize(position, glm::vec3(bbox_w, bbox_h, bbox_d));
	}
};
	

struct MapItem {
	AABB bbox;
	glm::vec3 position;
	glm::vec3 scale;
};

struct Collectible {
    glm::vec3 position;
    glm::vec3 velocity;
    bool active;
    float timer;
    float duration;
    float blink_time;
    AABB bbox;
    float scale;
    bool visible_this_frame;
    int type; // 0 = health, 1 = transform, 2 = special

    Collectible() : active(false), timer(0.0f), duration(10.0f), blink_time(7.0f), scale(0.15f), visible_this_frame(true), type(0) {}
};

#endif