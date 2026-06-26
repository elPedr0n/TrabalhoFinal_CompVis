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

#include "breakables.h"
#include "sound.h"
#include <cmath>

float CheckMapCollisionX(AABB bbox, float move_x) {
    for (int i = 0; i < g_num_platforms; i++) {
        move_x = bbox.GetClipX(map[i].bbox, move_x);
    }
    return move_x;
}

float CheckMapCollisionY(AABB bbox, float move_y) {
    for (int i = 0; i < g_num_platforms; i++) {
        move_y = bbox.GetClipY(map[i].bbox, move_y);
    }
    return move_y;
}

float CheckMapCollisionZ(AABB bbox, float move_z) {
    for (int i = 0; i < g_num_platforms; i++) {
        move_z = bbox.GetClipZ(map[i].bbox, move_z);
    }
    return move_z;
}

float CheckBreakablesCollisionX(AABB bbox, float move_x, int ignore_id) {
    for (int i = 0; i < MAX_BREAKABLES; ++i) {
        if (i != ignore_id && g_breakables[i].active) {
            move_x = bbox.GetClipX(g_breakables[i].bbox, move_x);
        }
    }
    return move_x;
}

float CheckBreakablesCollisionY(AABB bbox, float move_y, int ignore_id) {
    for (int i = 0; i < MAX_BREAKABLES; ++i) {
        if (i != ignore_id && g_breakables[i].active) {
            move_y = bbox.GetClipY(g_breakables[i].bbox, move_y);
        }
    }
    return move_y;
}

float CheckBreakablesCollisionZ(AABB bbox, float move_z, int ignore_id) {
    for (int i = 0; i < MAX_BREAKABLES; ++i) {
        if (i != ignore_id && g_breakables[i].active) {
            move_z = bbox.GetClipZ(g_breakables[i].bbox, move_z);
        }
    }
    return move_z;
}

void ResolvePlayerMapCollisions() {
    auto& bbox = player.characters[player.active_character].bbox;
    for (int p_i = 0; p_i < g_num_platforms; p_i++) {
        const auto& item = map[p_i];
        if (bbox.Intersects(item.bbox)) {
            glm::vec3 centerP = (bbox.min + bbox.max) * 0.5f;
            glm::vec3 centerM = (item.bbox.min + item.bbox.max) * 0.5f;
            glm::vec3 extP = (bbox.max - bbox.min) * 0.5f;
            glm::vec3 extM = (item.bbox.max - item.bbox.min) * 0.5f;
            
            float dx = centerP.x - centerM.x;
            float dz = centerP.z - centerM.z;
            
            float px = (extP.x + extM.x) - std::abs(dx);
            float pz = (extP.z + extM.z) - std::abs(dz);
            
            if (px < pz) {
                if (dx > 0) player.position.x += px + 0.001f;
                else        player.position.x -= px + 0.001f;
            } else {
                if (dz > 0) player.position.z += pz + 0.001f;
                else        player.position.z -= pz + 0.001f;
            }
            
            glm::vec3 size = player.active_character == 0 ? bigchill_size : (player.active_character == 1 ? swampfire_size : bentennyson_size);
            bbox = makeAABBFromGround(player.position, size);
        }
    }
}

#include "particles.h"
bool ProcessSwampfireMeleeHitboxes(const SwampfireAnimResult& animRes, SwampfireAnimState& state, int restore_object_id, bool just_triggered) 
{
    bool hit_something = false;
    if (!animRes.punch1_active && !animRes.punch2_active) return hit_something;

    glm::vec3 forward = glm::vec3(sin(player.rotate), 0.0f, cos(player.rotate));
    glm::vec3 hitbox_size = glm::vec3(0.8f, 0.5f, 0.8f);  // tune these
    float reach = 0.35f;
    float height = 0.5f;

    if (animRes.punch1_active) {
        glm::vec3 center = player.position + forward * reach + glm::vec3(0.0f, height, 0.0f);
        AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);
        DrawBoundingBox(punch_box, restore_object_id);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].visible) continue;
            if (g_enemies[i].is_dead) continue;
            if (state.punch1_hit_enemies.count(i)) continue;
            if (punch_box.Intersects(g_enemies[i].bbox)) {
                printf("Punch 1 hit enemy %d!\n", i);
                hit_something = true;
                state.punch1_hit_enemies.insert(i);
                ApplyDamageToEnemy(i, 20.0f);
                glm::vec3 overlap_min = glm::max(punch_box.min, g_enemies[i].bbox.min);
                glm::vec3 overlap_max = glm::min(punch_box.max, g_enemies[i].bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts; popts.color = HexToRgb("#ffffff"); popts.life=0.3f; popts.scale=0.1f; popts.speed=3.0f; popts.count=15;
                Particles_Spawn(contact, popts);
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (state.punch1_hit_enemies.count(1000 + i)) continue; // offset to reuse set
            if (punch_box.Intersects(g_breakables[i].bbox)) {
                hit_something = true;
                state.punch1_hit_enemies.insert(1000 + i);
                ApplyDamageToBreakable(i, 20.0f);
            }
        }
    }

    if (animRes.punch2_active) {
        glm::vec3 center = player.position + forward * reach + glm::vec3(0.0f, height, 0.0f);
        AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);
        DrawBoundingBox(punch_box, restore_object_id);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].visible) continue;
            if (g_enemies[i].is_dead) continue;
            if (state.punch2_hit_enemies.count(i)) continue;
            if (punch_box.Intersects(g_enemies[i].bbox)) {
                printf("Punch 2 hit enemy %d!\n", i);
                hit_something = true;
                state.punch2_hit_enemies.insert(i);
                ApplyDamageToEnemy(i, 20.0f);
                glm::vec3 overlap_min = glm::max(punch_box.min, g_enemies[i].bbox.min);
                glm::vec3 overlap_max = glm::min(punch_box.max, g_enemies[i].bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts; popts.color = HexToRgb("#ffffff"); popts.life=0.3f; popts.scale=0.1f; popts.speed=3.0f; popts.count=15;
                Particles_Spawn(contact, popts);
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (state.punch2_hit_enemies.count(1000 + i)) continue;
            if (punch_box.Intersects(g_breakables[i].bbox)) {
                hit_something = true;
                state.punch2_hit_enemies.insert(1000 + i);
                ApplyDamageToBreakable(i, 20.0f);
            }
        }
    }
    
    return hit_something;
}

bool ProcessBigChillMeleeHitboxes(const BigChillAnimResult& animRes, BigChillAnimState& state, int restore_object_id, bool just_triggered) {
    bool hit_something = false;
    if (!animRes.punch_active && !animRes.magic_active) return hit_something;

    glm::vec3 forward(sin(player.rotate), 0.0f, cos(player.rotate));
    float reach = 0.4f;
    float height = 0.5f;

    if (animRes.punch_active) {
        glm::vec3 center = player.position + forward * reach + glm::vec3(0.0f, height, 0.0f);
        glm::vec3 hitbox_size = glm::vec3(0.8f, 0.5f, 0.8f);
        AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);
        DrawBoundingBox(punch_box, restore_object_id);

        for (int i = 0; i < 20; i++) {
            if (!g_enemies[i].visible || g_enemies[i].is_dead) continue;
            if (state.punch_hit_enemies.count(i)) continue;

            if (punch_box.Intersects(g_enemies[i].bbox)) {
                hit_something = true;
                state.punch_hit_enemies.insert(i);
                ApplyDamageToEnemy(i, 20.0f);
                glm::vec3 overlap_min = glm::max(punch_box.min, g_enemies[i].bbox.min);
                glm::vec3 overlap_max = glm::min(punch_box.max, g_enemies[i].bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts; popts.color = HexToRgb("#ffffff"); popts.life=0.3f; popts.scale=0.1f; popts.speed=3.0f; popts.count=15;
                Particles_Spawn(contact, popts);
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (state.punch_hit_enemies.count(1000 + i)) continue;
            if (punch_box.Intersects(g_breakables[i].bbox)) {
                hit_something = true;
                state.punch_hit_enemies.insert(1000 + i);
                ApplyDamageToBreakable(i, 20.0f);
            }
        }
    }

    if (animRes.magic_active) {
        // Lower the center
        glm::vec3 center = player.position + forward * 0.8f + glm::vec3(0.0f, 0.5f, 0.0f);
        
        // Calculate dynamic AABB extents based on rotation
        float local_x = 0.4f; // Half of 0.8 width
        float local_y = 0.5f; // Half of 1.0 height
        float local_z = 0.75f; // Half of 1.5 length
        
        float abs_sin = std::abs(sin(player.rotate));
        float abs_cos = std::abs(cos(player.rotate));
        
        float world_x = local_x * abs_cos + local_z * abs_sin;
        float world_z = local_x * abs_sin + local_z * abs_cos;
        
        glm::vec3 dynamic_hitbox_size(world_x * 2.0f, local_y * 2.0f, world_z * 2.0f);
        
        AABB magic_box = MakeAABBFromCenterSize(center, dynamic_hitbox_size);
        DrawBoundingBox(magic_box, restore_object_id);

        for (int i = 0; i < 20; i++) {
            if (!g_enemies[i].visible || g_enemies[i].is_dead) continue;
            if (magic_box.Intersects(g_enemies[i].bbox)) {
                hit_something = true;
                g_enemies[i].is_frozen = true;
                g_enemies[i].frozen_timer = 3.0f;
                // continuous low damage without triggering flinch
                ApplyDamageToEnemy(i, 15.0f * delta_t, false); 
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (magic_box.Intersects(g_breakables[i].bbox)) {
                hit_something = true;
                ApplyDamageToBreakable(i, 15.0f * delta_t); 
            }
        }
    }
    
    return hit_something;
}



bool ProcessBenMeleeHitboxes(const BenAnimResult& animRes, BenAnimState& state, int restore_object_id, bool just_triggered) 
{
    bool hit_something = false;
    if (!animRes.punch_active && !animRes.big_slap_active) return hit_something;

    glm::vec3 forward = glm::vec3(sin(player.rotate), 0.0f, cos(player.rotate));
    float reach = 0.35f;
    float height = 0.5f;

    if (animRes.punch_active) {
        glm::vec3 hitbox_size = glm::vec3(0.8f, 0.5f, 0.8f);  // tune these
        glm::vec3 center = player.position + forward * reach + glm::vec3(0.0f, height, 0.0f);
        AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);
        DrawBoundingBox(punch_box, restore_object_id);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].visible) continue;
            if (g_enemies[i].is_dead) continue;
            if (state.punch_hit_enemies.count(i)) continue;
            if (punch_box.Intersects(g_enemies[i].bbox)) {
                printf("Ben punch hit enemy %d!\n", i);
                state.punch_hit_enemies.insert(i);
                hit_something = true;
                ApplyDamageToEnemy(i, 5.0f);
                glm::vec3 overlap_min = glm::max(punch_box.min, g_enemies[i].bbox.min);
                glm::vec3 overlap_max = glm::min(punch_box.max, g_enemies[i].bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts; popts.color = HexToRgb("#ffffff"); popts.life=0.3f; popts.scale=0.1f; popts.speed=3.0f; popts.count=10;
                Particles_Spawn(contact, popts);
                state.attack_speed_multiplier *= 2.0f; // Increase speed on hit
                if (state.attack_speed_multiplier > 6.0f) {
                    state.attack_speed_multiplier = 6.0f;
                }
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (state.punch_hit_enemies.count(1000 + i)) continue;
            if (punch_box.Intersects(g_breakables[i].bbox)) {
                state.punch_hit_enemies.insert(1000 + i);
                hit_something = true;
                ApplyDamageToBreakable(i, 5.0f);
            }
        }
    }

    if (animRes.big_slap_active) {
        glm::vec3 hitbox_size = glm::vec3(1.2f, 1.0f, 1.2f); // Big slap has a bigger hitbox!
        glm::vec3 center = player.position + forward * 0.5f + glm::vec3(0.0f, height, 0.0f);
        AABB slap_box = MakeAABBFromCenterSize(center, hitbox_size);
        DrawBoundingBox(slap_box, restore_object_id);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].visible || g_enemies[i].is_dead) continue;
            if (state.big_slap_hit_enemies.count(i)) continue;
            if (slap_box.Intersects(g_enemies[i].bbox)) {
                printf("Big slap applied to enemy %d!\n", i); // History log per the user's request
                state.big_slap_hit_enemies.insert(i);
                hit_something = true;
                ApplyDamageToEnemy(i, 10.0f); // More damage than normal punch
                glm::vec3 overlap_min = glm::max(slap_box.min, g_enemies[i].bbox.min);
                glm::vec3 overlap_max = glm::min(slap_box.max, g_enemies[i].bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts; popts.color = HexToRgb("#FFFFFF"); popts.life=0.5f; popts.scale=0.15f; popts.speed=5.0f; popts.count=20;
                Particles_Spawn(contact, popts);
            }
        }
        for (int i = 0; i < MAX_BREAKABLES; i++) {
            if (!g_breakables[i].active) continue;
            if (state.big_slap_hit_enemies.count(1000 + i)) continue;
            if (slap_box.Intersects(g_breakables[i].bbox)) {
                state.big_slap_hit_enemies.insert(1000 + i);
                hit_something = true;
                ApplyDamageToBreakable(i, 10.0f);
            }
        }
    }

    return hit_something;
}


void ProcessEnemyMeleeHitboxes()
{
    auto& player_bbox = player.characters[player.active_character].bbox;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].visible) continue;
        if (!g_enemies[i].punch_active) continue;
        glm::vec3 forward = glm::vec3(
            sin(g_enemies[i].rotate), 0.0f, cos(g_enemies[i].rotate));
        
        // Ajustando tamanho e alcance para casar com o ataque
        glm::vec3 hitbox_size = glm::vec3(0.8f, 0.6f, 0.8f);
        float reach = 0.8f;
        float height = 0.3f;

        glm::vec3 center = g_enemies[i].position 
                         + forward * reach 
                         + glm::vec3(0.0f, height, 0.0f);
        AABB punch_box = MakeAABBFromCenterSize(center, hitbox_size);


        // Somente depois de desenhar, verificamos se já bateu no player para não contar dano duplo
        if (g_enemies[i].has_hit_player) continue;

        if (punch_box.Intersects(player_bbox)) {
            printf("Enemy %d hit the player!\n", i);
            g_enemies[i].has_hit_player = true;
            if (rand() % 2 == 0) PlaySoundEffect("../../data/sounds/knight_slice1.wav");
            else PlaySoundEffect("../../data/sounds/knight_slice2.wav");
            if (!player.is_dead) {
                glm::vec3 overlap_min = glm::max(punch_box.min, player_bbox.min);
                glm::vec3 overlap_max = glm::min(punch_box.max, player_bbox.max);
                glm::vec3 contact = (overlap_min + overlap_max) * 0.5f;
                ParticleOptions popts;
                popts.color = HexToRgb("#ffffff");
                popts.life = 0.3f;
                popts.scale = 0.15f;
                popts.speed = 2.0f;
                popts.count = 15;
                Particles_Spawn(contact, popts);
                
                ApplyDamageToPlayer(50.0f, g_enemies[i].position);
            }
        }
    }
}