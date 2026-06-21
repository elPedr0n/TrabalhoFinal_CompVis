#ifndef FRAGMENTS_H
#define FRAGMENTS_H

#include "globals.h"
#include <glm/glm.hpp>
#include <glad/glad.h>

#define MAX_FRAGMENTS 500

struct Fragment {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 rotation;
    glm::vec3 rotation_speed;
    
    float scale;
    glm::vec3 color;
    
    bool active;
    float timer;
    float duration;
    
    Fragment() : active(false) {}
};

extern Fragment g_fragments[MAX_FRAGMENTS];

void SpawnFragments(glm::vec3 pos, int count, float scale, glm::vec3 color);
void UpdateFragments();
void DrawFragments(GLint model_uniform, GLint object_id_uniform, GLint override_kd_uniform, GLint use_override_kd_uniform);

#endif // FRAGMENTS_H
