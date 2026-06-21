#pragma once

#include <glm/glm.hpp>
#include <map>
#include <string>
#include <glad/glad.h>
#include "sceneobject.h"

// Particle options for fine-grained control (color, life, scale, speed, count)
struct ParticleOptions {
    glm::vec3 color = glm::vec3(1.0f, 0.75f, 0.12f); // default fire yellow
    float life = 0.5f;
    float scale = 0.02f;
    float speed = 1.0f;
    int count = 4;
    bool additive = true; // Use additive blending (e.g. for fire)
};

// Single spawn with options: specify `ParticleOptions` to control color, life, scale, speed and count.
void Particles_Spawn(const glm::vec3 &pos, const ParticleOptions &opts);

// Directional spawn: spawns particles moving primarily in `dir` direction with some `spread`
void Particles_SpawnDirectional(const glm::vec3 &pos, const glm::vec3 &dir, float spread, const ParticleOptions &opts);

// Convert 0xRRGGBB hex color to normalized glm::vec3
inline glm::vec3 HexToRgb(unsigned int hex) {
    float r = float((hex >> 16) & 0xFF) / 255.0f;
    float g = float((hex >> 8) & 0xFF) / 255.0f;
    float b = float((hex >> 0) & 0xFF) / 255.0f;
    return glm::vec3(r,g,b);
}

// Accept a hex string like "#RRGGBB" or "RRGGBB" or short form "#RGB" and convert to rgb
inline glm::vec3 HexToRgb(const char *hexStr)
{
    if (!hexStr) return glm::vec3(1.0f,1.0f,1.0f);
    std::string s(hexStr);
    if (!s.empty() && s[0] == '#') s.erase(s.begin());
    if (s.size() == 3) {
        // expand short form E.g. "F5A" -> "FF55AA"
        std::string e;
        e.reserve(6);
        for (char c : s) { e.push_back(c); e.push_back(c); }
        s = e;
    }
    if (s.size() != 6) return glm::vec3(1.0f,1.0f,1.0f);
    unsigned int v = (unsigned int) strtoul(s.c_str(), NULL, 16);
    return HexToRgb(v);
}

// Update particles simulation
void Particles_Update(float delta_t);

// Draw particles using the virtual scene mesh `the_sphere` scaled small
void Particles_Draw(const std::map<std::string, SceneObject> &scene,
                    GLuint gpuProgramID,
                    GLint modelUniform,
                    GLint objectIdUniform,
                    float global_scale);
