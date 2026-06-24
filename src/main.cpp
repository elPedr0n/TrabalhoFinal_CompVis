//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     CÓDIGO BASE PARA O TRABALHO FINAL
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
// Quaternion helpers for slerp
#include <glm/gtc/quaternion.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>
#include <stb_image.h>

// Headers da biblioteca para carregar modelos glTF
#include <tiny_gltf.h>
#include <gltf_utils.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "globals.h"
#include "sceneobject.h"
#include "animation.h"
#include "screens.h"
#include "breakables.h"
#include "sound.h"

float g_transform_sound_timer = 0.0f;
bool g_play_transform_sound = false;
#include "fragments.h"

#include <future>
#include <chrono>
#include <functional>

// Projectiles and particles
#include "projectiles.h"
#include "particles.h"
#include "ferris_wheel.h"

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;
    std::map<std::string, std::vector<float>> face_texture_selector_by_shape;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        ParseFaceTextureSelectorsFromObj(filename);
        printf("OK.\n");
    }

    void ParseFaceTextureSelectorsFromObj(const char* filename)
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
                else if (material_name.find("paredes") != std::string::npos || material_name.find("concrete") != std::string::npos)
                    current_selector = 10.0f; // Sinaliza que deve usar Concreto
                else if (material_name.find("Barrier") != std::string::npos || material_name.find("barrier") != std::string::npos)
                    current_selector = 17.0f; // Orange color
                else if (material_name.find("Material.001") != std::string::npos || material_name.find("wood") != std::string::npos)
                    current_selector = 9.0f;  // Sinaliza que deve usar Madeira
                else if (material_name.find("phong") != std::string::npos || material_name.find("lambert") != std::string::npos)
                    current_selector = 11.0f;
                else if (material_name.find("sides") != std::string::npos)
                    current_selector = 12.0f;
                else if (material_name.find("top") != std::string::npos)
                    current_selector = 14.0f;
                continue;
            }

            if (line.rfind("f ", 0) == 0 && !current_shape.empty())
            {
                face_texture_selector_by_shape[current_shape].push_back(current_selector);
            }
        }
    }

    // Variáveis para guardar as dimensões do modelo
    AABB aabb;
    glm::vec3 aabb_dimensions; // x = Largura, y = Altura, z = Profundidade

    // Variáveis pra desenhar as AABBs
    glm::vec3 aabb_vertices[8];

    // Função que calcula tudo isso
    void ComputeBoundingBox() {
        // Verifica se os vértices foram carregados com sucesso pelo tinyobj
        if (attrib.vertices.empty()) return;

        float min_x = std::numeric_limits<float>::max();
        float min_y = std::numeric_limits<float>::max();
        float min_z = std::numeric_limits<float>::max();

        float max_x = std::numeric_limits<float>::lowest();
        float max_y = std::numeric_limits<float>::lowest();
        float max_z = std::numeric_limits<float>::lowest();

        // Acessamos o vetor gigante de vértices do tinyobjloader
        size_t num_vertices = attrib.vertices.size() / 3;

        for (size_t v = 0; v < num_vertices; v++) 
        {
            float vx = attrib.vertices[3 * v + 0];
            float vy = attrib.vertices[3 * v + 1];
            float vz = attrib.vertices[3 * v + 2];

            min_x = std::min(min_x, vx);
            min_y = std::min(min_y, vy);
            min_z = std::min(min_z, vz);

            max_x = std::max(max_x, vx);
            max_y = std::max(max_y, vy);
            max_z = std::max(max_z, vz);
        }

        // Salva os valores calculados nas variáveis que criamos no Passo 1
        this->aabb = AABB(glm::vec3(min_x, min_y, min_z), glm::vec3(max_x, max_y, max_z));
        
        // As dimensões finais são a diferença entre o máximo e o mínimo
        this->aabb_dimensions = this->aabb.max - this->aabb.min;

        // Imprime no console para você ler facilmente quando rodar o jogo
        printf("Dimensoes -> Largura(X): %.2f, Altura(Y): %.2f, Profund(Z): %.2f\n", 
            this->aabb_dimensions.x, this->aabb_dimensions.y, this->aabb_dimensions.z);
            
            this->aabb_vertices[0] = glm::vec3(this->aabb.min.x, this->aabb.min.y, this->aabb.min.z); // V0
            this->aabb_vertices[1] = glm::vec3(this->aabb.max.x, this->aabb.min.y, this->aabb.min.z); // V1
            this->aabb_vertices[2] = glm::vec3(this->aabb.max.x, this->aabb.max.y, this->aabb.min.z); // V2
            this->aabb_vertices[3] = glm::vec3(this->aabb.min.x, this->aabb.max.y, this->aabb.min.z); // V3
            this->aabb_vertices[4] = glm::vec3(this->aabb.min.x, this->aabb.min.y, this->aabb.max.z); // V4
            this->aabb_vertices[5] = glm::vec3(this->aabb.max.x, this->aabb.min.y, this->aabb.max.z); // V5
            this->aabb_vertices[6] = glm::vec3(this->aabb.max.x, this->aabb.max.y, this->aabb.max.z); // V6
            this->aabb_vertices[7] = glm::vec3(this->aabb.min.x, this->aabb.max.y, this->aabb.max.z); // V7
        }

    AABB ComputeBoundingBoxForShape(size_t shape_index) {                                                                                                                                  
        float min_x = std::numeric_limits<float>::max();                                                                                                                                   
        float min_y = std::numeric_limits<float>::max();                                                                                                                                   
        float min_z = std::numeric_limits<float>::max();                                                                                                                                   
                                                                                                                                                                                           
        float max_x = std::numeric_limits<float>::lowest();                                                                                                                                
        float max_y = std::numeric_limits<float>::lowest();                                                                                                                                
        float max_z = std::numeric_limits<float>::lowest();                                                                                                                                
                                                                                                                                                                                           
        // Iterate over the indices of this specific shape                                                                                                                                 
        for (size_t i = 0; i < shapes[shape_index].mesh.indices.size(); i++) {                                                                                                             
            tinyobj::index_t idx = shapes[shape_index].mesh.indices[i];                                                                                                                    
                                                                                                                                                                                           
            float vx = attrib.vertices[3 * idx.vertex_index + 0];                                                                                                                          
            float vy = attrib.vertices[3 * idx.vertex_index + 1];                                                                                                                          
            float vz = attrib.vertices[3 * idx.vertex_index + 2];                                                                                                                          
                                                                                                                                                                                           
            min_x = std::min(min_x, vx);                                                                                                                                                   
            min_y = std::min(min_y, vy);                                                                                                                                                   
            min_z = std::min(min_z, vz);                                                                                                                                                   
                                                                                                                                                                                           
            max_x = std::max(max_x, vx);                                                                                                                                                   
            max_y = std::max(max_y, vy);                                                                                                                                                   
            max_z = std::max(max_z, vz);                                                                                                                                                   
        }                                                                                                                                                                                  
                                                                                                                                                                                           
        return AABB(glm::vec3(min_x, min_y, min_z), glm::vec3(max_x, max_y, max_z));                                                                                                       
    }                     
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

std::vector<AABB> parseColliders(const std::string& filepath);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename, bool use_rgba = false); // Função que carrega imagens de textura
void LoadUITexture(const char* filename, GLuint unit);
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
void DrawBoundingBox(AABB& aabb, int restore_object_id); // Desenha AABB em wireframe
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f, glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
float TextRendering_GetStringWidth(GLFWwindow* window, const std::string &str, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

//Movimentação do player 
void UpdatePosition(bool can_move, bool can_rotate = false);
void ResolvePlayerMapCollisions();

// Faz a logica de criacao do ataque do swampfire

// função de update dos inimigos
void UpdateEnemies();

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 1.26f; // Ângulo no plano ZX em relação ao eixo Z
float g_MovementTheta = 1.26f;
bool g_UseFixedCameras = true;
bool g_IsMovementBuffered = false;
float g_CameraPhi = 0.22f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_aabb_min_uniform;
GLint g_aabb_max_uniform;
GLint g_bone_matrices_uniform;
GLint g_hud_health_ratio_uniform;
GLint g_hud_bar2_ratio_uniform;
GLint g_hud_bar3_ratio_uniform;
GLint g_hud_omnitrix_frame_uniform;
float g_omnitrix_anim_frame = 15.0f;
GLint g_current_time_uniform;

// HUD Debug Variables
struct UiConfig {
    float x, y, scale_x, scale_y;
    const char* name;
};
UiConfig g_ui_items[13] = {
    { -0.860f, -0.175f, 0.060f, 0.500f, "Health BG" },
    { -0.860f, -0.175f, 0.060f, 0.500f, "Health FG" },
    { -0.830f, -0.020f, 0.028f, 0.330f, "Grey Container" },
    { -0.830f, 0.135f, 0.023f, 0.161f, "Green Bar" },
    { -0.830f, -0.172f, 0.023f, 0.163f, "Yellow Bar" },
    { -0.825f, 0.360f, 0.035f, 0.065f, "Cap Others" },
    { -0.895f, 0.360f, 0.035f, 0.065f, "Cap Health" },
    { -0.865f, 0.595f, 0.090f, 0.145f, "Omnitrix" },
    { -0.845f, 0.735f, 0.090f, 0.150f, "Hologram Base" },
    { -0.855f, 0.785f, 0.100f, 0.180f, "Big Chill Image" },
    { -0.850f, 0.795f, 0.045f, 0.165f, "Swampfire Image" },
    { -0.935f, -0.900f, 2.370f, 2.080f, "Recent Attack Text" },
    { -0.935f, -0.800f, 2.370f, 1.670f, "Previous Attack Text" }
};
int g_ui_selected_elem = 0; // 0 to 12
int g_ui_selected_param = 0; // 0: X, 1: Y, 2: Scale X, 3: Scale Y
bool g_ui_debug_enabled = false;
// Axes debug VAO/VBO
GLuint g_AxesVAO = 0;
GLuint g_AxesVBO = 0;
GLuint g_AxesColorVBO = 0;
GLuint g_BBoxVAO = 0;
GLuint g_BBoxVBO = 0;
GLuint g_BBoxEBO = 0;

void InitAxes()
{
    if (g_AxesVAO != 0) return;

    // 3 axes lines: X (blue), Y (green), Z (red) - each line = 2 vertices
    GLfloat positions[] = {
        0.0f, 0.0f, 0.0f, 1.0f,  0.5f, 0.0f, 0.0f, 1.0f, // X
        0.0f, 0.0f, 0.0f, 1.0f,  0.0f, 0.5f, 0.0f, 1.0f, // Y
        0.0f, 0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.5f, 1.0f  // Z
    };

    GLfloat colors[] = {
        0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f, // X vertices - blue
        0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f, // Y vertices - green
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f  // Z vertices - red
    };

    glGenVertexArrays(1, &g_AxesVAO);
    glBindVertexArray(g_AxesVAO);

    glGenBuffers(1, &g_AxesVBO);
    glBindBuffer(GL_ARRAY_BUFFER, g_AxesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    // position at location 0 (vec4)
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &g_AxesColorVBO);
    glBindBuffer(GL_ARRAY_BUFFER, g_AxesColorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    // color at location 6 (vec3)
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(6);

    glBindVertexArray(0);
}

void InitBoundingBox()
{
    if (g_BBoxVAO != 0) return;

    const GLuint bbox_indices[] = {
        0,1, 1,2, 2,3, 3,0, // base
        4,5, 5,6, 6,7, 7,4, // topo
        0,4, 1,5, 2,6, 3,7  // conexões
    };

    glGenVertexArrays(1, &g_BBoxVAO);
    glBindVertexArray(g_BBoxVAO);

    glGenBuffers(1, &g_BBoxVBO);
    glBindBuffer(GL_ARRAY_BUFFER, g_BBoxVBO);
    glBufferData(GL_ARRAY_BUFFER, 8 * 4 * sizeof(GLfloat), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &g_BBoxEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_BBoxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(bbox_indices), bbox_indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;
// Store OpenGL texture and sampler IDs for binding at draw time
std::vector<GLuint> g_LoadedTextureIDs;
std::vector<GLuint> g_LoadedSamplerIDs;

// Vetor global para movimentação
bool keys[1024] = {false};

Player player;

float gravidade = -6.0f;
float delta_t;

float bigchill_jump_speed = 5.0f;
float swampfire_jump_speed = 4.0f;
float bentennyson_jump_speed = 3.0f;


// Gambiarra mais absurda eh us guri 
glm::vec3 bigchill_size = glm::vec3(1.38963f * player.characters[0].scale, 1.96548f * player.characters[0].scale, 0.454046f * player.characters[0].scale);
glm::vec3 swampfire_size = glm::vec3(3.28f * player.characters[1].scale, 3.8f * player.characters[1].scale, 2.0f * player.characters[1].scale);
glm::vec3 bentennyson_size = glm::vec3(1.18f * player.characters[2].scale, 1.5f * player.characters[2].scale, 0.9f * player.characters[2].scale);

struct SpawnPoint g_spawn_points[MAX_SPAWN_POINTS] = {
    SpawnPoint(glm::vec3(0.162f, 1.000f, -19.182f), 0, 1),
    SpawnPoint(glm::vec3(4.820f, 1.000f, -20.590f), 0, 1),
    SpawnPoint(glm::vec3(5.149f, 1.000f, -25.142f), 0, 1),
    SpawnPoint(glm::vec3(2.463f, 1.000f, -25.913f), 0, 1),
    SpawnPoint(glm::vec3(2.675f, 1.000f, -31.658f), 0, 1),
    SpawnPoint(glm::vec3(8.278f, 1.000f, -36.553f), 0, 1),
    SpawnPoint(glm::vec3(4.253f, 1.000f, -40.824f), 0, 1),
    SpawnPoint(glm::vec3(5.961f, 1.000f, -47.035f), 0, 1),
    SpawnPoint(glm::vec3(10.160f, 1.000f, -46.688f), 0, 1),
    SpawnPoint(glm::vec3(16.273f, 1.000f, -51.056f), 0, 1),
    SpawnPoint(glm::vec3(20.158f, 1.000f, -51.102f), 0, 1),
    SpawnPoint(glm::vec3(20.177f, 1.000f, -55.054f), 0, 1),
    SpawnPoint(glm::vec3(17.362f, 1.000f, -59.580f), 0, 1),
    SpawnPoint(glm::vec3(11.262f, 1.000f, -60.843f), 0, 1),
    SpawnPoint(glm::vec3(18.921f, 1.000f, -62.191f), 0, 1),
    SpawnPoint(glm::vec3(6.840f, 0.988f, -84.157f), 1, 1),
    SpawnPoint(glm::vec3(4.212f, 0.988f, -90.785f), 1, 2),
    SpawnPoint(glm::vec3(4.775f, 0.988f, -96.124f), 1, 1),
    SpawnPoint(glm::vec3(7.543f, 0.988f, -98.300f), 1, 2),
    SpawnPoint(glm::vec3(10.514f, 0.988f, -97.883f), 1, 2),
    SpawnPoint(glm::vec3(10.520f, 0.988f, -84.577f), 1, 1)
};
int g_num_spawn_points = 21;

Enemy g_enemies[MAX_ENEMIES] = {
    Enemy(2.0f, 2.0f, -2.0f, 0.0f, 0.5f, true, 1.0f, 0.99f, 0.775f)
};


#include "projectiles.h"

MapItem map[MAX_PLATFORMS];
int g_num_platforms = 0;


void LoadModelTextureFixed(const char* filename, GLuint unit, GLuint program) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 4);
    if (!data) {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        return;
    }
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    // Bind uniform
    glUseProgram(program);
    char uniform_name[32];
    sprintf(uniform_name, "TextureImage%u", unit);
    glUniform1i(glGetUniformLocation(program, uniform_name), unit);
    glUseProgram(0);
}

struct FrustumPlane {
    glm::vec3 normal;
    float distance;
};

void ExtractFrustumPlanes(const glm::mat4& vp, FrustumPlane* planes) {
    planes[0].normal.x = vp[0][3] + vp[0][0]; planes[0].normal.y = vp[1][3] + vp[1][0]; planes[0].normal.z = vp[2][3] + vp[2][0]; planes[0].distance = vp[3][3] + vp[3][0];
    planes[1].normal.x = vp[0][3] - vp[0][0]; planes[1].normal.y = vp[1][3] - vp[1][0]; planes[1].normal.z = vp[2][3] - vp[2][0]; planes[1].distance = vp[3][3] - vp[3][0];
    planes[2].normal.x = vp[0][3] + vp[0][1]; planes[2].normal.y = vp[1][3] + vp[1][1]; planes[2].normal.z = vp[2][3] + vp[2][1]; planes[2].distance = vp[3][3] + vp[3][1];
    planes[3].normal.x = vp[0][3] - vp[0][1]; planes[3].normal.y = vp[1][3] - vp[1][1]; planes[3].normal.z = vp[2][3] - vp[2][1]; planes[3].distance = vp[3][3] - vp[3][1];
    planes[4].normal.x = vp[0][3] + vp[0][2]; planes[4].normal.y = vp[1][3] + vp[1][2]; planes[4].normal.z = vp[2][3] + vp[2][2]; planes[4].distance = vp[3][3] + vp[3][2];
    planes[5].normal.x = vp[0][3] - vp[0][2]; planes[5].normal.y = vp[1][3] - vp[1][2]; planes[5].normal.z = vp[2][3] - vp[2][2]; planes[5].distance = vp[3][3] - vp[3][2];

    for (int i = 0; i < 6; i++) {
        float length = glm::length(planes[i].normal);
        planes[i].normal /= length;
        planes[i].distance /= length;
    }
}

bool IsSphereInFrustum(const glm::vec3& center, float radius, FrustumPlane* planes) {
    for (int i = 0; i < 6; i++) {
        if (glm::dot(planes[i].normal, center) + planes[i].distance < -radius) {
            return false;
        }
    }
    return true;
}
#include "gamepad.h"
#include "sound.h"

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    InitGamepadMappings();
    InitSoundSystem();

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "INF01047 - Ben 10 Força Alienígena Ultra Remaster (OpenGL version)", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Papo do windows pra poder rodar mais de boa
    // Limit FPS to the monitor refresh rate (VSync)
    glfwSwapInterval(1);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Initialize debug axes VAO
    InitAxes();
    InitBoundingBox();

    // Carregamos imagens para as telas iniciais em unidades de textura fixas (15 e 16)
    LoadUITexture("../../data/title.jpg", 15); // TextureImage15
    LoadUITexture("../../data/Save Icon (Ben 10 Alien Force)/icon_ico.png", 16); // TextureImage16
    LoadUITexture("../../data/omnitrix.png", 13); // TextureImage13
    LoadUITexture("../../data/glow.png", 17); // TextureImage17
    LoadUITexture("../../data/big_chill_hologram.png", 18); // TextureImage18
    LoadUITexture("../../data/swampfire_hologram.png", 19); // TextureImage19

    LoadModelTextureFixed("../../data/map_background/teste_barraca/textures/kkgrmk_1.png", 20, g_GpuProgramID);
    LoadModelTextureFixed("../../data/map_background/teste_barraca/textures/kkgrpblhwi_2.png", 21, g_GpuProgramID);
    LoadModelTextureFixed("../../data/map_background/teste_barraca/textures/kkgrpicg_3.png", 22, g_GpuProgramID);
    LoadModelTextureFixed("../../data/map_background/teste_barraca/textures/kkgrplmn_4.png", 23, g_GpuProgramID);
    LoadModelTextureFixed("../../data/map_background/teste_barraca/textures/kkgryn_0.png", 24, g_GpuProgramID);
    LoadModelTextureFixed("../../data/map_background/japanese_noodle_stand/textures/Banner1_baseColor.jpeg", 25, g_GpuProgramID);
    LoadModelTextureFixed("../../data/map_background/japanese_noodle_stand/textures/lambert1_baseColor.jpeg", 26, g_GpuProgramID);
    LoadModelTextureFixed("../../data/map_background/japanese-chocolate-banana-stall/textures/cb_0.png", 27, g_GpuProgramID);
    
    // The save icon is now loaded as a 3D model

    ObjModel saveicon_model("../../data/Save Icon (Ben 10 Alien Force)/list_ico.obj");
    ComputeNormals(&saveicon_model);
    saveicon_model.ComputeBoundingBox();
    glm::vec3 saveicon_center = (saveicon_model.aabb.min + saveicon_model.aabb.max) / 2.0f;
    BuildTrianglesAndAddToVirtualScene(&saveicon_model);

    // Inicializamos o código para renderização de texto.
    glCheckError();
    TextRendering_Init();

    // ==========================================
    // TITLE SCREEN LOOP
    // ==========================================
    PlayMusic("../../data/sounds/title.mp3", false);
    while (!glfwWindowShouldClose(window)) {
        if (keys[GLFW_KEY_ENTER]) {
            break; // Proceed to loading
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_GpuProgramID);

        DrawTitleScreen(g_model_uniform, g_view_uniform, g_projection_uniform, g_object_id_uniform);

        // Blinking "Press the ENTER key"
        float time_sec = glfwGetTime();
        if (fmod(time_sec, 2.0f) < 1.0f) {
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            float text_scale = (float)h / 600.0f * 1.5f; // Scale proportional to screen height
            
            const char* prompt_text = IsGamepadConnected() ? "Press the START button" : "Press the ENTER key";

            // Para mudar a posição, altere o text_y (vertical) ou adicione um offset no text_x (horizontal) abaixo:
            float str_w = TextRendering_GetStringWidth(window, prompt_text, text_scale);
            
            float x_offset = 0.556f; // Deslocamento solicitado
            float text_x = (-str_w / 2.0f) + x_offset; // Centralizado + deslocamento
            float text_y = -0.731f;                    // Posição vertical solicitada
            
            // Draw black border
            TextRendering_PrintString(window, prompt_text, text_x - 0.005f, text_y, text_scale, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            TextRendering_PrintString(window, prompt_text, text_x + 0.005f, text_y, text_scale, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            TextRendering_PrintString(window, prompt_text, text_x, text_y - 0.005f, text_scale, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            TextRendering_PrintString(window, prompt_text, text_x, text_y + 0.005f, text_scale, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            // Draw yellow text
            TextRendering_PrintString(window, prompt_text, text_x, text_y, text_scale, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        ProcessGamepadInput(window);
    }
    StopMusic();

    if (glfwWindowShouldClose(window)) {
        glfwTerminate();
        return 0;
    }

    // ==========================================
    // COOPERATIVE LOADING SEQUENCE
    // ==========================================
    auto RenderLoadingStep = [&]() {
        if (glfwWindowShouldClose(window)) {
            glfwTerminate();
            std::exit(0);
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_GpuProgramID);
        
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (float)width / (height > 0 ? (float)height : 1.0f);
        
        // Setup orthographic projection for the 3D icon
        glm::mat4 proj = Matrix_Orthographic(-1.0f * aspect, 1.0f * aspect, -1.0f, 1.0f, -10.0f, 10.0f);
        glm::mat4 view = Matrix_Identity();
        glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(proj));

        // Position bottom left, scale appropriately, and rotate around Z axis
        float icon_offset_x = 0.2f;
        glm::mat4 model = Matrix_Translate(-aspect + icon_offset_x, -0.85f, 0.0f);
        model = model * Matrix_Scale(0.05f, 0.05f, 0.05f); // Adjust scale to make it very small
        model = model * Matrix_Rotate_X(M_PI / 12.0f); // Slightly tilt it to see it in 3D
        model = model * Matrix_Rotate_Z(glfwGetTime() * 3.0f); // Smooth time-based rotation
        model = model * Matrix_Translate(-saveicon_center.x, -saveicon_center.y, -saveicon_center.z); // Center the object before rotating
        
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, 31); // 31 = LOADING_SPINNER mapped to TextureImage16

        // Iterate over shapes to draw the save icon
        for (size_t i = 0; i < saveicon_model.shapes.size(); ++i) {
            std::string shape_name = saveicon_model.shapes[i].name;
            DrawVirtualObject(shape_name.c_str());
        }

        float text_scale = (float)height / 600.0f * 1.5f;
        // Posiciona o texto relativo à posição do ícone, mantendo a distância proporcional à altura
        float text_offset_x = icon_offset_x + 0.15f; 
        float text_x = -1.0f + (text_offset_x / aspect);
        TextRendering_PrintString(window, "Loading...", text_x, -0.87f, text_scale, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        glfwSwapBuffers(window);
        glfwPollEvents();
        ProcessGamepadInput(window);
    };

    // Helper para carregar fluidamente
    auto LoadGltfMemory = [](const std::string& path) {
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string err, warn;
        bool is_glb = (path.length() >= 4 && path.substr(path.length() - 4) == ".glb");
        if (is_glb) loader.LoadBinaryFromFile(&model, &err, &warn, path);
        else loader.LoadASCIIFromFile(&model, &err, &warn, path);
        computeNormalsForGLTF<uint16_t>(model);
        return model;
    };

    auto AsyncLoadGLTF = [&](const std::string& path, const std::string& base_name) {
        auto fut = std::async(std::launch::async, LoadGltfMemory, path);
        while (fut.wait_for(std::chrono::milliseconds(16)) != std::future_status::ready) {
            RenderLoadingStep();
        }
        tinygltf::Model model = fut.get();
        buildTrianglesAndAddToVirtualSceneFromGLTF(model, base_name);
        return model;
    };

    RenderLoadingStep();
    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/red_brick_diff_1k.jpg");      // TextureImage0
    LoadTextureImage("../../data/aguinha.jpeg"); // TextureImage1
    LoadTextureImage("../../data/bcck1.png"); // TextureImage2
    LoadTextureImage("../../data/bcck2.png"); // TextureImage3
    LoadTextureImage("../../data/TNT/TNT.png"); // TextureImage4
    LoadTextureImage("../../data/map_ground/A5_WoodTextureSeamless.png"); // TextureImage9
    LoadTextureImage("../../data/map_ground/concreto.jpeg"); // TextureImage10
    LoadTextureImage("../../data/health_bar.png", true); // TextureImage8
    LoadTextureImage("../../data/map_ground/speedboat_n1/textures/phong12_baseColor.png"); // TextureImage11 (idx 8)
    LoadTextureImage("../../data/map_ground/madeira_lados.png"); // TextureImage12 (idx 9)
    LoadTextureImage("../../data/map_ground/madeira_cima.png"); // TextureImage14 (idx 10)


    RenderLoadingStep();
    auto AsyncLoadOBJ = [&](const char* path) {
        auto fut = std::async(std::launch::async, [path](){
            ObjModel m(path);
            ComputeNormals(&m);
            return m;
        });
        while (fut.wait_for(std::chrono::milliseconds(16)) != std::future_status::ready) {
            RenderLoadingStep();
        }
        return fut.get();

        
    };

    RenderLoadingStep();
    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel = AsyncLoadOBJ("../../data/sphere.obj");
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel bunnymodel = AsyncLoadOBJ("../../data/bunny.obj");
    BuildTrianglesAndAddToVirtualScene(&bunnymodel);

    ObjModel planemodel = AsyncLoadOBJ("../../data/plane.obj");
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel blockmodel = AsyncLoadOBJ("../../data/TNT/TNT.obj");
    BuildTrianglesAndAddToVirtualScene(&blockmodel);
    blockmodel.ComputeBoundingBox();

    // Load map model
    ObjModel ground_model = AsyncLoadOBJ("../../data/map_ground/chao_mapa.obj");
    BuildTrianglesAndAddToVirtualScene(&ground_model);

    ObjModel barraca_model = AsyncLoadOBJ("../../data/map_background/teste_barraca/teste_barraca.obj");
    for (auto& shape : barraca_model.shapes) shape.name = "ice_" + shape.name;
    BuildTrianglesAndAddToVirtualScene(&barraca_model);
    ObjModel macarrao_model = AsyncLoadOBJ("../../data/map_background/barraca_macarrao/barraca_macarrao.obj");
    for (auto& shape : macarrao_model.shapes) shape.name = "noodle_" + shape.name;
    BuildTrianglesAndAddToVirtualScene(&macarrao_model);
    ObjModel banana_model = AsyncLoadOBJ("../../data/map_background/barraca_banana/barraca_banana.obj");
    for (auto& shape : banana_model.shapes) shape.name = "banana_" + shape.name;
    BuildTrianglesAndAddToVirtualScene(&banana_model);

    RenderLoadingStep();

    std::vector<AABB> colliders = parseColliders("../../data/map_ground/colliders.txt");                                                                                                                                                      
                                                                                                                                                                                           
    // Loop through all shapes in the OBJ                                                                                                                                                  
    for (int i = 0; i < colliders.size(); i++) {                
        if (i < MAX_PLATFORMS) {                                                                                                                                  
            map[i].bbox = colliders[i]; 
            g_num_platforms++;
        } else {                                                                                                                                                                       
            printf("WARNING: Too many colliders! Increase MAX_PLATFORMS.\n");                                                                                                          
        }                                                                                                                                                                              
    }

    ground_model.ComputeBoundingBox();


    std::vector<glm::vec3> barracas_positions = {
        glm::vec3(-1.1f, 1.0f, -4.954f), 
        glm::vec3(-1.2f, 1.0f, -7.9f),
        glm::vec3(-4.7f, 1.0f, -10.4f),
        glm::vec3(-1.1f, 1.0f, -13.7f),
        glm::vec3(-1.2f, 1.0f, -16.5f),
        glm::vec3(-4.7f, 1.0f, -19.0f),
        glm::vec3(-0.8f, 1.0f, -22.5f),
        glm::vec3(-0.2f, 1.0f, -25.3f),
        glm::vec3(-3.4f, 1.0f, -28.7f),
        glm::vec3(1.0f, 1.0f, -30.8f),
        glm::vec3(1.3f, 1.0f, -33.8f),
        glm::vec3(-1.6f, 1.0f, -37.4f),
        glm::vec3(2.8f, 1.0f, -39.6f),
        glm::vec3(3.4f, 1.0f, -42.5f),
        glm::vec3(1.0f, 1.0f, -45.0f)
    };

    std::vector<glm::vec3> tower_positions = {
        glm::vec3(9.5f, 6.0f, -81.373f),
        glm::vec3(9.5f, 6.0f, -71.373f),
        glm::vec3(1.6f, 6.0f, -71.373f),
        glm::vec3(1.6f, 6.0f, -81.373f),
        glm::vec3(9.5f, 6.0f, -101.373f),
        glm::vec3(1.6f, 6.0f, -101.373f)
    };


    std::vector<glm::vec3> wall_positions = {
       glm::vec3(5.5f, 3.0f, -71.373f),
       glm::vec3(5.5f, 3.0f, -101.373f),
       glm::vec3(9.5f, 3.0f, -76.373f), 
       glm::vec3(1.5f, 3.0f, -76.373f) 
    };

    // Load swampfire glTF and build GPU resources; loader prints diagnostics
    tinygltf::Model gltfmodel = AsyncLoadGLTF("../../data/swampfire__ben_10_alien_force/scene.gltf", "the_swampfire");
    tinygltf::Model bentennyson_model = AsyncLoadGLTF("../../data/ben_tennyson.glb", "the_bentennyson");
    tinygltf::Model foreverknight_model = AsyncLoadGLTF("../../data/forever_knight.glb", "the_foreverknight");
    tinygltf::Model castle_model = AsyncLoadGLTF("../../data/castelin/scene.gltf", "the_castle");
    tinygltf::Model wall_gltf_model = AsyncLoadGLTF("../../data/map_background/wall.glb", "the_wall");
    tinygltf::Model tower_gltf_model = AsyncLoadGLTF("../../data/map_background/tower.glb", "the_tower");
    tinygltf::Model bigchill_model = AsyncLoadGLTF("../../data/big_chill_ben_10.glb", "the_bigchill");
    tinygltf::Model bigchill_cloaked_model = AsyncLoadGLTF("../../data/big_chill_cloaked.glb", "the_bigchill_cloaked");
    tinygltf::Model ferris_wheel_model = AsyncLoadGLTF("../../data/map_background/ferris_wheel/scene.gltf", "the_ferris_wheel");
    
    // Breakables
    AsyncLoadGLTF("../../data/breakables/low_poly_asset_teddy_bear.glb", "the_teddy_bear");
    AsyncLoadGLTF("../../data/breakables/wooden_box_low_poly.glb", "the_wooden_box");
    AsyncLoadGLTF("../../data/breakables/simple_park_bench.glb", "the_park_bench");
    
    // We no longer use a GLTF fireball; projectiles will use the static `the_sphere` mesh from OBJ imports.
    tinygltf::Model emptyModel; // placeholder when no GLTF is used for projectiles
    
    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    // glCheckError();
    // TextRendering_Init(); // This is now done before the title screen

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    bool game_is_running = true;
    while (game_is_running && !glfwWindowShouldClose(window)) {
        // RESET GAME STATE
        player = Player();
        player.start_time = (float)glfwGetTime();
        for (int i = 0; i < MAX_ENEMIES; i++) g_enemies[i].visible = false;
        for (int i = 0; i < MAX_COLLECTIBLES; i++) g_collectibles[i].active = false;
        
        // BUG 2: Resetar o número de inimigos spawnáveis quando o player morrer
        for (int i = 0; i < g_num_spawn_points; i++) {
            g_spawn_points[i].enemies_spawned = 0;
            g_spawn_points[i].active_enemy_id = -1;
        }
        
        for (int i = 0; i < MAX_BREAKABLES; i++) g_breakables[i].active = false;
        SpawnBreakable(glm::vec3(4.396f, 0.000f, -4.801f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 3.572f);
        SpawnBreakable(glm::vec3(4.396f, 1.200f, -4.801f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 3.572f);
        SpawnBreakable(glm::vec3(1.099f, 0.000f, -5.125f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 2.118f);
        SpawnBreakable(glm::vec3(0.401f, 0.000f, -8.648f), 0.080f, "the_park_bench", 30.0f, 9.000f, 10.000f, 20.000f, glm::vec3(0.32f, 0.26f, 0.22f), 0.20f, 15, 1.571f);
        SpawnBreakable(glm::vec3(4.270f, 0.000f, -15.677f), 0.080f, "the_park_bench", 30.0f, 9.000f, 10.000f, 20.000f, glm::vec3(0.32f, 0.26f, 0.22f), 0.20f, 15, 4.712f);
        SpawnBreakable(glm::vec3(4.596f, 0.000f, -21.789f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 4.650f);
        SpawnBreakable(glm::vec3(4.650f, 0.000f, -23.112f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 3.745f);
        SpawnBreakable(glm::vec3(1.280f, 0.000f, -27.149f), 0.080f, "the_park_bench", 30.0f, 9.000f, 10.000f, 20.000f, glm::vec3(0.32f, 0.26f, 0.22f), 0.20f, 15, 1.571f);
        SpawnBreakable(glm::vec3(2.780f, 0.000f, -33.034f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 0.459f);
        SpawnBreakable(glm::vec3(2.780f, 1.200f, -33.034f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 0.459f);
        SpawnBreakable(glm::vec3(7.583f, 0.000f, -34.916f), 0.080f, "the_park_bench", 30.0f, 9.000f, 10.000f, 20.000f, glm::vec3(0.32f, 0.26f, 0.22f), 0.20f, 15, 4.712f);
        SpawnBreakable(glm::vec3(5.495f, 1.200f, -39.560f), 1.500f, "the_teddy_bear", 10.0f, 0.400f, 0.400f, 0.400f, glm::vec3(0.83f, 0.69f, 0.61f), 0.20f, 8, 1.521f);
        SpawnBreakable(glm::vec3(5.451f, 0.000f, -39.656f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 0.369f);
        SpawnBreakable(glm::vec3(0.327f, 0.000f, -18.605f), 1.500f, "the_teddy_bear", 10.0f, 0.400f, 0.400f, 0.400f, glm::vec3(0.83f, 0.69f, 0.61f), 0.20f, 8, 1.611f);
        SpawnBreakable(glm::vec3(7.879f, 0.000f, -51.031f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 0.317f);
        SpawnBreakable(glm::vec3(7.879f, 1.200f, -51.031f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 0.317f);
        SpawnBreakable(glm::vec3(7.574f, 0.000f, -49.338f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 0.441f);
        SpawnBreakable(glm::vec3(7.931f, 1.200f, -49.386f), 1.500f, "the_teddy_bear", 10.0f, 0.400f, 0.400f, 0.400f, glm::vec3(0.83f, 0.69f, 0.61f), 0.20f, 8, 1.381f);
        SpawnBreakable(glm::vec3(9.726f, 0.000f, -56.927f), 0.080f, "the_park_bench", 30.0f, 9.000f, 10.000f, 20.000f, glm::vec3(0.32f, 0.26f, 0.22f), 0.20f, 15, 1.571f);
        SpawnBreakable(glm::vec3(9.362f, 0.000f, -59.537f), 0.080f, "the_park_bench", 30.0f, 9.000f, 10.000f, 20.000f, glm::vec3(0.32f, 0.26f, 0.22f), 0.20f, 15, 1.571f);
        SpawnBreakable(glm::vec3(13.072f, 0.000f, -67.149f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 6.228f);
        SpawnBreakable(glm::vec3(10.946f, 0.000f, -66.791f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 0.686f);
        SpawnBreakable(glm::vec3(17.227f, 0.000f, -50.562f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 2.141f);
        SpawnBreakable(glm::vec3(9.498f, 0.000f, -64.202f), 1.500f, "the_teddy_bear", 10.0f, 0.400f, 0.400f, 0.400f, glm::vec3(0.83f, 0.69f, 0.61f), 0.20f, 8, 1.249f);
        SpawnBreakable(glm::vec3(21.579f, 0.000f, -57.297f), 0.080f, "the_park_bench", 30.0f, 9.000f, 10.000f, 20.000f, glm::vec3(0.32f, 0.26f, 0.22f), 0.20f, 15, 4.712f);
        SpawnBreakable(glm::vec3(16.613f, 0.000f, -57.810f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 0.654f);
        SpawnBreakable(glm::vec3(16.368f, 1.200f, -57.509f), 1.500f, "the_teddy_bear", 10.0f, 0.400f, 0.400f, 0.400f, glm::vec3(0.83f, 0.69f, 0.61f), 0.20f, 8, 1.069f);
        SpawnBreakable(glm::vec3(12.429f, 0.000f, -47.390f), 0.080f, "the_park_bench", 30.0f, 9.000f, 10.000f, 20.000f, glm::vec3(0.32f, 0.26f, 0.22f), 0.20f, 15, 3.142f);
        SpawnBreakable(glm::vec3(22.484f, -0.371f, -81.857f), 1.500f, "the_teddy_bear", 10.0f, 0.400f, 0.400f, 0.400f, glm::vec3(0.83f, 0.69f, 0.61f), 0.20f, 8, 0.116f);
        SpawnBreakable(glm::vec3(5.375f, -0.012f, -84.465f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 2.049f);
        SpawnBreakable(glm::vec3(5.375f, 1.188f, -84.465f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 2.049f);
        SpawnBreakable(glm::vec3(8.606f, -0.012f, -84.490f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 3.221f);
        SpawnBreakable(glm::vec3(5.169f, -0.012f, -97.720f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 0.641f);
        SpawnBreakable(glm::vec3(9.207f, -0.012f, -97.917f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 5.994f);
        SpawnBreakable(glm::vec3(3.910f, -0.012f, -92.668f), 0.400f, "the_wooden_box", 25.0f, 3.000f, 3.000f, 3.000f, glm::vec3(0.57f, 0.52f, 0.47f), 0.40f, 12, 1.621f);
        SpawnBreakable(glm::vec3(3.632f, 1.188f, -92.300f), 1.500f, "the_teddy_bear", 10.0f, 0.400f, 0.400f, 0.400f, glm::vec3(0.83f, 0.69f, 0.61f), 0.20f, 8, 1.621f);
        SpawnBreakable(glm::vec3(4.043f, -0.012f, -87.881f), 0.080f, "the_park_bench", 30.0f, 9.000f, 10.000f, 20.000f, glm::vec3(0.32f, 0.26f, 0.22f), 0.20f, 15, 1.571f);

        // Fake loading screen for 2 seconds
        float fake_load_start = (float)glfwGetTime();
        while ((float)glfwGetTime() - fake_load_start < 2.0f && !glfwWindowShouldClose(window)) {
            RenderLoadingStep();
        }

        // --- WARM-UP PASS ---
        // Desenha todos os modelos uma vez (sem exibir na tela) para forçar o driver OpenGL 
        // a carregar as texturas e VAOs na VRAM, evitando quedas de FPS nas transformações.
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Desabilita escrita de cor
        glDisable(GL_DEPTH_TEST);
        glUseProgram(g_GpuProgramID);
        for (const auto& pair : g_VirtualScene) {
            // Re-bind as texturas caso a malha tenha textura carregada
            if (pair.second.texture_id != 0) {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, pair.second.texture_id);
                glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
            }
            DrawVirtualObject(pair.first.c_str());
        }
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpa novamente
        // --- END WARM-UP ---

        float anterior = (float)glfwGetTime();

    // Variáveis de estado da máquina de animação
    bool is_attacking = false;

    // Swampfire animation local state (preserves timers and flags)
    SwampfireAnimState swampfire_state;
    BenAnimState ben_state;
    BigChillAnimState bigchill_cloaked_state;
    BigChillAnimState bigchill_ben10_state;

    int current_enemy_anim = 36;
    bool t_key_was_down = false;

    GltfAnimator swampfireAnimator(gltfmodel);
    // Animator placeholder for projectiles (no GLTF for projectiles)
    GltfAnimator fireballAnimator(emptyModel);
    GltfAnimator bentennysonAnimator(bentennyson_model);
    GltfAnimator foreverknightAnimator(foreverknight_model);
    GltfAnimator bigchillBen10Animator(bigchill_model);
    GltfAnimator bigchillCloakedAnimator(bigchill_cloaked_model);
    bool bigchill_was_jumping = false;
    // keep swampfire_state alive for the main loop (defined above)

    bool inner_loop_running = true;
    PlayMusic("../../data/sounds/song1.mp3", true);
    while (inner_loop_running && !glfwWindowShouldClose(window))
    {
        UpdateSoundSystem();

        if (g_play_transform_sound) {
            g_transform_sound_timer -= delta_t;
            if (g_transform_sound_timer <= 0.0f) {
                PlaySoundEffect("../../data/sounds/omintrix_transform.wav"); // Intentionally matched spelling from filesystem
                g_play_transform_sound = false;
            }
        }

        static bool was_dead = false;
        static bool was_won = false;
        
        if (player.is_dead && !was_dead) {
            was_dead = true;
            StopMusic();
            PlayMusic("../../data/sounds/defeat.mp3", false);
        } else if (player.has_won && !was_won) {
            was_won = true;
            StopMusic();
            PlayMusic("../../data/sounds/victory.mp3", false);
        }
        if (keys[GLFW_KEY_ESCAPE])
            glfwSetWindowShouldClose(window, GL_TRUE);

        // Update delta time
        double agora = glfwGetTime();
        delta_t = (float)(agora - anterior);

        // Atualizar energia de transformacao e especial
        if (!player.is_dead) {
            if (player.active_character == 2) {
                g_omnitrix_anim_frame += delta_t * 60.0f;
                if (g_omnitrix_anim_frame > 15.0f) g_omnitrix_anim_frame = 15.0f;
            } else {
                g_omnitrix_anim_frame -= delta_t * 60.0f;
                if (g_omnitrix_anim_frame < 0.0f) g_omnitrix_anim_frame = 0.0f;
            }

            player.special_energy += (100.0f / 15.0f) * delta_t; // Recarrega em 15s
            if (player.special_energy > player.max_special_energy) {
                player.special_energy = player.max_special_energy;
            }

            if (player.active_character == 2) {
                float old_energy = player.transform_energy;
                player.transform_energy += 10.0f * delta_t; // Recarrega em 10s
                if (old_energy < player.max_transform_energy && player.transform_energy >= player.max_transform_energy) {
                    PlayOmnitrixSound("../../data/sounds/omnitrix_ready.wav");
                }
                if (player.transform_energy > player.max_transform_energy) {
                    player.transform_energy = player.max_transform_energy;
                }
            } else {
                static bool is_waiting_oops = false;
                if (!is_waiting_oops) {
                    player.transform_energy -= (100.0f / 40.0f) * delta_t; // Dura 40s
                }
                
                if (player.transform_energy <= 0.0f) {
                    player.transform_energy = 0.0f;
                    
                    if (!is_waiting_oops) {
                        is_waiting_oops = true;
                        PlayOopsSound();
                    }
                    
                    if (is_waiting_oops && IsOopsSoundFinished()) {
                        is_waiting_oops = false;
                        
                        // Force revert to Ben
                        PlayDetransformSound("../../data/sounds/omnitrix_detransform.wav");
                        player.active_character = 2; // Ben
                        glm::vec3 size = bentennyson_size;
                        printf("Energy depleted! Reverting to Ben.\n");
                        player.characters[player.active_character].bbox = makeAABBFromGround(player.position, size);
                        ResolvePlayerMapCollisions();

                        ParticleOptions popts;
                        popts.color = HexToRgb("#ff0000"); // Red flash on forced revert
                        popts.life = 0.25f + 0.15f * 1.0f;
                        popts.scale = 0.15f + 0.01f * 6.0f;
                        popts.speed = 0.1f + 0.8f * 3.0f;
                        popts.count = std::max(2, (int)std::round(8.0f * 6.0f));
                        Particles_Spawn(glm::vec3(player.position.x, player.position.y, player.position.z), popts);
                    }
                }
            }
        }
        anterior = (float)agora;

        // Update UI timers
        auto updateAttackUI = [&](AttackUI& atk) {
            if (!atk.active) return;
            if (&atk == &player.recent_attack && player.active_character == 0 && bigchill_cloaked_state.is_q_attacking && atk.text == "Ice breath") {
                atk.timer = 0.0f;
            } else {
                atk.timer += delta_t;
            }
            if (atk.timer > 3.0f) { // wait 3 seconds before moving left
                if (&atk == &player.recent_attack) {
                    atk.x_offset -= 1.0f * delta_t; // move left
                }
                if (atk.x_offset < -2.0f || atk.timer > 4.0f) {
                    atk.active = false;
                }
            }
        };
        updateAttackUI(player.previous_attack);
        updateAttackUI(player.recent_attack);

        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(0.02f, 0.02f, 0.08f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);
        glUniform1f(g_current_time_uniform, (float)agora);

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        glm::vec4 camera_position_c;
        glm::vec4 camera_lookat_l;
        glm::vec4 camera_view_vector;
        glm::vec4 camera_up_vector;

        static std::vector<glm::vec4> fixed_camera_positions = {
            glm::vec4(6.74, 2.97, -11.33, 1.0f),
            glm::vec4(7.92, 2.97, -21.48, 1.0f),
            glm::vec4(0.11, 2.55, -19.39, 1.0f),
            glm::vec4(1.41, 3.07, -28.31, 1.0f), // new 4th camera
            glm::vec4(3.18, 3.24, -36.55, 1.0f),
            glm::vec4(23.86, 3.10, -55.50, 1.0f),
            glm::vec4(27.43, 4.38, -65.10, 1.0f),
            glm::vec4(24.51, 5.90, -91.46, 1.0f),
            glm::vec4(11.74, 3.93, -99.51, 1.0f)
        };
        static std::vector<glm::vec4> fixed_camera_lookats = {
            glm::vec4(3.47, 1.50, -10.54, 1.0f),
            glm::vec4(4.66, 1.50, -20.68, 1.0f),
            glm::vec4(2.25, 1.50, -23.42, 1.0f),
            glm::vec4(3.10, 1.50, -31.19, 1.0f), // new 4th camera lookat
            glm::vec4(5.94, 1.50, -39.46, 1.0f),
            glm::vec4(18.73, 1.50, -56.18, 1.0f),
            glm::vec4(21.17, 1.95, -68.75, 1.0f),
            glm::vec4(19.74, 1.91, -87.00, 1.0f),
            glm::vec4(7.91, 1.49, -95.32, 1.0f)
        };
        static int current_camera_idx = 0;

        if (g_UseFixedCameras) {
            camera_position_c = fixed_camera_positions[current_camera_idx];
            camera_lookat_l   = fixed_camera_lookats[current_camera_idx];
            camera_up_vector  = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
            camera_view_vector = camera_lookat_l - camera_position_c;

            // Atualiza a orientação do controle para a câmera atual se não estiver em buffer
            if (!g_IsMovementBuffered) {
                g_CameraTheta = atan2(-camera_view_vector.x, -camera_view_vector.z);
            }
        } else {
            float r = g_CameraDistance;
            float y = r*sin(g_CameraPhi);
            float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
            float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

            float height_offset = 1.5f;

            camera_lookat_l    = glm::vec4(player.position.x, player.position.y + height_offset, player.position.z, 1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
            camera_position_c  = camera_lookat_l + glm::vec4(x, y + 0.5, z, 0.0f); // Ponto "c", centro da câmera
            camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
            camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
        }

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -50.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        if (g_UseFixedCameras) {
            glm::vec4 player_pos = glm::vec4(player.position.x, player.position.y + 1.0f, player.position.z, 1.0f);
            glm::vec4 ndc_pos = projection * view * player_pos;
            
            bool out_of_frustum = false;
            if (ndc_pos.w > 0.0f) {
                glm::vec4 n = ndc_pos / ndc_pos.w;
                // Um pouco de folga (1.2f) antes de considerar fora da tela
                if (n.x < -1.2f || n.x > 1.2f || n.y < -1.2f || n.y > 1.2f) {
                    out_of_frustum = true;
                }
            } else {
                out_of_frustum = true;
            }

            // O foco da câmera (LookAt) representa a verdadeira "zona" que a câmera cobre ao longo do percurso.
            // Avaliar a posição da câmera gera glitches porque as câmeras podem estar em ângulos estranhos (ex: câmera 3).
            float dist_current = glm::distance(glm::vec3(player_pos), glm::vec3(fixed_camera_lookats[current_camera_idx]));
            float dist_next = 999999.0f;
            float dist_prev = 999999.0f;

            if (current_camera_idx < (int)fixed_camera_positions.size() - 1) {
                dist_next = glm::distance(glm::vec3(player_pos), glm::vec3(fixed_camera_lookats[current_camera_idx + 1]));
            }
            if (current_camera_idx > 0) {
                dist_prev = glm::distance(glm::vec3(player_pos), glm::vec3(fixed_camera_lookats[current_camera_idx - 1]));
            }

            if (out_of_frustum) {
                // Se saiu da tela, muda para a zona adjacente se ela for fisicamente mais próxima do player que a atual.
                // Usar menor que a atual IMPEDE qualquer glitch de vai-e-vem.
                if (dist_next < dist_current && dist_next < dist_prev) {
                    current_camera_idx++;
                } else if (dist_prev < dist_current && dist_prev < dist_next) {
                    current_camera_idx--;
                }
            } else {
                // Se ainda está visível na tela, troca de câmera suavemente quando o jogador 
                // entrar definitivamente no raio de ação (LookAt) da próxima câmera
                float margin = 3.0f; 
                if (dist_next < dist_current - margin && dist_next < dist_prev) {
                    current_camera_idx++;
                } else if (dist_prev < dist_current - margin && dist_prev < dist_next) {
                    current_camera_idx--;
                }
            }
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));
        
        FrustumPlane frustum_planes[6];
        glm::mat4 view_projection = projection * view;
        ExtractFrustumPlanes(view_projection, frustum_planes);

        // Pass player data for lighting
        glUniform3f(glGetUniformLocation(g_GpuProgramID, "player_position"), player.position.x, player.position.y + 0.5f, player.position.z);
        glUniform1i(glGetUniformLocation(g_GpuProgramID, "active_alien"), player.active_character);

        // =============================================
        // POSTES DE LUZ - Edite as posições aqui!
        // Formato: {x, y, z} — y é a altura do poste
        // Para adicionar um novo poste: adicione uma linha nova no array
        // Para remover: delete a linha
        // Máximo: 8 postes (MAX_LAMPS no shader)
        // =============================================
        static const glm::vec3 lamp_positions[] = {
            {  4.820f, 4.0f, -13.884f },  // Poste 1
            {  4.736f, 4.0f, -20.137f },  // Poste 2
            {  5.691f, 4.0f, -25.550f },  // Poste 3
            {  6.629f, 4.0f, -31.499f },  // Poste 4
            {  8.429f, 4.0f, -38.928f },  // Poste 5
            {  5.804f, 4.0f, -46.470f },  // Poste 6
            {  9.581f, 4.0f, -53.897f },  // Poste 7 
            {  19.522f, 4.0f, -51.211f },   // Poste 8 
            {  21.224f, 4.0f, -59.004f },
            {  11.633f, 4.0f, -65.781f }
        };
        static const int num_lamps = sizeof(lamp_positions) / sizeof(lamp_positions[0]);
        glUniform1i(glGetUniformLocation(g_GpuProgramID, "num_lamps"), num_lamps);
        glUniform3fv(glGetUniformLocation(g_GpuProgramID, "lamp_positions"), num_lamps, glm::value_ptr(lamp_positions[0]));

        #define SPHERE 0
        #define BUNNY  1
        #define PLANE  2
        #define CHILL  3
        #define SWAMPFIRE 4
        #define BLOCO 5
        #define FIREBALL 6
        #define BENTENNYSON 8
        #define FOREVERKNIGHT 9
        #define CASTLE 11
        #define UAF_CHILL 15
        #define AXES_DEBUG 100
        #define BBOX_DEBUG 101
        #define COLLECT_OBJ 10
        #define GROUND 13
        #define SKYBOX 99

        // Re-bind all previously loaded textures/samplers to their texture units
        for (GLuint tu = 0; tu < g_NumLoadedTextures; ++tu)
        {
            glActiveTexture(GL_TEXTURE0 + tu);
            glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[tu]);
            glBindSampler(tu, g_LoadedSamplerIDs[tu]);
        }

        // ==========================================
        // DRAW SKYBOX (Background gradient + clouds)
        // ==========================================
        glDepthMask(GL_FALSE); // Don't write to depth buffer
        glDisable(GL_CULL_FACE); // See inside the sphere

        model = Matrix_Translate(camera_position_c.x, camera_position_c.y, camera_position_c.z) * Matrix_Scale(40.0f, 40.0f, 40.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, SKYBOX);

        // Uses a very simple white texture so it doesn't affect color calculation in shader too much, or shader ignores Kd0 for SKYBOX
        DrawVirtualObject("the_sphere");

        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);

        // Compute animations for all characters at the top
        BigChillAnimResult bigchillBen10Res = computeBigChillBen10Animation(bigchill_model, keys, player.jumping, delta_t, agora, bigchill_ben10_state);
        BigChillAnimResult bigchillCloakedRes = computeBigChillCloakedAnimation(bigchill_cloaked_model, keys, player.jumping, delta_t, agora, bigchill_cloaked_state);
        SwampfireAnimResult animRes = computeSwampfireAnimation(gltfmodel, keys, player.jumping, delta_t, agora, swampfire_state);
        BenAnimResult benRes = computeBenAnimation(bentennyson_model, keys, player.jumping, delta_t, agora, ben_state);

        is_attacking = false;
        if (player.active_character == 0) is_attacking = bigchill_cloaked_state.is_attacking || bigchill_cloaked_state.is_q_attacking;
        else if (player.active_character == 1) is_attacking = swampfire_state.is_e_attacking || swampfire_state.q_state > 0;
        else if (player.active_character == 2) is_attacking = ben_state.is_attacking || ben_state.is_q_attacking;

        // Attack Detection logic
        static bool prev_ben_attacking = false;
        if (player.active_character == 2 && ben_state.is_attacking && !prev_ben_attacking) {
            player.pushAttack("Light punch"); 
        }
        prev_ben_attacking = ben_state.is_attacking;

        static bool prev_ben_punch_hitbox = false;
        if (benRes.punch_active && !prev_ben_punch_hitbox) PlaySoundEffect("../../data/sounds/ben_punch.wav");
        bool ben_punch_just_triggered = benRes.punch_active && !prev_ben_punch_hitbox;
        prev_ben_punch_hitbox = benRes.punch_active;

        static bool prev_sf_punch1 = false;
        static bool prev_sf_punch2 = false;
        if (animRes.punch1_active && !prev_sf_punch1) PlaySoundEffect("../../data/sounds/swampfire_punch.wav");
        if (animRes.punch2_active && !prev_sf_punch2) PlaySoundEffect("../../data/sounds/swampfire_punch.wav");
        bool sf_punch_just_triggered = (animRes.punch1_active && !prev_sf_punch1) || (animRes.punch2_active && !prev_sf_punch2);
        prev_sf_punch1 = animRes.punch1_active;
        prev_sf_punch2 = animRes.punch2_active;

        static bool prev_bc_punch1 = false;
        bool bc_punch_just_triggered = false;
        if (player.active_character == 0) {
            if (!player.jumping) {
                if (bigchill_cloaked_state.is_attacking && bigchillCloakedRes.punch_sound_trigger) {
                    bc_punch_just_triggered = true;
                    PlaySoundEffect("../../data/sounds/punch_not_connect.wav");
                }
            } else {
                if (bigchill_ben10_state.is_attacking && bigchillBen10Res.punch_sound_trigger) {
                    bc_punch_just_triggered = true;
                    PlaySoundEffect("../../data/sounds/punch_not_connect.wav");
                }
            }
        }
        prev_bc_punch1 = bigchillCloakedRes.punch_active;

        static bool prev_ben_q = false;
        static bool prev_ben_slap_hitbox = false;
        if (player.active_character == 2 && ben_state.is_q_attacking && !prev_ben_q) {
            player.pushAttack("Big slap");
        }
        if (benRes.big_slap_active && !prev_ben_slap_hitbox) PlaySoundEffect("../../data/sounds/ben_heavy.wav");
        bool ben_slap_just_triggered = benRes.big_slap_active && !prev_ben_slap_hitbox;
        prev_ben_slap_hitbox = benRes.big_slap_active;
        prev_ben_q = ben_state.is_q_attacking;

        static bool prev_swamp_e = false;
        if (player.active_character == 1 && swampfire_state.is_e_attacking && !prev_swamp_e) {
            player.pushAttack("Beat up");
        }
        prev_swamp_e = swampfire_state.is_e_attacking;

        if (player.active_character == 1 && animRes.spawn_fireball_strength > 0.0f) {
            player.pushAttack("Fireball");
        }

        static bool prev_bc_e = false;
        if (player.active_character == 0 && bigchill_cloaked_state.is_attacking && !prev_bc_e) {
            player.pushAttack("Cold punch");
        }
        prev_bc_e = bigchill_cloaked_state.is_attacking;

        static bool prev_bc_q = false;
        if (player.active_character == 0 && bigchill_cloaked_state.is_q_attacking) {
            if (bigchill_cloaked_state.q_attack_timer > 1.0f) {
                StartIceBreath();
            } else {
                StopIceBreath();
            }
            if (!prev_bc_q) player.pushAttack("Ice breath");
        } else {
            StopIceBreath();
        }
        prev_bc_q = bigchill_cloaked_state.is_q_attacking;

        // O personagem só pode se mover se não estiver no meio de um ataque, morto ou sofrendo flinch
        bool can_move = !is_attacking && !player.is_dead && !player.is_flinching && !(player.active_character == 2 && ben_state.is_dancing);
        bool can_rotate = false;
        if ((player.active_character == 1 && swampfire_state.q_state > 0) || 
            (player.active_character == 0 && bigchill_cloaked_state.is_q_attacking)) {
            can_rotate = true;
        }
        UpdatePosition(can_move, can_rotate);

        // Handle proximity spawning
        for (int i = 0; i < g_num_spawn_points; ++i) {
            if (g_spawn_points[i].enemies_spawned < g_spawn_points[i].max_enemies && 
                g_spawn_points[i].active_enemy_id == -1) {
                if (glm::distance(player.position, g_spawn_points[i].position) < 5.0f) {
                    SpawnEnemy(g_spawn_points[i].position, i);
                }
            }
        }

        UpdateEnemies();
        UpdateCollectibles();
        UpdateBreakables();
        UpdateFragments();
        ProcessEnemyMeleeHitboxes();
        // Process Big Chill hitboxes if active
        if (player.active_character == 0) {
            BigChillAnimResult& activeRes = player.jumping ? bigchillBen10Res : bigchillCloakedRes;
            BigChillAnimState& activeState = player.jumping ? bigchill_ben10_state : bigchill_cloaked_state;
            
            ProcessBigChillMeleeHitboxes(activeRes, activeState, CHILL, bc_punch_just_triggered);
            if (activeRes.magic_active) {
                glm::vec3 forward(sin(player.rotate), 0.0f, cos(player.rotate));
                glm::vec3 spawn_pos = player.position + forward * 0.2f + glm::vec3(0.02f, 0.85f, 0.02f);
                ParticleOptions popts;
                popts.color = HexToRgb("#00FFFF"); // Vibrant Cyan
                popts.life = 0.3f;
                popts.scale = 0.20f;
                popts.speed = 2.0f;
                popts.count = 20;
                Particles_SpawnDirectional(spawn_pos, forward, 0.4f, popts);
            }
        }

        // Draw controlled BigChill if visible
        if (player.active_character == 0)
        {
            model = Matrix_Translate(player.position.x, player.position.y, player.position.z)
                    * Matrix_Scale(player.characters[0].scale, player.characters[0].scale, player.characters[0].scale)
                    * Matrix_Rotate_Y(player.rotate);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, CHILL);

            
            static float bigchill_wings_alpha = 0.0f;
            float transition_speed = 4.0f;
            if (player.jumping) {
                bigchill_wings_alpha += delta_t * transition_speed;
                if (bigchill_wings_alpha > 1.0f) bigchill_wings_alpha = 1.0f;
            } else {
                bigchill_wings_alpha -= delta_t * transition_speed;
                if (bigchill_wings_alpha < 0.0f) bigchill_wings_alpha = 0.0f;
            }

            static float g_WingsRotationY = 0.0f;
            if (keys[GLFW_KEY_LEFT_BRACKET]) {
                g_WingsRotationY -= delta_t * 2.0f;
                printf("Wings Rotation Y: %f\n", g_WingsRotationY);
            }
            if (keys[GLFW_KEY_RIGHT_BRACKET]) {
                g_WingsRotationY += delta_t * 2.0f;
                printf("Wings Rotation Y: %f\n", g_WingsRotationY);
            }

            bool loop_ben10 = (bigchillBen10Res.current_anim_index != 1 && bigchillBen10Res.current_anim_index != 0 && bigchillBen10Res.current_anim_index != 2 && bigchillBen10Res.current_anim_index != 9);
            bigchillBen10Animator.update(bigchill_model, bigchillBen10Res.current_anim_index, bigchillBen10Res.anim_time_to_pass, loop_ben10);
            const auto& bigchillBen10Bones = bigchillBen10Animator.getBoneMatrices();
            
            bool loop_cloaked = (bigchillCloakedRes.current_anim_index != 1 && bigchillCloakedRes.current_anim_index != 0 && bigchillCloakedRes.current_anim_index != 5);
            bigchillCloakedAnimator.update(bigchill_cloaked_model, bigchillCloakedRes.current_anim_index, bigchillCloakedRes.anim_time_to_pass, loop_cloaked);
            const auto& bigchillCloakedBones = bigchillCloakedAnimator.getBoneMatrices();
            
            static bool castle_reached = false;
            if (!castle_reached && player.position.x >= 11.0f && player.position.z >= -99.14f && player.position.z <= -82.65f) {
                castle_reached = true;
                PlayTransition("../../data/sounds/transition.mp3", "../../data/sounds/song2.mp3");
            }
            
            for (const auto& pair : g_VirtualScene) {
                bool is_ben10 = (pair.first.find("the_bigchill_") == 0 && pair.first.find("the_bigchill_cloaked") != 0 && pair.first.find("the_bigchill_uaf") != 0);
                bool is_cloaked = (pair.first.find("the_bigchill_cloaked") == 0);
                
                // HIDE the cloak from big_chill_ben_10 model
                if (is_ben10 && (pair.first == "the_bigchill_1" || pair.first == "the_bigchill_2")) continue;
                
                if (is_ben10 || is_cloaked) {
                    float current_alpha = 1.0f;
                    if (is_ben10) current_alpha = bigchill_wings_alpha;
                    else if (is_cloaked) current_alpha = 1.0f - bigchill_wings_alpha;
                    
                    if (current_alpha <= 0.01f) continue;
                    
                    if (current_alpha < 0.99f) {
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    } else {
                        glDisable(GL_BLEND);
                    }
                    
                    glUniform1f(glGetUniformLocation(g_GpuProgramID, "bigchill_part_alpha"), current_alpha);
                    
                    glActiveTexture(GL_TEXTURE2);
                    GLuint tex_to_use = pair.second.texture_id;
                    if (tex_to_use == 0 && g_LoadedTextureIDs.size() > 3) {
                        glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[2]); // bcck1.png
                        glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
                        
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[3]); // bcck2.png
                        glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
                    } else {
                        glBindTexture(GL_TEXTURE_2D, tex_to_use);
                        glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
                    }
                    
                    if (g_bone_matrices_uniform >= 0) {
                        if (is_ben10) {
                            bool is_wings = (pair.first == "the_bigchill_3");
                            if (is_wings) {
                                std::vector<glm::mat4> idBones(100, Matrix_Identity());
                                idBones[0] = Matrix_Rotate_Y(g_WingsRotationY);
                                glUniformMatrix4fv(g_bone_matrices_uniform, 100, GL_FALSE, glm::value_ptr(idBones[0]));
                            } else if (!bigchillBen10Bones.empty()) {
                                glUniformMatrix4fv(g_bone_matrices_uniform, (GLsizei)bigchillBen10Bones.size(), GL_FALSE, (const GLfloat*)bigchillBen10Bones.data());
                            } else {
                                std::vector<glm::mat4> idBones(100, Matrix_Identity());
                                glUniformMatrix4fv(g_bone_matrices_uniform, 100, GL_FALSE, glm::value_ptr(idBones[0]));
                            }
                        } else if (is_cloaked) {
                            if (!bigchillCloakedBones.empty()) {
                                glUniformMatrix4fv(g_bone_matrices_uniform, (GLsizei)bigchillCloakedBones.size(), GL_FALSE, (const GLfloat*)bigchillCloakedBones.data());
                            } else {
                                std::vector<glm::mat4> idBones(100, Matrix_Identity());
                                glUniformMatrix4fv(g_bone_matrices_uniform, 100, GL_FALSE, glm::value_ptr(idBones[0]));
                            }
                        }
                    }
                    
                    bool disable_culling = false;
                    if (is_ben10 && (pair.first == "the_bigchill_1" || pair.first == "the_bigchill_2")) disable_culling = true;
                    if (is_cloaked) disable_culling = true;
                    
                    if (disable_culling) glDisable(GL_CULL_FACE);
                    else glEnable(GL_CULL_FACE);
                    
                    DrawVirtualObject(pair.first.c_str());
                    
                    if (disable_culling) glEnable(GL_CULL_FACE);
                    
                    glDisable(GL_BLEND);
                }
            }
            glUniform1f(glGetUniformLocation(g_GpuProgramID, "bigchill_part_alpha"), 1.0f);
            DrawBoundingBox(player.characters[0].bbox, CHILL);
            glEnable(GL_CULL_FACE);
        }

        // Swampfire and Ben computations are now done above

        // Draw Swampfire instances if visible
        if (player.active_character == 1)
        {
            int current_anim_index = animRes.current_anim_index;
            ProcessSwampfireMeleeHitboxes(animRes, swampfire_state, SWAMPFIRE, sf_punch_just_triggered);
            float anim_time_to_pass = animRes.anim_time_to_pass;

            // Atualiza o animador modular
            swampfireAnimator.update(gltfmodel, current_anim_index, anim_time_to_pass, !player.is_dead);

            // Envia para a placa de vídeo
            const auto& boneMatrices = swampfireAnimator.getBoneMatrices();
            if (g_bone_matrices_uniform >= 0 && !boneMatrices.empty()) {
                glUniformMatrix4fv(g_bone_matrices_uniform, 
                                   (GLsizei)boneMatrices.size(), 
                                   GL_FALSE, 
                                   (const GLfloat*)boneMatrices.data());
            }

            for (int i = 0; i < 20; i++) {
                std::string name = "the_swampfire_" + std::to_string(i);
                if (g_VirtualScene.find(name) != g_VirtualScene.end()) {
                    float anim_y_offset = 0.0f;
                    float anim_x_offset = 0.0f;
                    float anim_z_offset = 0.0f;

                    // Idle, Walk, Charging, Launching Fireball
                    if (current_anim_index == 6 ||
                        current_anim_index == 8 ||
                        current_anim_index == 2 ||
                        current_anim_index == 3)
                    {
                        anim_y_offset = 0.085f;
                        anim_x_offset = 0.0f;
                        anim_z_offset = -0.275f;
                    }

                    // A matriz model aplica offset em X (eixo lateral local) após a rotação!
                    model = Matrix_Translate(player.position.x, player.position.y + anim_y_offset, player.position.z)
                          * Matrix_Rotate_Y(player.rotate - (3.14159265f / 6))
                          * Matrix_Translate(anim_x_offset, 0.0f, anim_z_offset)
                          * Matrix_Scale(player.characters[1].scale, player.characters[1].scale, player.characters[1].scale)
                          * Matrix_Rotate_X(0.175f);
                    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, g_VirtualScene[name].texture_id);
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
                    glUniform1i(g_object_id_uniform, SWAMPFIRE);

                    DrawVirtualObject(name.c_str());
                    DrawBoundingBox(player.characters[1].bbox, SWAMPFIRE);
                }
            }
        }

        // Draw Ben Tennyson instances if visible
        if (player.active_character == 2)
        {
            ProcessBenMeleeHitboxes(benRes, ben_state, BENTENNYSON, ben_punch_just_triggered || ben_slap_just_triggered);
            float ben_y_offset = 0.0f;
            if (player.is_dead) {
                ben_y_offset = 0.10f;
            } else if (player.is_flinching) {
                ben_y_offset = -0.110f;
            } else if (ben_state.is_q_attacking) {
                ben_y_offset = -0.12f;
            }
            model = Matrix_Translate(player.position.x, player.position.y + ben_y_offset, player.position.z)
                  * Matrix_Scale(player.characters[2].scale, player.characters[2].scale, player.characters[2].scale)
                  * Matrix_Rotate_Y(player.rotate);

            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BENTENNYSON);

            bentennysonAnimator.update(bentennyson_model, benRes.current_anim_index, benRes.anim_time_to_pass, !(player.is_dead || player.is_flinching));
            const auto& benBones = bentennysonAnimator.getBoneMatrices();
            
            if (g_bone_matrices_uniform >= 0) {
                if (!benBones.empty()) {
                    glUniformMatrix4fv(g_bone_matrices_uniform, (GLsizei)benBones.size(), GL_FALSE, (const GLfloat*)benBones.data());
                } else {
                    std::vector<glm::mat4> idBones(100, Matrix_Identity());
                    glUniformMatrix4fv(g_bone_matrices_uniform, 100, GL_FALSE, glm::value_ptr(idBones[0]));
                }
            }

            for (const auto& pair : g_VirtualScene) {
                if (pair.first.find("the_bentennyson_") == 0) {
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_2D, pair.second.texture_id);
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
                    DrawVirtualObject(pair.first.c_str());
                }
            }
            DrawBoundingBox(player.characters[2].bbox, BENTENNYSON);
        }

            // Fireball projectiles: spawn on Q-release (strength provided by animRes), update and draw via modular API
            if (g_VirtualScene.find("the_sphere") != g_VirtualScene.end()) {
                // Only spawn fireballs when the Swampfire character is active/visible
                if (player.active_character == 1 && animRes.spawn_fireball_strength > 0.0f) {
                    glm::vec3 ppos(player.position.x, player.position.y, player.position.z);
                    // spawn a sphere projectile (model base name "the_sphere")
                    Projectiles_Spawn(std::string("the_sphere"), animRes.spawn_fireball_strength, ppos, player.rotate);
                }
                Projectiles_Update(delta_t);
                // Update particles (projectiles emit particles each update)
                Particles_Update(delta_t);
                // Draw projectiles using the static sphere model from the virtual scene
                Projectiles_Draw(emptyModel, fireballAnimator, g_GpuProgramID, g_model_uniform, g_bone_matrices_uniform, g_object_id_uniform, g_VirtualScene, std::string("the_sphere"), player.characters[1].scale, FIREBALL);

                // (TEST SPHERE REMOVED)
            }


        bool t_is_down = keys[GLFW_KEY_T];


        // 1. Matriz de Modelo (Posição, Escala e Rotação do mapa)
        // Supondo que ele deve ficar na origem do mundo e com tamanho normal
        model = Matrix_Translate(0.0f, 0.0f, 0.0f) * Matrix_Scale(1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    
        // 2. Definir o ID do objeto para o Shader saber qual IF usar
        glUniform1i(g_object_id_uniform, GROUND);
    
        // 3. Ativar e Bindar as texturas (as que você carregou para a unidade 9 e 10)
        // Ativa a Texture Unit 9
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[5]);
        // Substitua `textura_id_9` pela variável/dicionário onde você salvou o ID da textura gerado pelo OpenGL
        glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage9"), 9);
    
        // Ativa a Texture Unit 10 (se precisar da segunda textura simultaneamente)
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[6]);
        glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage10"), 10);

        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[8]);
        glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage11"), 11);

        glActiveTexture(GL_TEXTURE12);
        glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[9]);
        glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage12"), 12);

        glActiveTexture(GL_TEXTURE14);
        glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[10]);
        glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage14"), 14);

    
        // 4. Desenhar o Objeto Visual
        // Mude a string abaixo EXATAMENTE para o nome do objeto (mesh) salvo dentro do seu arquivo .obj
        glDisable(GL_CULL_FACE); // Desabilita culling para o chão, que é duplo-face
        for (const auto& shape : ground_model.shapes) {
            if (shape.name.find("Collider") != std::string::npos) {
                continue; // Skip physical collider meshes from being rendered visually
            }
            DrawVirtualObject(shape.name.c_str());
        }
        glEnable(GL_CULL_FACE); // Reabilita culling para os próximos objetos


        // Desenhamos o plano do marzao
        model = Matrix_Translate(0.0f, -2.0f, 0.0f)
                * Matrix_Scale(130.0f, 1.0f, 130.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE);
        DrawVirtualObject("the_plane");


        for (int i = 0; i < barracas_positions.size(); i++) {

            // Frustum Culling: Testamos se a barraca (assumindo um raio de 4.5 para cobrir o modelo) está na visão da câmera
            if (!IsSphereInFrustum(barracas_positions[i] + glm::vec3(0.0f, 1.0f, 0.0f), 4.5f, frustum_planes)) {
                continue;
            }

            if (i >= 7 && i < 14) {
                model = Matrix_Rotate_Y(-0.3f);
            } else {
                model = Matrix_Identity();
            }
            if (i % 3 == 0) {
                model = Matrix_Translate(barracas_positions[i].x, 1.0f, barracas_positions[i].z) * Matrix_Scale(1.4f, 1.4f, 1.4f) * model;
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, 50);
                for (const auto& shape : barraca_model.shapes) {
                    DrawVirtualObject(shape.name.c_str());
                }
            } else if (i % 3 == 1) {
                model = Matrix_Translate(barracas_positions[i].x, 1.0f, barracas_positions[i].z) * Matrix_Scale(1.4f, 1.4f, 1.4f) * model;
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, 51);
                for (const auto& shape : macarrao_model.shapes) {
                    DrawVirtualObject(shape.name.c_str());
                }
            } else {
                model = Matrix_Translate(barracas_positions[i].x, 1.0f, barracas_positions[i].z) * Matrix_Scale(1.4f, 1.4f, 1.4f) * model;
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, 52);
                for (const auto& shape : banana_model.shapes) {
                    DrawVirtualObject(shape.name.c_str());
                }
            }
        }

        for (int i = 0; i < wall_positions.size(); i++) {

            if (i <= 1) {
                model = Matrix_Identity();
            } else {
                model = Matrix_Rotate_Y(M_PI / 2.0f);
            }

            model = Matrix_Translate(wall_positions[i].x, wall_positions[i].y, wall_positions[i].z) * Matrix_Scale(0.9f, 0.9f, 0.9f) * model;
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, 11); // ID 11 samples TextureImage8 which we bind below
            for (const auto& pair : g_VirtualScene) {
                if (pair.first.find("the_wall_") == 0) {
                    glActiveTexture(GL_TEXTURE8);
                    glBindTexture(GL_TEXTURE_2D, pair.second.texture_id);
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
                    DrawVirtualObject(pair.first.c_str());
                }
            }
        }

        for (int i = 0; i < tower_positions.size(); i++) {
            model = Matrix_Translate(tower_positions[i].x, tower_positions[i].y, tower_positions[i].z);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, 11); 
            for (const auto& pair : g_VirtualScene) {
                if (pair.first.find("the_tower_") == 0) {
                    glActiveTexture(GL_TEXTURE8);
                    glBindTexture(GL_TEXTURE_2D, pair.second.texture_id);
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
                    DrawVirtualObject(pair.first.c_str());
                }
            }
        }

        // Desenhamos o Castelo
        // Scaled down by 0.01 so it's realistically sized, and placed within view at Z = -15
         model = Matrix_Translate(-4.693f, 0.0f, -90.1f) * Matrix_Scale(0.01f, 0.01f, 0.01f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CASTLE);
        for (const auto& pair : g_VirtualScene) {
            if (pair.first.find("the_castle_") == 0) {
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, pair.second.texture_id);
                glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
                
                // Desabilitar culling para garantir que o castelo seja visível por dentro e por fora
                glDisable(GL_CULL_FACE);
                DrawVirtualObject(pair.first.c_str());
                glEnable(GL_CULL_FACE);
            }
        }

        // Desenhamos a Roda Gigante (ferris wheel)
        {
            glm::mat4 fw_world = Matrix_Translate(5.3251f, 3.7862f, -58.783f)
                               * Matrix_Rotate_Y(M_PI / 2.0f)  // 90 graus no eixo Y
                               * Matrix_Scale(0.366f, 0.366f, 0.366f);
            DrawFerrisWheel(ferris_wheel_model, "the_ferris_wheel",
                           g_GpuProgramID, g_model_uniform, g_object_id_uniform,
                           fw_world, (float)agora, 0.4f);
        }

        // for (int i = 0; i < MAX_PLATFORMS; i++) {
        //     DrawBoundingBox(map[i].bbox, BBOX_DEBUG);
        // }

        // Draw particles (after opaque geometry)
        Particles_Draw(g_VirtualScene, g_GpuProgramID, g_model_uniform, g_object_id_uniform, 1.0f);

        // Desenhar o inimigo
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].visible) continue;

            float enemy_alpha = 1.0f;
            if (g_enemies[i].is_spawning) {
                enemy_alpha = g_enemies[i].spawn_timer / g_enemies[i].spawn_duration;
                if (enemy_alpha > 1.0f) enemy_alpha = 1.0f;
            } else if (g_enemies[i].is_flashing) {
                enemy_alpha = 1.0f - (g_enemies[i].flash_timer / 1.0f);
                if (enemy_alpha < 0.0f) enemy_alpha = 0.0f;
            }
            glUniform1f(glGetUniformLocation(g_GpuProgramID, "enemy_alpha"), enemy_alpha);

            if (!g_enemies[i].is_dead && !g_enemies[i].is_attacking && !g_enemies[i].is_flinching) {
                float dx = player.position.x - g_enemies[i].position.x;
                float dz = player.position.z - g_enemies[i].position.z;
                g_enemies[i].rotate = atan2(dx, dz);
            }

            int current_enemy_anim = 0; 
            float anim_time = agora;

            if (g_enemies[i].is_dead) {
                if (g_enemies[i].death_timer < g_enemies[i].death_anim_duration) {
                    current_enemy_anim = 16; 
                    anim_time = std::min(g_enemies[i].death_timer, 2.65f);
                } else {
                    current_enemy_anim = 17; 
                    anim_time = 0.0f; 
                }
            } else if (g_enemies[i].is_flinching) {
                current_enemy_anim = g_enemies[i].flinch_anim; 
                anim_time = g_enemies[i].flinch_timer;
            } else if (g_enemies[i].is_attacking) {
                if (g_enemies[i].type == 0) {
                    current_enemy_anim = 21; 
                    anim_time = g_enemies[i].attack_timer; 
                } else if (g_enemies[i].type == 1) {
                    if (g_enemies[i].attack_phase == 1) {
                        current_enemy_anim = 23; // Attack Idle
                        anim_time = g_enemies[i].attack_timer;
                    } else {
                        current_enemy_anim = 25; // Shooting
                        anim_time = g_enemies[i].attack_timer;
                    }
                }
            } else {
                float dist_to_player = glm::distance(
                    glm::vec3(g_enemies[i].position.x, 0.0f, g_enemies[i].position.z),
                    glm::vec3(player.position.x, 0.0f, player.position.z));
                if (dist_to_player > g_enemies[i].attack_range) {
                    current_enemy_anim = 34; 
                    anim_time = agora * 2.0f; 
                }
            }

            bool loop_anim = !(g_enemies[i].is_dead || g_enemies[i].is_flinching);
            if (g_enemies[i].type == 1 && (current_enemy_anim == 22 || current_enemy_anim == 23 || current_enemy_anim == 25)) {
                loop_anim = false;
            }
            foreverknightAnimator.update(foreverknight_model, current_enemy_anim, anim_time, loop_anim);

            float y_offset = (current_enemy_anim == 34) ? 0.2f : 0.0f;
            model = Matrix_Translate(g_enemies[i].position.x, g_enemies[i].position.y - 0.6f + y_offset, g_enemies[i].position.z)
                  * Matrix_Scale(0.8f, 0.8f, 0.8f)
                  * Matrix_Rotate_Y(g_enemies[i].rotate - 1.08f)
                  * Matrix_Translate(-3.985f, 0.043f, -0.205f);

            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, FOREVERKNIGHT);

            const auto& fkBones = foreverknightAnimator.getBoneMatrices();
            if (g_bone_matrices_uniform >= 0) {
                if (!fkBones.empty()) {
                    glUniformMatrix4fv(g_bone_matrices_uniform, (GLsizei)fkBones.size(), GL_FALSE, (const GLfloat*)fkBones.data());
                } else {
                    std::vector<glm::mat4> idBones(100, Matrix_Identity());
                    glUniformMatrix4fv(g_bone_matrices_uniform, 100, GL_FALSE, glm::value_ptr(idBones[0]));
                }
            }

            if (enemy_alpha < 1.0f) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
            }

            for (const auto& pair : g_VirtualScene) {
                if (pair.first.find("the_foreverknight_") == 0) {
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_2D, pair.second.texture_id);
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
                    
                    glDisable(GL_CULL_FACE);
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "is_frozen"), g_enemies[i].is_frozen ? 1 : 0);
                    DrawVirtualObject(pair.first.c_str());
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "is_frozen"), 0);
                    glEnable(GL_CULL_FACE);
                }
            }
            
            if (enemy_alpha < 1.0f) {
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
            }

            DrawBoundingBox(g_enemies[i].bbox, BUNNY);
        }
        glUniform1f(glGetUniformLocation(g_GpuProgramID, "enemy_alpha"), 1.0f);

        GLint override_kd_uniform = glGetUniformLocation(g_GpuProgramID, "OverrideKd");
        GLint use_override_kd_uniform = glGetUniformLocation(g_GpuProgramID, "UseOverrideKd");
        DrawBreakables(g_model_uniform, g_object_id_uniform, override_kd_uniform, use_override_kd_uniform);
        DrawFragments(g_model_uniform, g_object_id_uniform, override_kd_uniform, use_override_kd_uniform);

        // Draw Collectibles
        for (int i = 0; i < MAX_COLLECTIBLES; i++) {
            if (g_collectibles[i].active && g_collectibles[i].visible_this_frame) {
                float c_alpha = 1.0f;
                if (g_collectibles[i].timer >= g_collectibles[i].blink_time) {
                    c_alpha = 1.0f - (g_collectibles[i].timer - g_collectibles[i].blink_time) / (g_collectibles[i].duration - g_collectibles[i].blink_time);
                    if (c_alpha < 0.0f) c_alpha = 0.0f;
                }
                
                glEnable(GL_BLEND);
                glDepthMask(GL_FALSE);
                
                // 1) Outer semi-transparent colored sphere (Transparência ajustada para 85% de opacidade)
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glUniform1f(glGetUniformLocation(g_GpuProgramID, "enemy_alpha"), c_alpha * 0.85f); 

                model = Matrix_Translate(g_collectibles[i].position.x, g_collectibles[i].position.y, g_collectibles[i].position.z)
                      * Matrix_Scale(g_collectibles[i].scale, g_collectibles[i].scale, g_collectibles[i].scale);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                glUniform1i(g_object_id_uniform, 16 + g_collectibles[i].type); 
                DrawVirtualObject("the_sphere");

                // 2) Inner solid bright sphere (Pisca com alpha máximo 50%)
                glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Mesclar com a de fora luminosamente
                float blink_alpha = (sin(glfwGetTime() * 15.0f) + 1.0f) * 0.25f; // Oscillates from 0.0 to 0.5
                glUniform1f(glGetUniformLocation(g_GpuProgramID, "enemy_alpha"), c_alpha * blink_alpha); 
                
                model = Matrix_Translate(g_collectibles[i].position.x, g_collectibles[i].position.y, g_collectibles[i].position.z)
                      * Matrix_Scale(g_collectibles[i].scale * 0.4f, g_collectibles[i].scale * 0.4f, g_collectibles[i].scale * 0.4f);
                glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
                
                glUniform1i(g_object_id_uniform, 19); // INNER CORE WITH BLINK
                DrawVirtualObject("the_sphere");

                // Reset
                glUniform1f(glGetUniformLocation(g_GpuProgramID, "enemy_alpha"), 1.0f);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
            }
        }

        // Draw HUD
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glm::mat4 id_mat = Matrix_Identity();
        glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(id_mat));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(id_mat));
        
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[7]);

        // Draw Element 0 (Health BG)
        model = Matrix_Translate(g_ui_items[0].x, g_ui_items[0].y, 0.0f)
                        * Matrix_Scale(g_ui_items[0].scale_x, g_ui_items[0].scale_y, 1.0f)
                        * Matrix_Rotate_X(M_PI / 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, 20); // HUD_BAR_BG
        DrawVirtualObject("the_plane");

        // Draw Element 1 (Health FG)
        float h_ratio = player.health / 100.0f;
        glUniform1f(g_hud_health_ratio_uniform, h_ratio);
        if (h_ratio > 0.0f) {
            model = Matrix_Translate(g_ui_items[1].x, g_ui_items[1].y - (1.0f - h_ratio)*g_ui_items[1].scale_y, 0.0f)
                  * Matrix_Scale(g_ui_items[1].scale_x, g_ui_items[1].scale_y * h_ratio, 1.0f)
                  * Matrix_Rotate_X(M_PI / 2.0f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, 21); // HUD_BAR_FG
            DrawVirtualObject("the_plane");
        }

        // Draw Element 2 (Grey Container)
        model = Matrix_Translate(g_ui_items[2].x, g_ui_items[2].y, 0.0f)
              * Matrix_Scale(g_ui_items[2].scale_x, g_ui_items[2].scale_y, 1.0f)
              * Matrix_Rotate_X(M_PI / 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, 24); // HUD_BAR_CONTAINER2
        DrawVirtualObject("the_plane");

        // Draw Element 3 (Green Bar)
        float bar2_ratio = player.transform_energy / player.max_transform_energy;
        glUniform1f(g_hud_bar2_ratio_uniform, bar2_ratio);
        if (bar2_ratio > 0.0f) {
            model = Matrix_Translate(g_ui_items[3].x, g_ui_items[3].y - (1.0f - bar2_ratio)*g_ui_items[3].scale_y, 0.0f)
                  * Matrix_Scale(g_ui_items[3].scale_x, g_ui_items[3].scale_y * bar2_ratio, 1.0f)
                  * Matrix_Rotate_X(M_PI / 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, 22); // HUD_BAR_GREEN
        DrawVirtualObject("the_plane");
        }

        // Draw Element 4 (Yellow Bar)
        float bar3_ratio = player.special_energy / player.max_special_energy;
        glUniform1f(g_hud_bar3_ratio_uniform, bar3_ratio);
        if (bar3_ratio > 0.0f) {
            model = Matrix_Translate(g_ui_items[4].x, g_ui_items[4].y - (1.0f - bar3_ratio)*g_ui_items[4].scale_y, 0.0f)
                  * Matrix_Scale(g_ui_items[4].scale_x, g_ui_items[4].scale_y * bar3_ratio, 1.0f)
                  * Matrix_Rotate_X(M_PI / 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, 23); // HUD_BAR_YELLOW
        DrawVirtualObject("the_plane");
        }

        // Draw Element 5 (Cap Top)
        model = Matrix_Translate(g_ui_items[5].x, g_ui_items[5].y, 0.0f)
              * Matrix_Scale(g_ui_items[5].scale_x, g_ui_items[5].scale_y, 1.0f)
              * Matrix_Rotate_Z(M_PI)
              * Matrix_Rotate_X(M_PI / 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, 25); // HUD_BAR_CAP
        DrawVirtualObject("the_plane");

        // Draw Element 6 (Cap Bottom)
        model = Matrix_Translate(g_ui_items[6].x, g_ui_items[6].y, 0.0f)
              * Matrix_Scale(g_ui_items[6].scale_x, g_ui_items[6].scale_y, 1.0f)
              * Matrix_Rotate_Z(M_PI)
              * Matrix_Rotate_X(M_PI / 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, 25); // HUD_BAR_CAP
        DrawVirtualObject("the_plane");

        // Draw Omnitrix Button
        glUniform1f(g_hud_omnitrix_frame_uniform, g_omnitrix_anim_frame);
        model = Matrix_Translate(g_ui_items[7].x, g_ui_items[7].y, 0.0f)
              * Matrix_Scale(g_ui_items[7].scale_x, g_ui_items[7].scale_y, 1.0f)
              * Matrix_Rotate_X(M_PI / 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, 26); // HUD_OMNITRIX
        DrawVirtualObject("the_plane");

        if (player.active_character == 2 && !player.is_dead && g_omnitrix_anim_frame >= 15.0f) {
            model = Matrix_Translate(g_ui_items[8].x, g_ui_items[8].y, 0.0f)
                  * Matrix_Scale(g_ui_items[8].scale_x, g_ui_items[8].scale_y, 1.0f)
                  * Matrix_Rotate_X(M_PI / 2.0f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, 27); // HUD_HOLOGRAM_LIGHT
            DrawVirtualObject("the_plane");

            int ui_idx = (player.selected_alien == 0) ? 9 : 10;
            model = Matrix_Translate(g_ui_items[ui_idx].x, g_ui_items[ui_idx].y, 0.0f) // Draw hologram slightly above the light base
                  * Matrix_Scale(g_ui_items[ui_idx].scale_x, g_ui_items[ui_idx].scale_y, 1.0f)
                  * Matrix_Rotate_X(M_PI / 2.0f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, player.selected_alien == 0 ? 28 : 29); // 28: Big Chill, 29: Swampfire
            DrawVirtualObject("the_plane");
        }

        DrawBoundingBox(player.characters[player.active_character].bbox, 0);
        // ----------------------------

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // Draw Health (removido, usando apenas a barra)

        // Respawn Logic & Death Message
        if (player.is_dead || player.has_won) {
            player.death_timer += delta_t;
            if (player.final_time == 0.0f) {
                player.final_time = (float)glfwGetTime();
            }

            if (player.death_timer > 2.5f) {
                glUseProgram(g_GpuProgramID);
                DrawTextWindowBox(g_model_uniform, g_view_uniform, g_projection_uniform, g_object_id_uniform);

            int w, h;
            glfwGetWindowSize(window, &w, &h);
            float text_scale_title = (float)h / 600.0f * 1.5f;
            float text_scale_body = (float)h / 600.0f * 1.0f;
            
            const char* title_text = player.has_won ? "You win!" : "You died!";
            
            int elapsed = (int)(player.final_time - player.start_time);
            char stats_time[64]; snprintf(stats_time, sizeof(stats_time), "Time - %02d:%02d", elapsed / 60, elapsed % 60);
            char stats_enemies[64]; snprintf(stats_enemies, sizeof(stats_enemies), "Enemies slain - %d", player.enemies_slain);
            char stats_objs[64]; snprintf(stats_objs, sizeof(stats_objs), "Objects destroyed - %d", player.objects_destroyed);

            float title_w = TextRendering_GetStringWidth(window, title_text, text_scale_title);
            float time_w = TextRendering_GetStringWidth(window, stats_time, text_scale_body);
            float enemies_w = TextRendering_GetStringWidth(window, stats_enemies, text_scale_body);
            float objs_w = TextRendering_GetStringWidth(window, stats_objs, text_scale_body);
            float enter_w = TextRendering_GetStringWidth(window, "Press ENTER to respawn", text_scale_body);

            TextRendering_PrintString(window, title_text, -title_w/2.0f, 0.25f, text_scale_title, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); 
            TextRendering_PrintString(window, stats_time, -time_w/2.0f, 0.10f, text_scale_body, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); 
            TextRendering_PrintString(window, stats_enemies, -enemies_w/2.0f, 0.00f, text_scale_body, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); 
            TextRendering_PrintString(window, stats_objs, -objs_w/2.0f, -0.10f, text_scale_body, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); 
            TextRendering_PrintString(window, "Press ENTER to respawn", -enter_w/2.0f, -0.25f, text_scale_body, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

                if (keys[GLFW_KEY_ENTER]) {
                    inner_loop_running = false;
                }
            }
        } else if (player.is_flinching) {
            player.flinch_timer += delta_t;
            if (player.flinch_timer >= 0.5f) {
                player.is_flinching = false;
            }
        }


        auto drawBorderText = [&](const std::string& text, float x, float y, float scale, float alpha) {
            TextRendering_PrintString(window, text, x - 0.002f, y, scale, glm::vec4(0.0f, 0.0f, 0.0f, alpha));
            TextRendering_PrintString(window, text, x + 0.002f, y, scale, glm::vec4(0.0f, 0.0f, 0.0f, alpha));
            TextRendering_PrintString(window, text, x, y - 0.002f, scale, glm::vec4(0.0f, 0.0f, 0.0f, alpha));
            TextRendering_PrintString(window, text, x, y + 0.002f, scale, glm::vec4(0.0f, 0.0f, 0.0f, alpha));
            TextRendering_PrintString(window, text, x, y, scale, glm::vec4(1.0f, 1.0f, 1.0f, alpha));
        };

        // Draw Attack UI
        auto getAttackAlpha = [&](const AttackUI& atk) {
            float alpha = 1.0f;
            if (atk.timer > 1.0f) {
                // fade to 0.4f alpha between 1.0s and 3.0s
                alpha = 1.0f - 0.6f * ((atk.timer - 1.0f) / 2.0f);
                if (alpha < 0.4f) alpha = 0.4f;
            }
            if (atk.timer > 3.0f) {
                // fade out completely while moving
                alpha = 0.4f - 0.4f * ((atk.timer - 3.0f) / 1.0f);
                if (alpha < 0.0f) alpha = 0.0f;
            }
            return alpha;
        };

        if (player.previous_attack.active) {
            float alpha = getAttackAlpha(player.previous_attack);
            float x_pos = g_ui_items[12].x + player.previous_attack.x_offset;
            drawBorderText(player.previous_attack.text, x_pos, g_ui_items[12].y, g_ui_items[12].scale_x, alpha);
        }
        if (player.recent_attack.active) {
            float alpha = getAttackAlpha(player.recent_attack);
            float x_pos = g_ui_items[11].x + player.recent_attack.x_offset;
            drawBorderText(player.recent_attack.text, x_pos, g_ui_items[11].y, g_ui_items[11].scale_x, alpha);
        }

        // Draw Debug UI
        if (g_ui_debug_enabled) {
            float start_y = 0.8f;
            TextRendering_PrintString(window, "=== UI CONFIG ===", -0.95f, start_y, 1.0f, glm::vec4(0,0,0,1));
            start_y -= 0.05f;
            for (int i = 0; i < 13; i++) {
                char buf[256];
                snprintf(buf, sizeof(buf), "%c %s  [X: %.3f, Y: %.3f, SX: %.3f, SY: %.3f]", 
                    (i == g_ui_selected_elem) ? '>' : ' ',
                    g_ui_items[i].name,
                    g_ui_items[i].x, g_ui_items[i].y, g_ui_items[i].scale_x, g_ui_items[i].scale_y);
                
                glm::vec4 color = (i == g_ui_selected_elem) ? glm::vec4(0.2f,0.2f,0.2f,1.0f) : glm::vec4(0.0f,0.0f,0.0f,1.0f);
                TextRendering_PrintString(window, buf, -0.95f, start_y, 0.65f, color);
                start_y -= 0.035f;
            }
            char inst[256];
            snprintf(inst, sizeof(inst), "Selected Param: %s. Keys: U(toggle) P(print) 1/2(elem) 3/4(param) -/+(val)", 
                (g_ui_selected_param == 0) ? "X Offset" : ((g_ui_selected_param == 1) ? "Y Offset" : ((g_ui_selected_param == 2) ? "Scale X" : "Scale Y")));
            TextRendering_PrintString(window, inst, -0.95f, start_y - 0.02f, 0.8f, glm::vec4(0,0,0,1));
        }

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
        ProcessGamepadInput(window);

    } // end of inner loop
    } // end of outer loop

    // Finalizamos o uso dos recursos do sistema operacional
    CleanupSoundSystem();
    glfwTerminate();

    // Fim do programa
    return 0;
}

void DrawBoundingBox(AABB& aabb, int restore_object_id) {
    // Debug views removed by user request
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename, bool use_rgba)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    int req_channels = use_rgba ? 4 : 3;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, req_channels);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    GLint internal_format = use_rgba ? GL_SRGB8_ALPHA8 : GL_SRGB8;
    GLenum format = use_rgba ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    // Save IDs so we can re-bind these textures reliably at draw time
    g_LoadedTextureIDs.push_back(texture_id);
    g_LoadedSamplerIDs.push_back(sampler_id);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis de AABB do fragment shader com os limites do modelo.
    const AABB& aabb = g_VirtualScene[object_name].aabb;
    glUniform4f(g_aabb_min_uniform, aabb.min.x, aabb.min.y, aabb.min.z, 1.0f);
    glUniform4f(g_aabb_max_uniform, aabb.max.x, aabb.max.y, aabb.max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_aabb_min_uniform   = glGetUniformLocation(g_GpuProgramID, "aabb_min");
    g_aabb_max_uniform   = glGetUniformLocation(g_GpuProgramID, "aabb_max");
    g_bone_matrices_uniform = glGetUniformLocation(g_GpuProgramID, "boneMatrices[0]");
    g_hud_health_ratio_uniform = glGetUniformLocation(g_GpuProgramID, "hud_health_ratio");
    g_hud_bar2_ratio_uniform = glGetUniformLocation(g_GpuProgramID, "hud_bar2_ratio");
    g_hud_bar3_ratio_uniform = glGetUniformLocation(g_GpuProgramID, "hud_bar3_ratio");
    g_hud_omnitrix_frame_uniform = glGetUniformLocation(g_GpuProgramID, "hud_omnitrix_frame");
    g_current_time_uniform = glGetUniformLocation(g_GpuProgramID, "current_time");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage15"), 15);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage16"), 16);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage13"), 13);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage17"), 17);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage18"), 18);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage19"), 19);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage20"), 20);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage21"), 21);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage22"), 22);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage23"), 23);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage24"), 24);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage25"), 25);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage26"), 26);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage27"), 27);
    glUseProgram(0);
}

void LoadUITexture(const char* filename, GLuint unit) {
    printf("Carregando UI imagem \"%s\"... ", filename);
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 4);
    if (!data) {
        fprintf(stderr, "ERROR: Cannot open UI image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    printf("OK (%dx%d).\n", width, height);

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(data);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice e que pertencem ao mesmo "smoothing group".

    // Obtemos a lista dos smoothing groups que existem no objeto
    std::set<unsigned int> sgroup_ids;
    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        assert(model->shapes[shape].mesh.smoothing_group_ids.size() == num_triangles);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            unsigned int sgroup = model->shapes[shape].mesh.smoothing_group_ids[triangle];
            assert(sgroup >= 0);
            sgroup_ids.insert(sgroup);
        }
    }

    size_t num_vertices = model->attrib.vertices.size() / 3;
    model->attrib.normals.reserve( 3*num_vertices );

    // Processamos um smoothing group por vez
    for (const unsigned int & sgroup : sgroup_ids)
    {
        std::vector<int> num_triangles_per_vertex(num_vertices, 0);
        std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

        // Acumulamos as normais dos vértices de todos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                glm::vec4  vertices[3];
                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                    const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                    const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                    vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
                }

                const glm::vec4  a = vertices[0];
                const glm::vec4  b = vertices[1];
                const glm::vec4  c = vertices[2];

                const glm::vec4  n = crossproduct(b-a,c-a);

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    num_triangles_per_vertex[idx.vertex_index] += 1;
                    vertex_normals[idx.vertex_index] += n;
                }
            }
        }

        // Computamos a média das normais acumuladas
        std::vector<size_t> normal_indices(num_vertices, 0);

        for (size_t vertex_index = 0; vertex_index < vertex_normals.size(); ++vertex_index)
        {
            if (num_triangles_per_vertex[vertex_index] == 0)
                continue;

            glm::vec4 n = vertex_normals[vertex_index] / (float)num_triangles_per_vertex[vertex_index];
            n /= norm(n);

            model->attrib.normals.push_back( n.x );
            model->attrib.normals.push_back( n.y );
            model->attrib.normals.push_back( n.z );

            size_t normal_index = (model->attrib.normals.size() / 3) - 1;
            normal_indices[vertex_index] = normal_index;
        }

        // Escrevemos os índices das normais para os vértices dos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index =
                        normal_indices[ idx.vertex_index ];
                }
            }
        }

    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;
    std::vector<float>  texture_selector_coefficients;
    std::map<int, float> fallback_texture_unit_by_material_id;
    float next_fallback_texture_unit = 2.0f;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::lowest();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 aabb_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 aabb_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            int material_id = -1;
            if (triangle < model->shapes[shape].mesh.material_ids.size())
            {
                material_id = model->shapes[shape].mesh.material_ids[triangle];
            }

            float texture_selector = 2.0f; // Default
            if (material_id >= 0 && material_id < (int)model->materials.size())
            {
                const auto& material = model->materials[material_id];
                std::string mat_name = material.name;
                
                if (mat_name.find("bcck2") != std::string::npos)
                    texture_selector = 3.0f;
                else if (mat_name.find("bcck1") != std::string::npos)
                    texture_selector = 2.0f;
                else if (mat_name.find("paredes") != std::string::npos || mat_name.find("concrete") != std::string::npos)
                    texture_selector = 10.0f;
                else if (mat_name.find("Barrier") != std::string::npos || mat_name.find("barrier") != std::string::npos)
                    texture_selector = 17.0f;
                else if (mat_name.find("Material.001") != std::string::npos || mat_name.find("wood") != std::string::npos)
                    texture_selector = 9.0f;
                else if (mat_name.find("sides") != std::string::npos)
                    texture_selector = 12.0f;
                else if (mat_name.find("top") != std::string::npos)
                    texture_selector = 14.0f;
                else if (mat_name.find("phong12") != std::string::npos)
                    texture_selector = 11.0f;
                else if (mat_name.find("phongE5") != std::string::npos)
                    texture_selector = 15.0f; // Blue
                else if (mat_name.find("phongE6") != std::string::npos || mat_name.find("phong32") != std::string::npos)
                    texture_selector = 16.0f; // Red
                else if (mat_name.find("phongE7") != std::string::npos || mat_name == "phong11" || mat_name == "phong2" || mat_name == "pasted__phong11" || mat_name == "pasted__phong2")
                    texture_selector = 17.0f; // Orange
                else if (mat_name.find("lambert4") != std::string::npos || mat_name.find("phong28") != std::string::npos || mat_name.find("phongE3") != std::string::npos || mat_name.find("phongE8") != std::string::npos || mat_name == "phong10" || mat_name == "phong14" || mat_name == "phong19" || mat_name == "phong4" || mat_name == "phong5" || mat_name == "phong7" || mat_name == "phong8" || mat_name == "phong9" || mat_name.find("phongE1") != std::string::npos || mat_name.find("phongE2") != std::string::npos || mat_name == "phong17")
                    texture_selector = 18.0f; // Dark Gray / Black
                else if (mat_name.find("Banner1") != std::string::npos) texture_selector = 25.0f;
                else if (mat_name.find("lambert1") != std::string::npos) texture_selector = 26.0f;
                // BANANA STALL
                else if (mat_name.find("マテリアル.033") != std::string::npos || mat_name.find("マテリアル.034") != std::string::npos || mat_name.find("マテリアル.029") != std::string::npos || mat_name.find("マテリアル.030") != std::string::npos || mat_name.find("マテリアル.028") != std::string::npos) texture_selector = 27.0f; // cb_0.png
                else if (mat_name.find("マテリアル.014") != std::string::npos) texture_selector = 20.0f; // kkgrmk_1.png
                else if (mat_name.find("マテリアル.009") != std::string::npos) texture_selector = 109.0f;
                else if (mat_name.find("マテリアル.019") != std::string::npos) texture_selector = 119.0f;
                else if (mat_name.find("マテリアル.032") != std::string::npos) texture_selector = 132.0f;
                else if (mat_name.find("マテリアル.021") != std::string::npos) texture_selector = 121.0f;
                else if (mat_name.find("マテリアル.022") != std::string::npos) texture_selector = 122.0f;
                else if (mat_name.find("マテリアル.036") != std::string::npos) texture_selector = 136.0f;
                else if (mat_name.find("マテリアル.035") != std::string::npos) texture_selector = 135.0f;
                else if (mat_name.find("マテリアル.037") != std::string::npos) texture_selector = 137.0f;
                else if (mat_name.find("マテリアル.020") != std::string::npos) texture_selector = 120.0f;
                else if (mat_name.find("マテリアル.024") != std::string::npos) texture_selector = 124.0f;
                else if (mat_name.find("マテリアル.004") != std::string::npos) texture_selector = 104.0f;
                else if (mat_name.find("マテリアル.018") != std::string::npos) texture_selector = 118.0f;
                else if (mat_name.find("マテリアル.026") != std::string::npos) texture_selector = 126.0f;
                else if (mat_name.find("マテリアル.027") != std::string::npos) texture_selector = 127.0f;
                else if (mat_name.find("マテリアル.025") != std::string::npos) texture_selector = 125.0f;
                // ICE STALL
                else if (mat_name.find("kkgrpblhwi") != std::string::npos) texture_selector = 21.0f;
                else if (mat_name.find("kkgrpicg") != std::string::npos) texture_selector = 22.0f;
                else if (mat_name.find("kkgrplmn") != std::string::npos) texture_selector = 23.0f;
                else if (mat_name.find("マテリアル.031") != std::string::npos) texture_selector = 31.0f;
                else if (mat_name.find("マテリアル.035") != std::string::npos) texture_selector = 35.0f;
                else if (mat_name.find("マテリアル.036") != std::string::npos) texture_selector = 36.0f;
                else if (mat_name.find("マテリアル.037") != std::string::npos) texture_selector = 37.0f;
                else if (mat_name.find("マテリアル.038") != std::string::npos) texture_selector = 38.0f;
                else if (mat_name.find("マテリアル.039") != std::string::npos) texture_selector = 39.0f;
                else if (mat_name.find("マテリアル.040") != std::string::npos) texture_selector = 40.0f;
                else if (mat_name.find("マテリアル.041") != std::string::npos) texture_selector = 24.0f; // Textured Roof
                else if (mat_name.find("マテリアル.042") != std::string::npos) texture_selector = 20.0f; // Textured Machine
                else if (mat_name.find("マテリアル.043") != std::string::npos) texture_selector = 43.0f;
                else if (mat_name.find("マテリアル.044") != std::string::npos) texture_selector = 44.0f;
                else if (mat_name.find("マテリアル.045") != std::string::npos) texture_selector = 45.0f;
                else if (mat_name.find("マテリアル.046") != std::string::npos) texture_selector = 46.0f;
                else if (mat_name.find("マテリアル.047") != std::string::npos) texture_selector = 47.0f;
                else if (mat_name.find("マテリアル.048") != std::string::npos) texture_selector = 48.0f;
                else if (mat_name.find("マテリアル.049") != std::string::npos) texture_selector = 49.0f;
                else if (mat_name.find("マテリアル") != std::string::npos) texture_selector = 31.0f; // Solid fallback color
                else if (mat_name.find("phong") != std::string::npos || mat_name.find("lambert") != std::string::npos)
                    texture_selector = 19.0f; // White / Default
            }

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                aabb_min.x = std::min(aabb_min.x, vx);
                aabb_min.y = std::min(aabb_min.y, vy);
                aabb_min.z = std::min(aabb_min.z, vz);
                aabb_max.x = std::max(aabb_max.x, vx);
                aabb_max.y = std::max(aabb_max.y, vy);
                aabb_max.z = std::max(aabb_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }

                texture_selector_coefficients.push_back(texture_selector);
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.aabb = AABB(aabb_min, aabb_max);

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_selector_coefficients.empty() )
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
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;

        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        float height_offset = 1.5f;

        glm::vec4 camera_lookat_l    = glm::vec4(player.position.x, player.position.y + height_offset, player.position.z, 1.0f);
        glm::vec4 camera_position_c  = camera_lookat_l + glm::vec4(x, y + 0.5, z, 0.0f);
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c;
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);
        
        printf("--- Camera Parameters (Click) ---\n");
        printf("  Position: (%.2f, %.2f, %.2f)\n", camera_position_c.x, camera_position_c.y, camera_position_c.z);
        printf("  LookAt:   (%.2f, %.2f, %.2f)\n", camera_lookat_l.x, camera_lookat_l.y, camera_lookat_l.z);
        printf("  View Vec: (%.2f, %.2f, %.2f)\n", camera_view_vector.x, camera_view_vector.y, camera_view_vector.z);
        printf("  Up Vec:   (%.2f, %.2f, %.2f)\n", camera_up_vector.x, camera_up_vector.y, camera_up_vector.z);
        printf("---------------------------------\n");
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (player.is_dead || player.has_won) return;

    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;
    
        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f/2;
        float phimin = -phimax;
    
        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;
    
        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

void Correcao_KeyCallback(int key, int action, int mod);

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // =======================
    // Não modifique esta chamada! Ela é utilizada para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    Correcao_KeyCallback(key, action, mod);
    // =======================

    // Keep keys[] mapping for continuous input handling (was in KeyMapping)
    if (action == GLFW_PRESS)
        keys[key] = true;
    else if (action == GLFW_RELEASE)
        keys[key] = false;

    // Debug keys removed by user request

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        if (mod == 0 && !(mod & GLFW_MOD_SHIFT) && player.active_character == 2 && !player.is_dead) {
            player.selected_alien = (player.selected_alien == 0) ? 1 : 0;
            PlayOmnitrixSound("../../data/sounds/omnitrix_switch.wav");
            printf("Selected Alien: %d\n", player.selected_alien);
        } else {
            g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
        }
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Spawn point debug keys removed
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        if (mod & GLFW_MOD_SHIFT)
            g_AngleZ -= delta;
        else if (mod == 0 && !(mod & GLFW_MOD_SHIFT) && !player.is_dead) {
            if (player.active_character == 2) {
                // Try to transform
                if (player.transform_energy < player.max_transform_energy) {
                    PlayOmnitrixSound("../../data/sounds/omnitrix_no_transform.wav");
                    printf("Energy not full! Cannot transform.\n");
                } else if (g_omnitrix_anim_frame < 15.0f) {
                    PlayOmnitrixSound("../../data/sounds/omnitrix_no_transform.wav");
                    printf("Omnitrix is not fully opened! Cannot transform.\n");
                } else {
                    // Start transformation
                    PlayOmnitrixSound("../../data/sounds/omnitrix_hit.wav");
                    g_transform_sound_timer = 0.35f;
                    g_play_transform_sound = true;

                    player.active_character = player.selected_alien;
                    glm::vec3 size = player.active_character == 0 ? bigchill_size : swampfire_size;
                    printf("Switched to character %d\n", player.active_character);
                    player.characters[player.active_character].bbox = makeAABBFromGround(player.position, size);
                    ResolvePlayerMapCollisions();

                    ParticleOptions popts;
                    popts.color = HexToRgb("#06b800"); // Green flash for voluntary transform
                    popts.life = 0.25f + 0.15f * 1.0f;
                    popts.scale = 0.15f + 0.01f * 6.0f;
                    popts.speed = 0.1f + 0.8f * 3.0f;
                    popts.count = std::max(2, (int)std::round(8.0f * 6.0f));
                    Particles_Spawn(glm::vec3(player.position.x, player.position.y, player.position.z), popts);
                }
            } else {
                // Voluntary revert back to Ben
                player.active_character = 2;
                PlayDetransformSound("../../data/sounds/omnitrix_detransform.wav");
                glm::vec3 size = bentennyson_size;
                printf("Switched back to character 2\n");
                player.characters[player.active_character].bbox = makeAABBFromGround(player.position, size);
                ResolvePlayerMapCollisions();

                ParticleOptions popts;
                popts.color = HexToRgb("#06b800"); // GREEN flash for voluntary revert
                popts.life = 0.25f + 0.15f * 1.0f;
                popts.scale = 0.15f + 0.01f * 6.0f;
                popts.speed = 0.1f + 0.8f * 3.0f;
                popts.count = std::max(2, (int)std::round(8.0f * 6.0f));
                Particles_Spawn(glm::vec3(player.position.x, player.position.y, player.position.z), popts);
            }
        }
    }

    // Toggle entre câmera fixa e câmera livre/look-at
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        g_UseFixedCameras = !g_UseFixedCameras;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    if (key == GLFW_KEY_7 && action == GLFW_PRESS)
    {
        PlayTransition("../../data/sounds/transition.mp3", "../../data/sounds/song2.mp3");
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler e dados da câmera.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    // snprintf(buffer, 80, "Player Pos = X(%.2f) Y(%.2f) Z(%.2f)\n", player.position.x, player.position.y, player.position.z);
    // TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);

    char cam_buf[120];
    snprintf(cam_buf, 120, "Camera = Dist(%.2f) Phi(%.2f) Theta(%.2f)", g_CameraDistance, g_CameraPhi, g_CameraTheta);
    TextRendering_PrintString(window, cam_buf, -1.0f+pad/10, -1.0f+4*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :