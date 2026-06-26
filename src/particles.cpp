#include "particles.h"

#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>

// FONTE: Biblioteca GLM (OpenGL Mathematics) — usada para tipos matemáticos (glm::vec3, glm::mat4)
// e funções utilitárias. Repositório: https://github.com/g-truc/glm — Licença: MIT License
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "matrices.h"

namespace {
    struct Particle { glm::vec3 pos; glm::vec3 vel; float age=0.0f; float life=1.0f; float scale=0.1f; glm::vec3 color = glm::vec3(1.0f); bool additive = true; };
    std::vector<Particle> s_particles;
}

// Public spawn: position + options
void Particles_Spawn(const glm::vec3 &pos, const ParticleOptions &opts)
{
    int count = std::max(1, opts.count);
    for (int i = 0; i < count; ++i) {
        Particle p;
        float ang = ((float)std::rand() / RAND_MAX) * 2.0f * 3.14159265f;
        float up = ((float)std::rand() / RAND_MAX) * 0.6f + 0.2f;
        float speed = opts.speed * (((float)std::rand() / RAND_MAX) * 0.8f + 0.6f);
        p.pos = pos + glm::vec3(((float)(std::rand()%100)/100.0f - 0.5f) * 0.05f, 0.02f, ((float)(std::rand()%100)/100.0f - 0.5f) * 0.05f);
        p.vel = glm::vec3(std::sin(ang), up, std::cos(ang)) * speed;
        p.age = 0.0f;
        p.life = opts.life * (0.8f + ((float)std::rand() / RAND_MAX) * 0.4f);
        p.scale = opts.scale * (0.8f + ((float)std::rand() / RAND_MAX) * 0.4f);
        p.color = opts.color;
        p.additive = opts.additive;
        s_particles.push_back(p);
    }
}

void Particles_SpawnDirectional(const glm::vec3 &pos, const glm::vec3 &dir, float spread, const ParticleOptions &opts)
{
    int count = std::max(1, opts.count);
    glm::vec3 n_dir = glm::length(dir) > 0.0f ? glm::normalize(dir) : glm::vec3(1,0,0);
    for (int i = 0; i < count; ++i) {
        Particle p;
        float x_noise = (((float)std::rand() / RAND_MAX) * 2.0f - 1.0f) * spread;
        float y_noise = (((float)std::rand() / RAND_MAX) * 2.0f - 1.0f) * spread;
        float z_noise = (((float)std::rand() / RAND_MAX) * 2.0f - 1.0f) * spread;
        
        glm::vec3 final_dir = glm::normalize(n_dir + glm::vec3(x_noise, y_noise, z_noise));
        float speed = opts.speed * (((float)std::rand() / RAND_MAX) * 0.4f + 0.8f);
        
        p.pos = pos + glm::vec3(((float)(std::rand()%100)/100.0f - 0.5f) * 0.2f, ((float)(std::rand()%100)/100.0f - 0.5f) * 0.2f, ((float)(std::rand()%100)/100.0f - 0.5f) * 0.2f);
        p.vel = final_dir * speed;
        p.age = 0.0f;
        p.life = opts.life * (0.8f + ((float)std::rand() / RAND_MAX) * 0.4f);
        p.scale = opts.scale * (0.8f + ((float)std::rand() / RAND_MAX) * 0.4f);
        p.color = opts.color;
        p.additive = opts.additive;
        s_particles.push_back(p);
    }
}


void Particles_Update(float delta_t)
{
    for (auto &p : s_particles) {
        p.pos += p.vel * delta_t;
        p.vel *= 0.98f; // slight drag
        p.age += delta_t;
    }
    s_particles.erase(std::remove_if(s_particles.begin(), s_particles.end(), [](const Particle &p){ return p.age >= p.life; }), s_particles.end());
}

void Particles_Draw(const std::map<std::string, SceneObject> &scene,
                    GLuint gpuProgramID,
                    GLint modelUniform,
                    GLint objectIdUniform,
                    float global_scale)
{
    auto it = scene.find("the_sphere");
    if (it == scene.end()) return;
    
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    // Prepare override uniform locations (shader will use OverrideKd when UseOverrideKd==1)
    GLint overrideKdLoc = glGetUniformLocation(gpuProgramID, "OverrideKd");
    GLint useOverrideLoc = glGetUniformLocation(gpuProgramID, "UseOverrideKd");

    // Draw each particle as a small sphere instance using the virtual scene sphere mesh
    for (const auto &p : s_particles) {
        glm::mat4 model = Matrix_Translate(p.pos.x, p.pos.y, p.pos.z)
                        * Matrix_Scale(global_scale * p.scale, global_scale * p.scale, global_scale * p.scale);
        glUniformMatrix4fv(modelUniform, 1, GL_FALSE, glm::value_ptr(model));

        // Set override color for this particle
        if (useOverrideLoc >= 0) glUniform1i(useOverrideLoc, 1);
        if (overrideKdLoc >= 0) glUniform3fv(overrideKdLoc, 1, glm::value_ptr(p.color));

        if (p.additive) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        // Set a default object id (FIREBALL-like) so shader branches with textures behave
        glUniform1i(objectIdUniform, 6);

        if (it->second.texture_id != 0) {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, it->second.texture_id);
            glUniform1i(glGetUniformLocation(gpuProgramID, "TextureImage4"), 4);
        }

        glBindVertexArray(it->second.vertex_array_object_id);
        glDrawElements(it->second.rendering_mode, it->second.num_indices, GL_UNSIGNED_INT, (void*)(it->second.first_index * sizeof(GLuint)));
        glBindVertexArray(0);

        // Disable override after drawing this particle
        if (useOverrideLoc >= 0) glUniform1i(useOverrideLoc, 0);
    }

    // Restore GL state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
