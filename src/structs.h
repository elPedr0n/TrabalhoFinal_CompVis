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
			if(deltaX > 0 && max.x <= against.min.x) {
				//what is the distance to the other AABB?
				float clip = against.min.x - max.x;
				//if our move delta is larger than the distance to
				//the other AABB, set the move delta that distance
				if (deltaX > clip)
					deltaX = clip;
			}
			//the principle explained in the code above is the same for
			//everything else
			if (deltaX < 0 && min.x >= against.max.x) {
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
			if (deltaY > 0 && max.y <= against.min.y) {
				float clip = against.min.y - max.y;
				if (deltaY > clip)
					deltaY = clip;
			}
			if (deltaY < 0 && min.y >= against.max.y) {
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
			if (deltaZ > 0 && max.z <= against.min.z) {
				float clip = against.min.z - max.z;
				if (deltaZ > clip)
					deltaZ = clip;
			}
			if (deltaZ < 0 && min.z >= against.max.z) {
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

	Character() : model_name(""), scale(0.5f), bbox() {} 

	Character(const char* name, glm::vec3 pos, float bbox_w, float bbox_h, float bbox_d, float scale)
		: model_name(name), scale(scale), bbox(makeAABBFromGround(pos, glm::vec3(bbox_w * scale, bbox_h * scale, bbox_d * scale))) {}
};

struct Player {
	int active_character; // 0 = BigChill, 1 = Swampfire
	Character characters[MAX_CHARACTERS];

	glm::vec3 position;
	glm::vec3 speed;
	
	float rotate;
	float scale;
	float jump_speed;

	bool jumping;
	bool double_jump_available;

	Player()
		: active_character(2), position(0.0f, -1.0f, 0.0f), speed(2.0f, 0.0f, 2.0f), rotate(0.0f), scale(1.0f), jump_speed(6.0f), jumping(false), double_jump_available(false)
	{
		characters[0] = Character("the_bigchill", position, 1.38963f, 1.96548f, 0.454046f, 0.5f);
		characters[1] = Character("the_swampfire", position, 3.28f, 3.8f, 2.0f, 0.3f);
		characters[2] = Character("the_bentennyson", position, 2.0f, 2.5f, 2.0f, 0.6f);
	}
};

struct Enemy {
	glm::vec3 position;
	float rotate;
	float scale;
	bool visible;
	AABB bbox;
	float speed; // Velocidade de movimento do inimigo

	Enemy(float px, float py, float pz, float rot, float sc, bool vis, float bbox_w, float bbox_h, float bbox_d)
		: rotate(rot), scale(sc), visible(vis), speed(0.8f)
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

#endif