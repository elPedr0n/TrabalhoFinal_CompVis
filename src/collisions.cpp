#include "structs.h"

bool AABB::IntersectsX(AABB other) {
	return min.x < other.max.x && max.x > other.min.x;
}

bool AABB::IntersectsY(AABB other) {
	return min.y < other.max.y && max.y > other.min.y;
}

bool AABB::IntersectsZ(AABB other) {
	return min.z < other.max.z && max.z > other.min.z;
}

bool AABB::Intersects(AABB other) {
	return IntersectsX(other) && IntersectsY(other) && IntersectsZ(other);
}

void AABB::Move(float valueX, float valueY, float valueZ) {
	min.x += valueX;
	max.x += valueX;
	min.y += valueY;
	max.y += valueY;
	min.z += valueZ;
	max.z += valueZ;
}

float AABB::GetClipX(AABB against, float deltaX) {
	//are we overlapping the other axes?
	//(if we aren't, then an intersection could never actually take place)
	if((min.y + 0.005f) < against.max.y && (max.y - 0.005f) > against.min.y &&
	   (min.z + 0.005f) < against.max.z && (max.z - 0.005f) > against.min.z) {
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

float AABB::GetClipY(AABB against, float deltaY) {
	if ((min.x + 0.005f) < against.max.x && (max.x - 0.005f) > against.min.x &&
		(min.z + 0.005f) < against.max.z && (max.z - 0.005f) > against.min.z) {
		if (deltaY > 0 && max.y <= against.min.y + 0.002f) {
			float clip = against.min.y - max.y;
			if (deltaY > clip)
				deltaY = clip;
		}
		if (deltaY < 0 && min.y >= against.max.y - 0.5f) {
			float clip = against.max.y - min.y;
			if (deltaY < clip)
				deltaY = clip;
		}
		return deltaY;
	}
	return deltaY;
}

float AABB::GetClipZ(AABB against, float deltaZ) {
	if ((min.x + 0.005f) < against.max.x && (max.x - 0.005f) > against.min.x &&
		(min.y + 0.005f) < against.max.y && (max.y - 0.005f) > against.min.y) {
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

AABB MakeAABBFromCenterSize(const glm::vec3& center, const glm::vec3& size)
{
	glm::vec3 half = size * 0.5f;
	return AABB(center - half, center + half);
}

AABB makeAABBFromGround(const glm::vec3& position, const glm::vec3& size)
{
	glm::vec3 half = size * 0.5f;
	return AABB(glm::vec3(position.x - half.x, position.y, position.z - half.z),
				glm::vec3(position.x + half.x, position.y + size.y, position.z + half.z));
}
