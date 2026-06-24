#ifndef BREAKABLES_H
#define BREAKABLES_H

#include "globals.h"
#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>

#define MAX_BREAKABLES 50

struct Breakable {
    glm::vec3 position;
    float velocity_y;
    float scale;
    float rotation_x;
    float rotation_y;
    float rotation_z;
    float bbox_w, bbox_h, bbox_d;
    
    float health;
    float max_health;
    
    AABB bbox;
    std::string model_name;
    
    bool active;
    
    glm::vec3 fragment_color;
    float fragment_size;
    int fragment_count;
    
    bool is_flinching;
    float flinch_timer;
    
    Breakable() : active(false), is_flinching(false), flinch_timer(0.0f) {}
};

extern Breakable g_breakables[MAX_BREAKABLES];

void SpawnBreakable(glm::vec3 pos, float scale, const std::string& model_name, float health, float bbox_w, float bbox_h, float bbox_d, glm::vec3 frag_color, float frag_size, int frag_count, float rot_y = 0.0f);
void UpdateBreakables();
void ApplyDamageToBreakable(int id, float damage);
void DrawBreakables(GLint model_uniform, GLint object_id_uniform, GLint override_kd_uniform, GLint use_override_kd_uniform);

#endif
