### Commit com auxílio de IA: Carregamento de um dos modelos dos alienígenas para testes (5fe8cafcf566dbf1ee734b49811dcac3ad6e757d)

> Esse primeiro commit com IA acabei esquecendo de colocar o prompt, segue o código editado pela IA assim como seu prompt.

"Na pasta de data coloquei um novo modelo de um dos alienigenas do jogo com 2 texturas. Quero que você modifique a main e os arquivos .glsl para poder renderizar de forma correta esse novo modelo."

na `main.cpp`
```c
56 std::map<std::string, std::vector<float>> face_texture_selector_by_shape;
...
111 void ParseFaceTextureSelectorsFromObj(const char* filename)
    {
        std::ifstream file(filename);
        if (!file.good())
            return;

        std::string line;
        std::string current_shape;
        float current_selector = 2.0f; // bcck1.png => TextureImage2

        while (std::getline(file, line))
        {
            if (line.rfind("g ", 0) == 0 || line.rfind("o ", 0) == 0)
            {
                current_shape = line.substr(2);
                continue;
            }

            if (line.rfind("usemtl ", 0) == 0)
            {
                std::string material_name = line.substr(7);
                if (material_name.find("bcck2") != std::string::npos)
                    current_selector = 3.0f; // bcck2.png => TextureImage3
                else if (material_name.find("bcck1") != std::string::npos)
                    current_selector = 2.0f;
                continue;
            }

            if (line.rfind("f ", 0) == 0 && !current_shape.empty())
            {
                face_texture_selector_by_shape[current_shape].push_back(current_selector);
            }
        }
144    }
...
341 LoadTextureImage("../../data/bcck1.png"); // TextureImage2
342 LoadTextureImage("../../data/bcck2.png"); // TextureImage3
...
357 ObjModel bigchillmodel("../../data/big_chill_cloaked.obj")
358 ComputeNormals(&bigchillmodel);
359 BuildTrianglesAndAddToVirtualScene(&bigchillmodel);
...
828 int material_id = -1;
    if (triangle < model->shapes[shape].mesh.material_ids.size())
    {
        material_id = model->shapes[shape].mesh.material_ids[triangle];
    }

    float texture_selector = 2.0f; // bcck1.png => TextureImage2
    auto face_selector_it = model->face_texture_selector_by_shape.find(model->shapes[shape].name);
    if (face_selector_it != model->face_texture_selector_by_shape.end() &&
        triangle < face_selector_it->second.size())
    {
        texture_selector = face_selector_it->second[triangle];
    }
    else if (material_id >= 0 && material_id < (int)model->materials.size())
    {
        const auto& material = model->materials[material_id];
        const bool uses_bcck2 =
            material.name.find("bcck2") != std::string::npos ||
            material.diffuse_texname.find("bcck2") != std::string::npos;
        const bool uses_bcck1 =
            material.name.find("bcck1") != std::string::npos ||
            material.diffuse_texname.find("bcck1") != std::string::npos;

        if (uses_bcck2)
        {
            texture_selector = 3.0f;
        }
        else if (!uses_bcck1)
        {
            auto it = fallback_texture_unit_by_material_id.find(material_id);
            if (it == fallback_texture_unit_by_material_id.end())
            {
                fallback_texture_unit_by_material_id[material_id] = next_fallback_texture_unit;
                texture_selector = next_fallback_texture_unit;
                next_fallback_texture_unit = (next_fallback_texture_unit == 2.0f) ? 3.0f : 2.0f;
            }
            else
            {
                texture_selector = it->second;
            }
        }
    }
    else if (material_id >= 0)
    {
        auto it = fallback_texture_unit_by_material_id.find(material_id);
        if (it == fallback_texture_unit_by_material_id.end())
        {
            fallback_texture_unit_by_material_id[material_id] = next_fallback_texture_unit;
            texture_selector = next_fallback_texture_unit;
            next_fallback_texture_unit = (next_fallback_texture_unit == 2.0f) ? 3.0f : 2.0f;
        }
        else
        {
            texture_selector = it->second;
        }
883 }
...
931 texture_selector_coefficients.push_back(texture_selector);
...
989 if ( !texture_selector_coefficients.empty() )
    {
        GLuint VBO_texture_selector_coefficients_id;
        glGenBuffers(1, &VBO_texture_selector_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_selector_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_selector_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_selector_coefficients.size() * sizeof(float), texture_selector_coefficients.data());
        location = 3; // "(location = 3)" em "shader_vertex.glsl"
        number_of_dimensions = 1; // float em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
1001 }
```

no `shader_fragment.glsl`
```
15 in float material_id;
...
26 #define CHILL  3
...
37 uniform sampler2D TextureImage3;
...
75 vec3 Kd0 = vec3(1.0, 0.0, 1.0);
...
140 else if ( object_id == CHILL )
    {
        // Coordenadas de textura do Big Chill, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

        // "material_id" aqui recebe diretamente a unidade de textura (2 ou 3).
        Kd0 = (material_id > 2.5)
            ? texture(TextureImage3, vec2(U,V)).rgb
            : texture(TextureImage2, vec2(U,V)).rgb;
150 }
```
no `shader_vertex.glsl`
```
8 layout (location = 3) in float material_coefficients;
...
23 out float material_id;
...
68 material_id = material_coefficients;
```

### Commit com auxílio de IA: Ajuste de rotação no modelo, olhar para frente enquanto anda

PROMPT: 
 to com a seguinte função para a atualização da posição de um modelo em minha aplicação em opengl, c++. quero que a partir dela me diga como posso pegar o angulo de rotação em torno do eixo Y a fim de fazer ele sempre olhar pra onde esta indo

```c
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "globals.h"
#include <bits/stdc++.h>

#define X 0
#define Y 1
#define Z 2

void UpdatePosition() {
//Inputs do jogador

    if (keys[GLFW_KEY_A]) {
        player_pos[X] -= player_speed[X] * delta_t;
    }

    if (keys[GLFW_KEY_D]) {
        player_pos[X] += player_speed[X] * delta_t;
    }

    if (keys[GLFW_KEY_S]) {
        player_pos[Z] += player_speed[Z] * delta_t;
    }

    if (keys[GLFW_KEY_W]) {
        player_pos[Z] -= player_speed[Z]* delta_t;
    }


    if (keys[GLFW_KEY_SPACE] and !jumping){
        keys[GLFW_KEY_SPACE] = false;
        jumping = true;
        player_speed[Y] = jump_speed;
    }

    player_speed[Y] += gravidade;
    player_pos[Y] += player_speed[Y] * delta_t;

    if (player_pos[Y] < -1) { //Aqui defini o chao como -1
        player_pos[Y] = -1;
        player_speed[Y] = 0;
        jumping = false;
    }
} 
```

### Commit com auxílio de IA: Adicionado modelo GLTF do Swampfire e adicionada capacidade de trocar entre modelos de jogadores
PROMPT:
Quero adicionar um modelo GLTF (Swampfire) ao projeto usando a biblioteca `tinygltf`. Siga os seguintes objetivos
Objetivos:
- Incluir `tinygltf` no build e todas as bibliotecas auxiliares necessárias
- Carregar o arquivo .gltf, e extrair meshes, materiais e texturas.
- Integrar ao código existente sem atrapalhar o suporte a OBJ.

Now implement the next changes:
- by starting the program, player will control big chill
- whenever "z" is pressed, player changes between controlling big chill and swampfire, by replacing the controlled model and hiding the other. 
- implementation should be open to addition of new transformations in the future


### Commit com IA: Ajuste na movimentação e orientação do modelo, agora funcionando em relação à câmera e não ao sistema global

PROMPT 1:
Queria saber como q faço par a  minha movimentação e orientação do modelo ser em relação a camera, pois ta bem torto no momento usando o sistema de coordenadas globais, segue a função de atualizar a posição e orientação

```c
void UpdatePosition() {

    float move_x = 0.0f;
    float move_z = 0.0f;

    //Inputs do jogador
    if (keys[GLFW_KEY_A]) move_x -= 1.0f;
    if (keys[GLFW_KEY_D]) move_x += 1.0f;
    if (keys[GLFW_KEY_S]) move_z += 1.0f;
    if (keys[GLFW_KEY_W]) move_z -= 1.0f;

    if (move_x != 0.0f || move_z != 0.0f) {
        float target_angle = atan2(move_x, move_z);

        // Converte para graus se seu motor/matriz usar graus
        player_rotate = target_angle * (180.0f / M_PI);
    }

    if (keys[GLFW_KEY_SPACE] and !jumping){
        keys[GLFW_KEY_SPACE] = false;
        jumping = true;
        player_speed[Y] = jump_speed;
    }

    player_speed[Y] += gravidade;
    player_pos[Y] += player_speed[Y] * delta_t;
    player_pos[X] += move_x * player_speed[X] * delta_t;
    player_pos[Z] += move_z * player_speed[Z] * delta_t;

    if (player_pos[Y] < -1) { //Aqui defini o chao como -1
        player_pos[Y] = -1;
        player_speed[Y] = 0;
        jumping = false;
    }
} 
```

PROMPT 2:
Agora ajeite a rotação do modelo, esta muito dura e incorreta, não está apontando a frente do modelo para o local correto em relação ao vetor de deslocamento.

### Commit com auxílio de IA: Adicionadas animações idle e de movimentação funcionais, demais em DEBUG
1. Atualizar a Leitura do GLTF (gltf_utils.h)

Atualmente, sua função buildTrianglesAndAddToVirtualSceneFromGLTF(gltfmodel) (que não está na main, mas é chamada por ela) provavelmente lê apenas posições (X,Y,Z), normais e texturas (U,V).
Para animações, cada vértice do Ben 10 precisa saber a quais "ossos" ele está grudado e com qual força. Você precisará extrair dois novos atributos dos accessors do GLTF e enviá-los para a GPU via VBOs:

    JOINTS_0: Um vec4 de inteiros indicando os IDs dos 4 ossos que afetam aquele vértice.

    WEIGHTS_0: Um vec4 de floats indicando o "peso" (influência) de cada osso no vértice.
A Solução Ideal:
Remova a parte do stbi_load fixa do Swampfire de dentro dessa função. Deixe a função buildTrianglesAndAddToVirtualSceneFromGLTF ser genérica, usando apenas o texture_id que ela já leu do próprio GLTF. Se a textura do modelo não estiver carregando pelo GLTF automaticamente, é melhor arrumar o path das imagens no arquivo .gltf (que é um JSON e pode ser editado no bloco de notas) do que fazer hardcode no parser.
2. Modificar o Vertex Shader (shader_vertex.glsl)

O seu shader precisará receber os dados dos ossos e calcular a posição final do vértice deformado antes de multiplicar pelas matrizes de projeção e câmera. A lógica básica fica assim:
OpenGL Shading Language

#version 330 core

layout (location = 0) in vec4 model_coefficients; // Posição
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;
// Novos atributos lidos do VAO:
layout (location = 4) in uvec4 jointIds; 
layout (location = 5) in vec4 weights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Array com as matrizes de transformação de todos os ossos do modelo
const int MAX_BONES = 100;
uniform mat4 boneMatrices[MAX_BONES];

void main()
{
    // Calcula a deformação do esqueleto somando a influência dos 4 ossos
    mat4 boneTransform = boneMatrices[jointIds[0]] * weights[0];
    boneTransform     += boneMatrices[jointIds[1]] * weights[1];
    boneTransform     += boneMatrices[jointIds[2]] * weights[2];
    boneTransform     += boneMatrices[jointIds[3]] * weights[3];

    // Aplica a deformação na posição original
    vec4 skinnedPosition = boneTransform * model_coefficients;

    // Transforma para as coordenadas do mundo e da tela
    gl_Position = projection * view * model * skinnedPosition;
}
3. A Lógica Matemática na main.cpp

Dentro do seu while (!glfwWindowShouldClose(window)), utilizando a variável delta_t que você já calcula, você terá que:

    Encontrar os Keyframes: Usar o tempo atual da aplicação para buscar nos dados brutos do tinygltf qual é a translação, rotação (quaternions) e escala de cada "Node" (osso) no momento exato.

    Interpolar: Interpolar esfericamente (Slerp) os quaternions entre o frame passado e o próximo frame para o movimento ser fluido.

    Calcular a Hierarquia: Percorrer a árvore de nós do GLTF multiplicando a matriz local do osso "filho" pela do "pai" (ex: a matriz da mão multiplica pela do antebraço, que multiplica pela do braço).

    Matriz Inversa de Repouso (Inverse Bind Matrix): Multiplicar o resultado final pela matriz de repouso do modelo.

    Enviar para a GPU: Pegar o array de matrizes resultantes e passar para o uniform mat4 boneMatrices via glUniformMatrix4fv antes de chamar o DrawVirtualObject("the_swampfire_0").
Perfeito! Implementar uma máquina de estados baseada nas teclas de movimentação é exatamente o que vai dar "vida" ao seu Swampfire.

No momento, o seu código pega o tempo global (agora) e lê a animação 0 fixamente. Para criar essa máquina de estados, vamos verificar o vetor global de teclado (keys) que você já mapeou na KeyCallback, determinando se o jogador está com o dedo em alguma tecla de movimento (WASD ou setas). Dependendo do resultado, escolhemos a animação 8 (Run) ou 6 (Idle).
O que modificar na sua main.cpp

Vá até a etapa 2 da montagem dos ossos, localizada dentro do if (g_characters[1].visible).

Você vai substituir este trecho antigo:

ANTES:
C++

// 2) Apply animations (if any)
if (!gltfmodel.animations.empty()) {
    float anim_time = fmod(agora, 1000.0f);
    const tinygltf::Animation &anim = gltfmodel.animations[0];
    float max_time = 0.0f;

DEPOIS (Cole este código):
C++

// 2) Apply animations (if any)
if (!gltfmodel.animations.empty()) {
    // 1. Avalia o estado atual do input (WASD ou Setas)
    bool is_moving = keys[GLFW_KEY_W] || keys[GLFW_KEY_A] || 
                     keys[GLFW_KEY_S] || keys[GLFW_KEY_D] ||
                     keys[GLFW_KEY_UP] || keys[GLFW_KEY_DOWN] || 
                     keys[GLFW_KEY_LEFT] || keys[GLFW_KEY_RIGHT];

    // 2. Máquina de estados: Se estiver movendo, corre (8). Se não, fica parado (6).
    int current_anim_index = is_moving ? 8 : 6;

    // Garante que o índice não vai estourar o limite de animações carregadas
    if (current_anim_index >= gltfmodel.animations.size()) {
        current_anim_index = 0; 
    }

    float anim_time = fmod(agora, 1000.0f);
    
    // 3. Carrega a animação selecionada pelo estado
    const tinygltf::Animation &anim = gltfmodel.animations[current_anim_index];
    float max_time = 0.0f;

Como isso funciona na prática

    A Leitura: A cada frame gerado no while, o programa olha para o array keys. Como a sua função KeyCallback preenche esse array instantaneamente com true no pressionamento e false ao soltar a tecla, o is_moving vira um gatilho muito preciso.

    O Tempo (agora): O cálculo da interpolação de frames que você já programou (anim_time = fmod(agora, max_time)) continuará funcionando perfeitamente! Como a animação 8 e a 6 têm durações (max_time) diferentes, o fmod fará o loop se adaptar dinamicamente ao tamanho da animação atual em milissegundos.

Basta compilar o código novamente. Agora, toda vez que você pressionar uma tecla de andar, o modelo assumirá a postura de corrida, e ao soltar, ele voltará para o ciclo de respiração idle.
GL_CLAMP_TO_EDGE



### Commit com IA: Desenho de um chão infinito para ficar mais legal a gameplay

PROMPT:
Quero que me de alternativas de como posso implementar uma textura "infinita" para o jogador, atualmente ele ta so flutuando em um pequeno plano.
Preferencialemnte, seria interessante manipular o modelo obj e a textura que já está presente na cena atual.

### Commit com IA: Colocar um novo modelo no mapa para futuramente testar tanto colisão como modelos de iluminação e double jump

PROMPT 1:
Qual seria o passo a passo para eu poder adicionar um novo modelo obj e poder colocar ele na cena, semelhante ao plano em @src/main.cpp.
Considerando que seria possível ter um .mtl vindo junto do .obj e textura

PROMPT 2: 
Ent, esse meu obj, toda vez q mudo pra usar o modelo do swampfire, ele usa a textura dele no bloco, pq usa a textureImage4, tem como eu colocar a 5 direto? O swampfire usa gltf pra carregar o modelo, q eh diferente do carregamento dos modelos obj  

### Commit com IA: Implementando colisão superior com as caixas  a fim de implementar um plataforming

PROMPT 1: 
Nessa minha aplicação em opengl eu tenho q montar parkour em plataforma, qual seria a melhor forma de implementar isso? Tendo em mente verificação de colisao e tals.

PROMPT 2:
seguinte, to tendando implementar colisoes, pĺataformas pro jogador poder pular e fazer parkour, so q n ta funcionando, olhe as funcoes em @src/player.cpp 
@src/platforms.cpp e na @src/main.cpp

PROMPT 3:
To usando a biblioteca tinyobj, me defina uma função que me de as dimensões de cada modelo, para que eu possa montar uma hitbox.

PROMPT 4:
Acredito que a lógica de colisão não está muito certa pois o centro do personagem não parece estar no pé do personagem, me de formas que eu possa solucionar esse problema.

### Commit com IA: Animações refinadas para pulo e ataques, animation.cpp criado para modularização
Objetivo: Refatorar a lógica de Skeletal Animation (Skinning) que está atualmente embutida no loop de renderização do main.cpp. Devemos mover essa lógica para uma classe modular em arquivos separados (animation.h e animation.cpp), limpando a main.cpp.

Por favor, execute estritamente os 3 passos abaixo:
Passo 1: Criar include/animation.h

Crie um novo arquivo chamado animation.h na pasta include/ com a seguinte definição de classe:
C++

#pragma once

#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

class GltfAnimator {
public:
    // Construtor: Prepara a hierarquia de nós e as matrizes de repouso (Inverse Bind Matrices)
    GltfAnimator(const tinygltf::Model& model);

    // Update: Calcula a interpolação do frame atual e atualiza as matrizes dos ossos
    void update(const tinygltf::Model& model, int anim_index, float current_time);

    // Retorna as matrizes prontas para a GPU
    const std::vector<glm::mat4>& getBoneMatrices() const { return boneMatrices; }

private:
    std::vector<int> node_parent;
    std::vector<glm::mat4> inverse_bind_matrices;
    std::vector<glm::mat4> boneMatrices;
};

Passo 2: Criar src/animation.cpp

Crie um novo arquivo chamado animation.cpp na pasta src/ implementando a lógica exata que tínhamos na main:
C++

#include "animation.h"
#include <algorithm>

GltfAnimator::GltfAnimator(const tinygltf::Model& model) {
    // 1. Extrair hierarquia de pais (node_parent)
    node_parent.assign(model.nodes.size(), -1);
    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        const tinygltf::Node &n = model.nodes[ni];
        for (int c : n.children) {
            if (c >= 0 && c < (int)model.nodes.size()) {
                node_parent[c] = (int)ni;
            }
        }
    }

    // 2. Extrair Inverse Bind Matrices da Skin 0 (assumindo 1 skin principal)
    if (!model.skins.empty()) {
        const tinygltf::Skin &skin = model.skins;
        if (skin.inverseBindMatrices >= 0) {
            const tinygltf::Accessor &acc = model.accessors[skin.inverseBindMatrices];
            const tinygltf::BufferView &bv = model.bufferViews[acc.bufferView];
            const tinygltf::Buffer &buf = model.buffers[bv.buffer];
            const float *mats = reinterpret_cast<const float*>(&buf.data[bv.byteOffset + acc.byteOffset]);
            
            inverse_bind_matrices.reserve(acc.count);
            for (size_t i = 0; i < acc.count; ++i) {
                inverse_bind_matrices.push_back(glm::make_mat4(&mats[16 * i]));
            }
        }
    }
}

void GltfAnimator::update(const tinygltf::Model& model, int anim_index, float current_time) {
    if (model.skins.empty()) return;
    const tinygltf::Skin &skin = model.skins;

    // 1) Extrai T, R, S base originais para TODOS os ossos
    std::vector<glm::vec3> node_T(model.nodes.size(), glm::vec3(0.0f));
    std::vector<glm::quat> node_R(model.nodes.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    std::vector<glm::vec3> node_S(model.nodes.size(), glm::vec3(1.0f));
    std::vector<bool> node_has_matrix(model.nodes.size(), false);
    std::vector<glm::mat4> local_matrix(model.nodes.size(), glm::mat4(1.0f));

    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        const tinygltf::Node &n = model.nodes[ni];
        if (n.matrix.size() == 16) {
            node_has_matrix[ni] = true;
            local_matrix[ni] = glm::make_mat4(n.matrix.data());
        } else {
            if (n.translation.size() == 3) node_T[ni] = glm::vec3(n.translation, n.translation, n.translation);
            if (n.rotation.size() == 4) node_R[ni] = glm::quat(n.rotation, n.rotation, n.rotation, n.rotation);
            if (n.scale.size() == 3) node_S[ni] = glm::vec3(n.scale, n.scale, n.scale);
        }
    }

    // 2) Aplica a animação
    if (!model.animations.empty() && anim_index >= 0 && anim_index < model.animations.size()) {
        float anim_time = fmod(current_time, 1000.0f);
        const tinygltf::Animation &anim = model.animations[anim_index];
        
        float max_time = 0.0f;
        std::vector<std::vector<float>> sampler_inputs(anim.samplers.size());
        std::vector<int> sampler_output_accessor(anim.samplers.size(), -1);
        
        for (size_t si = 0; si < anim.samplers.size(); ++si) {
            const auto &samp = anim.samplers[si];
            if (samp.input >= 0) {
                const tinygltf::Accessor &acc = model.accessors[samp.input];
                const tinygltf::BufferView &bv = model.bufferViews[acc.bufferView];
                const tinygltf::Buffer &buf = model.buffers[bv.buffer];
                const float *times = reinterpret_cast<const float*>(&buf.data[bv.byteOffset + acc.byteOffset]);
                sampler_inputs[si].assign(times, times + acc.count);
                if (!sampler_inputs[si].empty()) max_time = std::max(max_time, sampler_inputs[si].back());
            }
            sampler_output_accessor[si] = samp.output;
        }
        if (max_time > 0.0f) anim_time = fmod(current_time, max_time);

        for (const auto &ch : anim.channels) {
            int samp_idx = ch.sampler;
            if (samp_idx < 0 || samp_idx >= (int)anim.samplers.size()) continue;
            const auto &inputs = sampler_inputs[samp_idx];
            if (inputs.empty()) continue;
            
            size_t k = 0; while (k + 1 < inputs.size() && anim_time > inputs[k+1]) ++k;
            size_t k1 = std::min(k + 1, inputs.size()-1);
            float t0 = inputs[k], t1 = inputs[k1];
            float local_t = (t1 - t0) > 0.0f ? (anim_time - t0) / (t1 - t0) : 0.0f;

            int outAccIdx = sampler_output_accessor[samp_idx];
            if (outAccIdx < 0) continue;
            const tinygltf::Accessor &outAcc = model.accessors[outAccIdx];
            const tinygltf::BufferView &outBV = model.bufferViews[outAcc.bufferView];
            const tinygltf::Buffer &outBuf = model.buffers[outBV.buffer];
            const float *outData = reinterpret_cast<const float*>(&outBuf.data[outBV.byteOffset + outAcc.byteOffset]);

            size_t compCount = (outAcc.type == TINYGLTF_TYPE_VEC3) ? 3 : (outAcc.type == TINYGLTF_TYPE_VEC4 ? 4 : 1);
            const float *v0 = &outData[k * compCount];
            const float *v1 = &outData[k1 * compCount];

            int nodeIdx = ch.target_node;
            if (nodeIdx < 0 || nodeIdx >= (int)model.nodes.size()) continue;

            if (ch.target_path == "translation") {
                glm::vec3 t0v(0.0f), t1v(0.0f);
                for (size_t c = 0; c < compCount && c < 3; ++c) { t0v[c] = v0[c]; t1v[c] = v1[c]; }
                node_T[nodeIdx] = glm::mix(t0v, t1v, local_t);
                node_has_matrix[nodeIdx] = false;
            } else if (ch.target_path == "scale") {
                glm::vec3 s0v(1.0f), s1v(1.0f);
                for (size_t c = 0; c < compCount && c < 3; ++c) { s0v[c] = v0[c]; s1v[c] = v1[c]; }
                node_S[nodeIdx] = glm::mix(s0v, s1v, local_t);
                node_has_matrix[nodeIdx] = false;
            } else if (ch.target_path == "rotation") {
                glm::quat q0(1.0f,0.0f,0.0f,0.0f), q1(1.0f,0.0f,0.0f,0.0f);
                if (compCount >= 4) { q0 = glm::quat(v0, v0, v0, v0); q1 = glm::quat(v1, v1, v1, v1); }
                node_R[nodeIdx] = glm::normalize(glm::slerp(q0, q1, local_t));
                node_has_matrix[nodeIdx] = false;
            }
        }
    }

    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        if (!node_has_matrix[ni]) {
            local_matrix[ni] = glm::translate(glm::mat4(1.0f), node_T[ni]) 
                             * glm::mat4_cast(node_R[ni]) 
                             * glm::scale(glm::mat4(1.0f), node_S[ni]);
        }
    }

    // 3) Ordem Topológica e Matrizes Globais
    std::vector<glm::mat4> global_matrix(model.nodes.size(), glm::mat4(1.0f));
    std::vector<bool> matrix_computed(model.nodes.size(), false);

    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        if (matrix_computed[ni]) continue;

        std::vector<int> path;
        int curr = ni;
        while (curr != -1 && !matrix_computed[curr]) {
            path.push_back(curr);
            curr = node_parent[curr];
        }

        for (int i = (int)path.size() - 1; i >= 0; --i) {
            int node = path[i];
            int p = node_parent[node];
            if (p == -1) {
                global_matrix[node] = local_matrix[node];
            } else {
                global_matrix[node] = global_matrix[p] * local_matrix[node];
            }
            matrix_computed[node] = true;
        }
    }

    // 4) Matrizes finais (Bone Matrices)
    size_t jointCount = skin.joints.size();
    size_t uploadCount = std::min<size_t>(jointCount, 100);
    boneMatrices.assign(uploadCount, glm::mat4(1.0f));
    
    for (size_t j = 0; j < uploadCount; ++j) {
        int nodeIdx = skin.joints[j];
        glm::mat4 invBind(1.0f);
        if (j < inverse_bind_matrices.size()) invBind = inverse_bind_matrices[j];
        boneMatrices[j] = global_matrix[nodeIdx] * invBind;
    }
}

Passo 3: Limpar a main.cpp

No arquivo src/main.cpp:

    Adicione #include "animation.h" no topo do arquivo junto aos outros includes locais.

    Localize a região (linha ~380 a ~415) onde estão as implementações antigas de // --- Prepare inverse bind matrices per skin --- e // Build parent map for nodes. Apague todo esse bloco, pois ele agora pertence ao construtor do GltfAnimator.

    Imediatamente antes do while (!glfwWindowShouldClose(window)), instancie o animador:
    C++

    GltfAnimator swampfireAnimator(gltfmodel);

    Dentro do if (g_characters.visible), localize toda a lógica complexa que acabamos de refatorar (da linha if (!gltfmodel.skins.empty()) { até o envio das variáveis para g_bone_matrices_uniform).
    Substitua TUDO isso por apenas estas linhas limpas:
    C++

    bool is_moving = keys[GLFW_KEY_W] || keys[GLFW_KEY_A] ||
                     keys[GLFW_KEY_S] || keys[GLFW_KEY_D] ||
                     keys[GLFW_KEY_UP] || keys[GLFW_KEY_DOWN] ||
                     keys[GLFW_KEY_LEFT] || keys[GLFW_KEY_RIGHT];

    int current_anim_index = is_moving ? 8 : 6;

    // Atualiza a animação usando o módulo modular
    swampfireAnimator.update(gltfmodel, current_anim_index, agora);

    // Envia para a placa de vídeo
    if (g_bone_matrices_uniform >= 0) {
        glUniformMatrix4fv(g_bone_matrices_uniform, 
                           swampfireAnimator.getBoneMatrices().size(), 
                           GL_FALSE, 
                           glm::value_ptr(swampfireAnimator.getBoneMatrices()));
    }

OBS: Lembre-se de adicionar src/animation.cpp no seu CMakeLists.txt na lista de arquivos a serem compilados junto com o main.cpp!

Objetivo: Atualizar a main.cpp para incluir uma máquina de estados de animação avançada para o personagem Swampfire, atendendo a 3 requisitos:

    Animação 3 ao segurar 'Q', trocando para a animação 4 ao soltar.

    Animação 0 ao pular, com o tempo atrelado à duração do pulo.

    O personagem não deve se mover (congelar UpdatePosition()) enquanto ataca.

Por favor, faça apenas as 3 substituições abaixo na main.cpp:
Substituição 1: Variáveis de Estado

Localize a linha float anterior = (float)glfwGetTime(); (pouco antes do while (!glfwWindowShouldClose(window))).
Logo abaixo dela, adicione as seguintes variáveis de controle:
C++

    float anterior = (float)glfwGetTime();

    // Variáveis de estado da máquina de animação
    int q_state = 0; // 0=livre, 1=segurando Q, 2=soltou Q (anim 4 rodando)
    float q_release_time = 0.0f;
    float jump_timer = 0.0f;
    bool is_attacking = false;

Substituição 2: Travar Movimento no Ataque

Localize a linha que chama a função de movimento: UpdatePosition(); (linha 609).
Substitua essa linha pelo seguinte bloco condicional:
C++

        // O personagem só pode se mover se não estiver no meio de um ataque
        if (!is_attacking) {
            UpdatePosition();
        }

Substituição 3: A Nova Máquina de Estados

Localize o bloco da máquina de estados antiga do Swampfire (que começa com bool is_moving = ... e vai até a chamada do swampfireAnimator.update(...)).
Substitua todo esse bloco por esta nova lógica:
C++

                    bool is_moving = keys[GLFW_KEY_W] || keys[GLFW_KEY_A] ||
                                     keys[GLFW_KEY_S] || keys[GLFW_KEY_D] ||
                                     keys[GLFW_KEY_UP] || keys[GLFW_KEY_DOWN] ||
                                     keys[GLFW_KEY_LEFT] || keys[GLFW_KEY_RIGHT];

                    int current_anim_index = 6; // Padrão: Idle
                    is_attacking = false;
                    float anim_time_to_pass = agora; // O tempo que será passado para a animação

                    // --- MÁQUINA DE ESTADOS AVANÇADA ---

                    // 1. Ataque com E
                    if (keys[GLFW_KEY_E]) {
                        current_anim_index = 5;
                        is_attacking = true;
                    }
                    // 2. Ataque com Q (Segurar = 3, Soltar = 4)
                    else if (keys[GLFW_KEY_Q]) {
                        current_anim_index = 3;
                        q_state = 1;
                        is_attacking = true;
                    } else if (q_state == 1) {
                        // O jogador acabou de soltar a tecla Q
                        q_state = 2; 
                        q_release_time = agora;
                        current_anim_index = 4;
                        is_attacking = true;
                    } else if (q_state == 2) {
                        // Mantém a animação 4 rodando por um tempo fixo (ex: 0.8s) para finalizar o golpe
                        if (agora - q_release_time < 0.8f) {
                            current_anim_index = 4;
                            is_attacking = true;
                        } else {
                            q_state = 0; // Terminou o ataque
                        }
                    }

                    // 3. Pulo (Interpola do início ao fim)
                    if (!is_attacking && jumping) {
                        current_anim_index = 0;
                        jump_timer += delta_t;
                        
                        // Em vez do tempo global contínuo (agora), passamos o timer do pulo.
                        // O multiplicador '1.5f' ajusta a velocidade da animação. Aumente se quiser que a cambalhota gire mais rápido.
                        anim_time_to_pass = jump_timer * 1.5f; 
                    } else if (!jumping) {
                        jump_timer = 0.0f; // Reseta o pulo quando tocar no chão
                    }

                    // 4. Corrida / Idle
                    if (!is_attacking && !jumping) {
                        if (is_moving) {
                            current_anim_index = 8;
                        } else {
                            current_anim_index = 6;
                        }
                    }

                    // Atualiza a animação enviando o tempo correto (anim_time_to_pass em vez de agora)
                    swampfireAnimator.update(gltfmodel, current_anim_index, anim_time_to_pass);

a nossa máquina de estados precisa de cronômetros locais para que as animações de ataque resetem para o início quando engajadas, e a animação de pulo precisa parar no último frame em vez de entrar em loop (fazer fmod). Faça as seguintes alterações no arquivo main.cpp e crie as variáveis de tempo local necessárias:"
Passo 1: Atualizar as Variáveis de Inicialização

Localize o bloco de variáveis de estado que criamos antes (perto da inicialização do GltfAnimator) e substitua por este conjunto atualizado, que agora inclui trackers de tempo local e controle de qual era a animação anterior:
C++

    // Variáveis de estado da máquina de animação
    int q_state = 0; // 0=livre, 1=segurando Q, 2=soltou Q (anim 2 rodando)
    float q_release_time = 0.0f;
    float jump_timer = 0.0f;
    bool is_attacking = false;

    // NOVOS CONTROLES: Cronômetros locais para resetar animações
    int last_applied_anim_index = -1;
    float anim_start_time = 0.0f;

Passo 2: Atualizar a Lógica da Máquina de Estados

Substitua todo o bloco de código do Swampfire dentro de if (g_characters.visible) (desde a verificação de is_moving até a chamada do swampfireAnimator.update) por esta versão corrigida. Ela calcula um tempo local baseado em quando a animação começou, e limita o tempo do pulo para não dar loop:
C++

            bool is_moving = keys[GLFW_KEY_W] || keys[GLFW_KEY_A] ||
                             keys[GLFW_KEY_S] || keys[GLFW_KEY_D] ||
                             keys[GLFW_KEY_UP] || keys[GLFW_KEY_DOWN] ||
                             keys[GLFW_KEY_LEFT] || keys[GLFW_KEY_RIGHT];

            int current_anim_index = 6; // Padrão: Idle
            is_attacking = false;

            // --- MÁQUINA DE ESTADOS AVANÇADA ---

            // 1. Ataque com E (Animação 1)
            if (keys[GLFW_KEY_E]) {
                current_anim_index = 1;
                is_attacking = true;
            }
            // 2. Ataque com Q (Segurar = 3, Soltar = 2)
            else if (keys[GLFW_KEY_Q]) {
                current_anim_index = 3;
                q_state = 1;
                is_attacking = true;
            } else if (q_state == 1) {
                // O jogador acabou de soltar a tecla Q -> Ativa a animação 2 IMEDIATAMENTE
                q_state = 2; 
                q_release_time = agora;
                current_anim_index = 2;
                is_attacking = true;
            } else if (q_state == 2) {
                // Mantém a animação 2 rodando por 0.8s
                if (agora - q_release_time < 0.8f) {
                    current_anim_index = 2;
                    is_attacking = true;
                } else {
                    q_state = 0; // Terminou o combo, volta a ficar livre
                }
            }

            // 3. Pulo (Animação 0)
            if (!is_attacking && jumping) {
                current_anim_index = 0;
            }

            // 4. Corrida / Idle
            if (!is_attacking && !jumping) {
                if (is_moving) {
                    current_anim_index = 8;
                } else {
                    current_anim_index = 6;
                }
            }

            // --- GERENCIAMENTO DE TEMPO LOCAL (RESET DE ANIMAÇÃO) ---
            // Se a animação mudou neste frame, marcamos o tempo atual como o início dela
            if (current_anim_index != last_applied_anim_index) {
                anim_start_time = agora;
                last_applied_anim_index = current_anim_index;
                if (current_anim_index == 0) {
                    jump_timer = 0.0f; // Garante o reset do cronômetro do pulo
                }
            }

            // Calcula o tempo que passou especificamente dentro desta animação
            float anim_time_to_pass = agora - anim_start_time;

            // Tratamento especial para o Pulo: Não deixa entrar em loop (Clamp no tempo máximo)
            if (current_anim_index == 0) {
                jump_timer += delta_t;
                float speed_multiplier = 1.3f; // Ajuste a velocidade da cambalhota aqui
                anim_time_to_pass = jump_timer * speed_multiplier;

                // Descobre a duração máxima da animação 0 para travar o frame no final
                if (!gltfmodel.animations.empty() && !gltfmodel.animations.channels.empty()) {
                    int sampler_idx = gltfmodel.animations.channels.sampler;
                    int input_accessor_idx = gltfmodel.animations.samplers[sampler_idx].input;
                    float max_duration = gltfmodel.accessors[input_accessor_idx].maxValues;
                    
                    // Se o tempo do pulo estourar a duração da animação, trava no último frame!
                    if (anim_time_to_pass > max_duration) {
                        anim_time_to_pass = max_duration; 
                    }
                }
            }

            // Atualiza o esqueleto com o tempo local e corrigido
            swampfireAnimator.update(gltfmodel, current_anim_index, anim_time_to_pass);


### Commit com IA: Bounding boxes para melhor entendimento das colisões

PPROMPT 1:
Na @src/main.cpp a sttruct objmodel tem a definição dos vertices de uma bounding box, quero desenha-las ao redor de cada modelo, como faço?


### Commit com IA: Modularização das funções de carregamento de modelos GLTF, adição de projectiles.cpp e particles.cpp, além de refatoração geral

PROMPT 1:
make the animation used by swampfire be a modular function call from animation.cpp
also make this gltf loading function a function call, modularizing it so I can add the fireball later

PROMPT 2:
add the fireball with its animation, animate the fireball, make so the fireball only appears when "q" is released, and its size is proportional to the amout of time that the player has held it, up to a limit

PROMPT 3:
add the fireball stuff into projectiles.cpp, making it as modularized as possible

PROMPT 4:
erase all references to a fireball gltf. make it a solid orange sphere instead, but keep all handling of projectiles to its module
add a particles.cpp where particles will be handled. make the sphere be yellow and emit fire-like particles

PROMPT 5:
make the whole particle system more modularized, function calling including parameters for the particles (color, life, age, etc.). change particle color to use hex strings

### Commit com IA: Finalmente uma colisao que funciona vamoooo

PROMPT: I need to correct the logic in collision. Now, the player and bbox are not doing the same movement, the bbox dont move the same as the pĺayer. Read the files and give me an explanations of why is that and the solution


### Commit com IA: Implementacao de uma bbox temporaria no modelo do Swampfire e correcao de bugs relativos a gravidade e animacoes desse modelo em cima de plataformas do mapa.

PROMPT 1: So, i want to debug some stuff. I want you to just tell me what to change, dont apply anything to the files. The swampfire model, when on top of a platform, the clipping position is ok, but the animation is bugged, it keeps going to 2 diferent animations, i believe it is the jumping and idle animation.

PROMPT 2: Okay, but i have another problem, the way thar is implemented now, the ground doesnt have a collider, so even when idle on the ground, both models aabbs are going to through the floor, where do i change this? dont change any file.

### Commit com IA: Arrumado o bug de binding de texturas ao trocar para o Swampfire

PROMPT: Now I have another debug I need to do, dont change any of the files, I want to know where to change. When I press Z to change from the bigchill obj model to the swampfire gltf model, the texture of the platform change to some part of the swampfire model, it doesnt keep its own. Show me why this happens and where to change 


### Commit com IA: Coloquei a bbox tanto no inimigo quanto no projetil, tamo colidindo ja e banindo da existencia o projetil quando isso ocorre

PROMPT: Now I want to do a lot of stuff related to the projectile. Just tell me where to change and why, dont change any of the files
1. I want to add a collider to the projectile, an AABB that have its position updated alongside the drawing of the model.
2. This collider should cease its existence when the projectile cease existence
3. If the collider touches another AABB, that isnt the players, it should die (this is for the future, the projectile is an attack)

### Commit com IA: Implementacao dos ataques do modelo do swampfire. Criacao de hit com os inimigos e hitbox temporaria.

PROMPT: Now I want to implement a new thing, attacks. For now lets just focus on Swampfire attacks, since its the model with animatios. 
1. I want to create a new bbox for a especific time during its atack (bound to the E key).
1.1. The attack will be 2 bounding boxes, because the model throws 2 punches 
2. It will be drawn using de DrawBoundingBox function for now to debug it if necessary.
3. The only thing that we will test it collision is the enemy, for now lets just printf() a phrase indicating the collision occured.
4. If that bbox collided with an enemy, that bbox cannot damage the same enemy twice.
ONLY TELL ME WHAT TO DO, DONT CHANGE ANY FILES

### Commit com IA: Modelo do Ben adicionado com animações e leves ajustes no ataque do swampfire 

PROMPT 1: Make so the project is able to load .glb files as well. Then apply it to load the "ben_tennyson.glb" model on /data and add it to the transformation cycle

PROMPT 2: corrija as animações do swampfire, ele deve pelo menos ir até o fim da animação de soco após cada toque em "e"

### Commit com IA: Coloquei o ataque do Ben ja com a hitbox e arrumei as proporcoes da bbox dele 

PROMPT: I want to add the same method as the swampfire attack for the ben tennyson model. This one doesnt have more than 1 punch, so it will be just one. Just tell me what needs to be done in the files, where to change. DO NOT CHANGE DIRECTLY THE FILES

### Commit com IA: Movimentacao e ataques dos inimigos funcionando para o modelo obj atual

PROMPT: Now we are going to do a new thing, add some logic to the enemies.
1. Add the collision logic to the movement of the enemies, I dont wanna see them going into the blocks
2. I want to add an attack for them
2.1 This attack will happen once its distance to the player met X or the bboxes intersects (give me the 2 options so I can test the better)
2.2 The attack will happen for a period of time and should have a bbox, similar to ben and swampfire attack.
2.3 It is a single hit attack
3. For now the enemy is just the bunny, but keep in mind that later we will have a model gltf for them, so keep in mind this for future implementations
Do not change any of the files, give me the changes necessary and i will add them to the code.  

### Commit com IA: Cavaleiros Eternos adicionados como inimigos, assim como estrutura geral de combate, com saúde, dano e morte

PROMPT 1: faça o inimigo carregado ser o testforeverknight.glb em data. Carregue suas animações e textura também.

PROMPT 2:
Implemente um sistema de saúde para os inimigos exibido em texto acima deles. Ao sofrer dano de diferentes ataques (cada um com seu próprio valor de dano), o inimigo deve cancelar ações, tocar uma animação de reação (flinch) e não atacar. Ao zerar a vida, ele deve tocar a animação de morte apenas uma vez, piscar na tela e realizar um respawn em um local aleatório próximo.

PROMPT 3:
Crie hitboxes de ataque em AABB que respeitem o tempo exato das animações. A hitbox só deve ficar ativa durante os frames de impacto (ex: apenas nos 80% do meio da animação do soco) e deve ser rotacionada corretamente para onde o modelo do inimigo ou do jogador está apontando.

PROMPT 4:
Faça com que os projéteis (bolas de fogo) causem dano aos inimigos de forma dinâmica e proporcional ao seu tamanho (escala) no momento do impacto.

PROMPT 5:
Limpe o arquivo main.cpp. Separe e mova as funções de mecânica de jogo, processamento de hitboxes de ataque, aplicação de dano e atualização de estados para seus próprios arquivos dedicados (como player.cpp e enemies.cpp), usando headers para conectar as dependências.

PROMPT 6: Crie uma nova animação de caminhada e adicione-a diretamente ao arquivo do modelo 3D do cavaleiro, preservando todas as animações nativas. Para isso, crie um script em Python que manipule a estrutura binária do GLB, mesclando os canais de uma animação existente para a parte superior do corpo garantindo que a espada fique perfeitamente colada na mão — com cálculos para simular o movimento das pernas no ciclo de caminhada.

PROMPT 7: Adicione a última animação de etargeted_animations(1) para ben_tennyson.glb, e use essa animação como animação de flinch em caso de levar dano

PROMPT 8: Ao acertar um soco, a velocidade de ataque de ben deve duplicar, até que ben dê 10 socos por segundo. A animação ficará mais rápida.

PROMPT 9: O ataque inimigo agora dá dano no jogador, aplicando um knockback, tocando uma animação de reação de dor (flinch) e forçando o jogador a rotacionar imediatamente para encarar o inimigo que o atingiu.

PROMPT 10: Crie a mecânica de morte do jogador. Ao ter a vida zerada, independentemente de qual alien esteja ativo, o personagem deve forçar uma transformação de volta para o Ben (emitindo partículas na tela). O Ben tocará sua animação de morte (cancelando eventuais forças de pulo) e o jogo exibirá uma tela de "Você morreu!", esperando alguns segundos antes de resetar os inimigos e dar respawn no jogador.

PROMPT 11: Atualize o motor de movimentação e física. Garanta que a gravidade continue atuando mesmo enquanto o jogador está preso no ar em animações como flinch ou morte (não o deixando congelado voando). Adicione também uma varredura instantânea de colisões que "empurre" automaticamente o jogador para fora de blocos/paredes do cenário caso ele se transforme e sua nova hitbox acabe engolindo uma estrutura.
                          
### Commit com IA: Adicionei o modelo do castelo assim como a renderizacao dele
PROMPT: I added a new model in the data directory, it is a gltf castle. I believe it does not have any animation. I need to put it in the game, create me an implementation plan to put it into my game and implement it.

### Commit com IA: Finalmente temos um mapa mais decente, ajustes de spawn de inimigos, player e projeteis

PROMPT 1: I want now to create the map for the game. we already have some asstes in the data folder, but the idea is as the follows:       
   1. I will use blender to put some elements in a scene.
   2. I need to not just render all in one obj, i was thinking of making some sort of mapping in a file (txt, json whatever) to render in my scene during the game.           
How can I do that? give me a implementation plan

PROMPT 2: little change of plans, I will first make a model for the ground of my game. I need tyou to create a implementation plan for the part where I already have a model in blender, export, and put into my game. The game needs to have the collisions with it

PROMPT 3: eu adicionei la o meu modelo, agora preciso que me direcione para renderizar ele na tela, mexendo na parte do opengl caso necessario, ja instanciei as novas texturas que ele devera usar q sao duas, TextureImage 9 e 10. Nao mude nada nos arquivos, me diga oq deve mudar

PROMPT 4: eles estao spawnando errado, coloque os para spawnar apenas no Z negativo, me diga onde tera que mudar e n mude nd


### Commit com IA: Chao do mapa praticamente implementado com texturas

PROMPT 1: novamente vamos fazer mais coisas agoraaaaa. Seguinte, modelei mais coisas dentro do meu mapa. expandi a parte caminhavel, alem de adicionar novas texturas e um modelo. Preciso que vc va ate o meu @data/map_ground/chao_mapa.mtl identifique quais texturas n estao sendo carregadas, as carregue, coloque o modelo do barco para ser desenhado tambem, ele eh usado dentro do obj.
Nao cometa erros

PROMPT 2: nao quero que me desenhe os colliders inclusive, somente os cilindros, barco, Plane e Plane2. Fui rodar e deu erro para o modelo do barco

PROMPT 3: ta bem bugadas as texturas, os cilindros tao usando textura errada, um dos chaos tambem, o barco ta bem paia tb. Lembre que o modelo do barco é um gltf, n sei se muda algo na hora de renderizar


### Commit com IA: Ajuste na gravidade, versão protótipo da barra de saúde adicionada, saúde corrigida para ser equivalente entre transformações e coletáveis de saúde adicionados

PROMPT 1: atualmente, delta não está sendo executado da maneira correta, levando aos movimentos do jogador (incluindo pulo) serem dependentes da taxa de quadros do sistema. Conserte isso

PROMPT 2: adicione coletáveis, pequenos orbes vermelhos que serão dropados por inimigos e curarão a vida do jogador. na verdade devem ser várias bolinhas pequenas que caem no chão, e não podem curar o jogador acima de sua vida máxima. Elas devem ter um efeito magnético se o jogador chegar perto, e começar a piscar para depois sumir se o jogador não pegar elas a tempo. Elas devem ser meio transparentes .os coletáveis não precisam cair TÃO Longe. mova a lógica para collectibles.cpp

PROMPT 3:adicione uma barra de vida vermelha na esquerda. Ela que acompanhará a vida do jogador. Use o modelo nesse png, a barra vermelha

### Commit com IA: Ajuste na barra de vida, ajuste no posicionamento do swampfire, protótipo do modelo animado do big chill, adição e melhoria de ataques, mudança nas partículas

PROMPT 1: a barra de vida não está sendo exibida corretamente

PROMPT 2: adicione splash damage na bola de fogo do swampfire - ela dá dano em área agora.
também permita que o jogador gire o swampfire enquanto está atirando bolas de fogo. Também não permita que o jogador inicie outra bola de fogo enquanto não tiver lançado a primeira

PROMPT 3: carregue o modelo bigchillcloaked.glb  no mapa

PROMPT 4: Aplique as animações atuais no modelo para cada uma das suas funções. 

PROMPT 5: ao invés da grande hitbox em área para o ataque especial do big chill, ele deverá, conforme o jogador segura q, lançar uma ice breath usando o sistema de partículas
a ice breath vai dar dano baixo, mas vai principalmente fazer o seguinte com os inimigos:
- deixá-los mais lentos por um período, tanto em movimento quanto ataque
- deixá-los com um leve hue shift azul claro para indicar que estão congelados

PROMPT 6: as partículas, no geral, parecem bolhas. aqui vão algumas sugestões:
1. You’re drawing spheres (or point sprites with depth lighting)

If you render particles as:

small 3D spheres, or
GL_POINTS with lighting enabled

they will naturally look like shiny bubbles.

Fix

Use billboards (quads facing camera) instead of 3D geometry:

Each particle = 2 triangles (a quad)
Always rotate to face the camera
2. Blending mode is wrong

Bad blending makes particles look like glass beads.

Common mistake:
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

This is fine for UI, but can look “bubbly” if lighting is also on.

Better for particles (fire/smoke/light):
glBlendFunc(GL_SRC_ALPHA, GL_ONE);

(Additive blending often removes the “bubble glass” look)

3. Lighting is affecting particles

If you forgot to disable lighting for particle shaders:

specular highlights = bubble look
Fix:
Disable lighting in particle shader
Or set:
specular = 0
shininess = 0
4. Using GL_POINTS with default point sprites

If you use:

glEnable(GL_POINT_SPRITE);

and don’t control texture + attenuation properly, points can look like shiny beads.

Fix:
Use a proper quad mesh instead of GL_POINTS
Or control point size attenuation:
glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, ...);
5. Texture issue (very common)

If your particle texture is:

a white circle
too sharp edge
no soft falloff

it will look like soap bubbles.

Fix:

Use a soft radial gradient texture (faded edges, not solid circle)

PROMPT 7: faça com que, quando o big chill estiver no ar, ele troque pro modelo big_chill_uaf.glb. Durante a troca de modelo, gere uma nuvem de partículas como se fosse fumaça, para a transição

### Commit com IA: UI completa, menu principal, barras de vida, especial e transformação, histórico de ataques, tela de morte/vitória e carregamento

PROMPT 1: No arquivo usado para gerar a health bar, temos outras duas barras. Adicione-as na interface. Depois implementaremos para o que elas serão usadas
a barra cinza ao lado serve de container para essas duas barras amarela e verde. arrume isso


PROMPT 2: as novas barras farão o seguinte:
- VERDE - barra de transformação. Essa barra diminuirá lentamente enquanto ben estiver transformado em um alien. Essa barra subirá rapidamente quando ben não estiver transformado. Ben só pode se transformar se ela estiver cheia. Quando a barra esgotar enquanto ben for alien, ele voltará a ser humano imediatamente.

PROMPT 3: modifique a lógica de transformação! a todo tempo, um alien estará selecionado ( indicado em texto no canto superior direito - remova toda a lógica de sliders). Apertar "x" trocará entre esses aliens. Quando ben se transformar, vai virar o alien selecionado. Se apertar "z" enquanto transformado, volta a ser o ben. O botão "x" só vai funcionar enquanto você for o ben.

PROMPT 4: a fumaça vermelha só deve aparecer quando ben for transformado forçacamente - em outros casos deve ser a verde. Além disso, a transformação /destransformação deve funcionar da seguinte forma - primeiro as partículas são emitidas, aí o jogador fica parado e se transforma, depois continua seu movimento.

PROMPT 5: a segunda barra - amarela, é a barra de especial. Cada ação de "Q" tem um custo. Por exemplo, cada bola de fogo lançada deve gastar 10% do especial. Começar o "Ice breath" deve gastar 10% do especial e continuar gastando devagar enquanto o jogador ainda tem "Q" segurado. O jogador não deve ser capaz de usar especiais se não tiver a porcentagem necessária. A barra de especial deve carregar em velocidade média constantemente

PROMPT 6: ao iniciar o jogo, nada deve ser carregado. O jogador deve ser apresentado a imagem "title.jpg" que servirá de fundo. Após apertar "enter" o programa começará a carregar o mapa e modelos, exibindo o "Save icon" girando na tela enquanto carrega.

PROMPT 7: carregue a imagem "omnitrix.png". Ela deve ficar no primeiro quadro de animação quando ben estiver transformado. Entretanto, quando ele for humano, deve estar no último. As mudanças entre esses quadros devem passar por todos os do meio. Exiba esse botão acima da barra de saúde

PROMPT 8: na tela inicial, adicione um texto com fonte amarela com borda preta no canto inferior direito que pisca, dizendo "Press the ENTER key"

PROMPT 9: centralize o texto e deixe todo branco. Ao morrer, exiba novas estatísticas que serão rastreadas durante o jogo (algumas ainda não foram implementadas)
Time - tempo desde que começou (em formato MM:SS)
Enemies slain - número de inimigos mortos
Objects destroyed - número de quebráveis destruídos, ainda não foram implementados. Crie uma versão dessa tela que ao invés de dizer "You died!" diga, "You win!". A vitória ainda não foi implementada, mas já deve existir essa tela. Depois de morrer ou vencer, o jogo deve ir pra tela de carregamento e reiniciar tudo. a contagem de tempo deve parar no momento que o personagem morre/ganha. A contagem de inimigos mortos e objetos quebrados está completamente quebrada, e não reinicia ao morrer/iniciar novamente. Use a mesma fonte do "loading" e centralize o texto horizontalmente por favor. antes de exibir a tela de morte, deixe a animação de morte do ben tocar. Então, tranque o movimento da câmera até que o jogador aperte enter.

PROMPT 10: aplique a seção que parece uma luz circular no omnitrix de "holograms.png" quando o jogador estiver como "ben" em cima do omnitrix. quando o ben estiver com o swampfire selecionado, mostre a imagem do swampfire em cima dele. Quando for o big chill, mostre a imagem do big chill em cima dele

PROMPT 11: quando o jogador usar um ataque, um texto com o nome do ataque deve aparecer no canto inferior esquerdo. Devem haver dois slots, um para o mais recente (abaixo) e outro para o anterior (menor e acima). Quando o jogador usar outro ataque enquanto algum estiver no mais recente, esse ataque subirá para o anterior e o novo substituirá seu lugar. Ataques no anterior vão ficando transparentes até sumir. Ataques no mais recente começam a ficar transparentes, mas se continuarem muito tempo no spot de mais recente (nenhum outro ataque foi usado), vão se deslocar pela esquerda até sair da tela. Os nomes dos ataques são:
Ben: "Light punch" e "Jab" (jab é o ataque pesado, ainda não implementado)
Swampfire: "Light punch" e "Fireball"
Big chill: "Light punch" e "Ice breath". Use a mesma fonte (com borda preta) da tela de title. O nome deve continuar lá enquanto o ataque estiver sendo usado, no caso do ice breath.  no caso do fireball, o nome só deve aparecer quando a bola de fogo for de fato lançada. O tamanho do nome de cima não precisa ser tão pequeno. Remova o texto em cima da cabeça dos cavaleiros

PROMPT 12: pare de carregar holograms.png, e passe a carregar swampfire_hologram, big_chill_hologram e glow. Adicione sliders para TODOS os itens (incluindo os elementos da barra separadamente). Adicione um botão que manda todos os offsets e scales configurados atualmente para printar no terminal.

## Commit com IA: Quebráveis adicionados, coletáveis modificados e outros aspectos de combate

PROMPT 1: agora vamos implementar quebráveis. A partir de agora, qualquer objeto marcado como quebrável vai ter uma barra de vida e uma hurtbox, e poder ser destruída por ataques do jogador, podendo dropar coletáveis quando for destruída. Ao ser destruído, o objeto vai emitir fragmentos (polígonos quase 2, muito finos) proporcionais ao tamanho do objeto. Essas propriedades (vida do objeto, cor dos fragmentos, etc. são configuráveis por tipo de objeto. Implemente essas lógicas em novos arquivos, breakables.cpp e fragments.cpp


PROMPT 2: change some things about combat:
- when an enemy dies, instead of flashing it, make it slowly disappear (becoming transparent)
- make the number of collectibles dropped random between 1-3 (also make them less transparent and a little bigger
- add impact visual effects when attacks from the player or the enemy connect
- make so the attacks still deal damage when the enemy is freezed
- make so the enemies come closer before they attack

PROMPT 3: - os coletáveis não estão respeitando a colisão, e entrando pra dentro do mapa.
- os objetos quebráveis, o efeito de hit não deve deixar a caixa em si semi-transparente. o que eu quis dizer é que a cor branca aplicada sobre ele deve ser semi-transparente
- os fragmentos dos objetos quebráveis não devem se dispersar tanto
- quando um inimigo morrer, ele deve ficar transparente normalmente, não sem esse efeito granulado estranho

PROMPT 4: os fragmentos, os coletáveis e os inimigos devem desaparecer lentamente, ficando mais transparentes até sumirem. Atualmente, os inimigos desaparecem instantaneamente, os coletáveis piscam e os quebráveis diminuem de tamanho.

PROMPT 5: adicione partículas quando o quebrável quebrar. Adicione novos coletáveis, um verde que recarrega a barra de transformação e um amarelo que recarrega a barra de especial. 

PROMPT 6: os coletáveis passarão a aparecer assim: todos tem uma bola branca brilhosa pequena dentro e uma bola da cor respectiva, semi transparente em volta dela. (ambas são concêntricas). além disso, a parte visível da caixa está desalinhada com a caixa de colisão, caindo e ficando abaixo dela.

PROMPT 7: os quebráveis devem ter colisão com outros quebráveis. A orientação do quebrável deve ser correspondente a pra onde o ben estava olhando quando o invocou. A visualização do modelo da caixa está correta, mas o ursinho e o banco parecem estar flutuando. Não mude os valores de escala do modelo/caixa de colisão.

PROMPT 8: o modelo deve estar apontando PRO jogador, não pro outro lado. Além disso, o banco agora está enterrado embaixo do chão. Os outros modelos estão corretos. Por fim, os coletáveis não devem piscar a parte de fora, apenas a de dentro (branca e luminosa)

PROMPT 9: Antes de serem absorvidos, os orbes devem ir para o centro do modelo do personagem. Além disso, reduza levemente a transparência da parte de fora desses orbes, para 75%.

PROMPT 10: os quebráveis devem invocar apenas um coletável quando quebrado cada:
banco - amarelo ou verde (75/25)
caixa - amarelo ou verde (25/75)
ursinho - vermelho ou amarelo (25/75)

PROMPT 11: impeça a colisão da bola de fogo com o chão (no tamanho máximo ela estava sendo destruída)

### Commit com IA: Refatoração do mapeamento dos colliders do modelo obj para um colliders.txt

PROMPT: crie um parser que percorra cada linha do meu @[data/map_ground/colliders.txt] e me de os numeros de cada linha convertidos para floats. Eles correspondem aos colisores do mapa, com seus min e max. Irei usar esses valores para criar as bbox do mapa


### Commit com IA: Modificação da câmera e coloquei um mar brabo

PROMPT: Eu quero modificar a camera, ela esta em 3a pessoa mas quero modificar ela pra ficar do jeito parecidodo meu jogo, consegue criar uma logica que me indique os parametros da camera em tempo real, q nem a posição do player no canto inferior esquerdo, para eu poder calibrar e mexer no codigo dps?
