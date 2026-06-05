#version 330 core

// Atributos
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;
layout (location = 3) in float material_coefficients;
// Skinning attributes
layout (location = 4) in uvec4 jointIds;
layout (location = 5) in vec4 weights;
// Per-vertex color (used for debug axes)
layout (location = 6) in vec3 vertex_color;

// Matrizes uniformes
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Uniforme para identificar qual objeto estamos desenhando
// (Como o OpenGL compartilha uniformes, ele vai pegar o valor que você já envia na main.cpp!)
uniform int object_id; 
// BEN object id (match CPU-side)
#define BEN 8
#define FOREVERKNIGHT 9

// Array de ossos
const int MAX_BONES = 100;
uniform mat4 boneMatrices[MAX_BONES];

// Saídas para o Fragment Shader
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;
out float material_id;
out vec3 vert_color;

void main()
{
    // Por padrão, usamos a posição e normal originais do modelo estático
    vec4 local_position = model_coefficients;
    vec4 local_normal = normal_coefficients;

    // Se o objeto for o Swampfire (ID 4) ou Ben (ID BEN) ou ForeverKnight (ID FOREVERKNIGHT), aplicamos os ossos!
    if (object_id == 4 || object_id == BEN || object_id == FOREVERKNIGHT) 
    {
        float weightSum = weights[0] + weights[1] + weights[2] + weights[3];
        if (weightSum > 0.0) {
            mat4 boneTransform = mat4(0.0);
            boneTransform += boneMatrices[jointIds[0]] * weights[0];
            boneTransform += boneMatrices[jointIds[1]] * weights[1];
            boneTransform += boneMatrices[jointIds[2]] * weights[2];
            boneTransform += boneMatrices[jointIds[3]] * weights[3];

            // Entorta o vértice
            local_position = boneTransform * model_coefficients;
            
            // Entorta a normal para a luz acompanhar a malha (matemática de normais)
            mat3 boneNormalTransform = mat3(inverse(transpose(boneTransform)));
            local_normal = vec4(boneNormalTransform * normal_coefficients.xyz, 0.0);
        }
    }

    // Projeta na tela
    gl_Position = projection * view * model * local_position;

    // Calcula variáveis para a iluminação no Fragment Shader
    position_world = model * local_position;
    position_model = local_position;
    
    // Calcula a normal final global
    normal = inverse(transpose(model)) * local_normal;
    normal.w = 0.0;

    // Passa adiante texturas e materiais
    texcoords = texture_coefficients;
    material_id = material_coefficients;
    vert_color = vertex_color;
}