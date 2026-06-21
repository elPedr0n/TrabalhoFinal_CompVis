#ifndef FERRIS_WHEEL_H
#define FERRIS_WHEEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "tiny_gltf.h"

// Desenha a roda gigante respeitando a hierarquia de nós do GLTF.
// A roda principal (wheel) e as cabines giram continuamente no eixo Z.
//
// Parâmetros:
//   model          – o tinygltf::Model carregado de ferris_wheel/scene.gltf
//   base_name      – prefixo usado ao registrar no g_VirtualScene (ex: "the_ferris_wheel")
//   prog           – ID do programa OpenGL (glsl)
//   model_uniform  – localização do uniform "model"
//   obj_id_uniform – localização do uniform "object_id"
//   world          – matriz de modelo que o usuário quer aplicar (posição, escala, rotação no mundo)
//   time           – tempo acumulado em segundos para animar a rotação
//   spin_speed     – velocidade angular em rad/s (ex: 0.5f)
void DrawFerrisWheel(
    const tinygltf::Model& model,
    const std::string&     base_name,
    GLuint                 prog,
    GLint                  model_uniform,
    GLint                  obj_id_uniform,
    const glm::mat4&       world,
    float                  time,
    float                  spin_speed = 0.4f
);

void DrawHierarchicalGLTF(
    const tinygltf::Model& model,
    const std::string&     base_name,
    GLuint                 prog,
    GLint                  model_uniform,
    GLint                  obj_id_uniform,
    int                    obj_id,
    const glm::mat4&       world
);

#endif // FERRIS_WHEEL_H
