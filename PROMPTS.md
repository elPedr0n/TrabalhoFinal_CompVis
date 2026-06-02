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