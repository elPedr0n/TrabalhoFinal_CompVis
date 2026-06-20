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
// Projectiles and particles
#include "projectiles.h"
#include "particles.h"

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
                else if (material_name.find("paredes") != std::string::npos)
                    current_selector = 10.0f; // Sinaliza que deve usar Concreto
                else if (material_name.find("Material.001") != std::string::npos || material_name.find("wood") != std::string::npos)
                    current_selector = 9.0f;  // Sinaliza que deve usar Madeira
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

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
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
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
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
void UpdatePosition(bool can_move);
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
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
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

Enemy g_enemies[MAX_ENEMIES] = {
    Enemy(2.0f, 2.0f, -2.0f, 0.0f, 0.5f, true, 1.0f, 0.99f, 0.775f)
};


#include "projectiles.h"

MapItem map[MAX_PLATFORMS];


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

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/red_brick_diff_1k.jpg");      // TextureImage0
    LoadTextureImage("../../data/rocky_terrain_02_diff_1k.jpg"); // TextureImage1
    LoadTextureImage("../../data/bcck1.png"); // TextureImage2
    LoadTextureImage("../../data/bcck2.png"); // TextureImage3
    LoadTextureImage("../../data/TNT/TNT.png"); // TextureImage4
    LoadTextureImage("../../data/map_ground/A5_WoodTextureSeamless.png"); // TextureImage9
    LoadTextureImage("../../data/map_ground/concreto.jpeg"); // TextureImage10

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel bunnymodel("../../data/bunny.obj");
    ComputeNormals(&bunnymodel);
    BuildTrianglesAndAddToVirtualScene(&bunnymodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel bigchillmodel("../../data/big_chill_cloaked.obj");
    ComputeNormals(&bigchillmodel);
    BuildTrianglesAndAddToVirtualScene(&bigchillmodel);
    bigchillmodel.ComputeBoundingBox();

    ObjModel blockmodel("../../data/TNT/TNT.obj");
    ComputeNormals(&blockmodel);
    BuildTrianglesAndAddToVirtualScene(&blockmodel);
    blockmodel.ComputeBoundingBox();
    // for (int i = 0; i < MAX_PLATFORMS; i++) {
    //     map[i].scale = glm::vec3(0.1f, 0.1f, 0.1f);
    //     glm::vec3 pos = {i * 4.0f + 2.0f, -1.0f, i * 2.0f}; // Example positions for platforms
    //     map[i].bbox = AABB(pos, blockmodel.aabb.min * map[i].scale, blockmodel.aabb.max * map[i].scale);
    //     map[i].position = pos;
    //     // printf("Platform %d -> Position: (%.2f, %.2f, %.2f), Scale: (%.2f, %.2f, %.2f)\n", 
    //     //     i, map[i].position.x, map[i].position.y, map[i].position.z,
    //     //     map[i].scale.x, map[i].scale.y, map[i].scale.z);
    //     // printf("platform min: (%.2f, %.2f, %.2f), max: (%.2f, %.2f, %.2f)\n", 
    //     //     map[i].bbox.min.x, map[i].bbox.min.y, map[i].bbox.min.z,
    //     //     map[i].bbox.max.x, map[i].bbox.max.y, map[i].bbox.max.z);
    // }


    // Load map model
    ObjModel ground_model("../../data/map_ground/chao_mapa.obj");
    ComputeNormals(&ground_model);
    BuildTrianglesAndAddToVirtualScene(&ground_model);

    int current_platform_index = 0;                                                                                                                                                        
                                                                                                                                                                                           
    // Loop through all shapes in the OBJ                                                                                                                                                  
    for (size_t i = 0; i < ground_model.shapes.size(); ++i) {                                                                                                                                 
        std::string shape_name = ground_model.shapes[i].name;                                                                                                                                 
                                                                                                                                                                                           
        // Check if this shape is meant to be a collider                                                                                                                                   
        if (shape_name.find("Collider") != std::string::npos) {                                                                                                                            
            if (current_platform_index < MAX_PLATFORMS) {                                                                                                                                  
                // Add the shape's AABB to the collision map                                                                                                                               
                map[current_platform_index].bbox = ground_model.ComputeBoundingBoxForShape(i);                                                                                                
                current_platform_index++;                                                                                                                                                  
            } else {                                                                                                                                                                       
                printf("WARNING: Too many colliders! Increase MAX_PLATFORMS.\n");                                                                                                          
            }                                                                                                                                                                              
        }                                                                                                                                                                                  
    }  

    ground_model.ComputeBoundingBox();


    // Load swampfire glTF and build GPU resources; loader prints diagnostics
    tinygltf::Model gltfmodel = loadGltfModelAndBuildScene("../../data/swampfire__ben_10_alien_force/scene.gltf", "the_swampfire");
    tinygltf::Model bentennyson_model = loadGltfModelAndBuildScene("../../data/ben_tennyson.glb", "the_bentennyson");
    tinygltf::Model foreverknight_model = loadGltfModelAndBuildScene("../../data/forever_knight.glb", "the_foreverknight");
    tinygltf::Model castle_model = loadGltfModelAndBuildScene("../../data/castelin/scene.gltf", "the_castle");
    // We no longer use a GLTF fireball; projectiles will use the static `the_sphere` mesh from OBJ imports.
    tinygltf::Model emptyModel; // placeholder when no GLTF is used for projectiles
    
    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    float anterior = (float)glfwGetTime();

    // Variáveis de estado da máquina de animação
    bool is_attacking = false;

    // Swampfire animation local state (preserves timers and flags)
    SwampfireAnimState swampfire_state;
    BenAnimState ben_state;

    int current_enemy_anim = 36;
    bool t_key_was_down = false;

    GltfAnimator swampfireAnimator(gltfmodel);
    // Animator placeholder for projectiles (no GLTF for projectiles)
    GltfAnimator fireballAnimator(emptyModel);
    GltfAnimator bentennysonAnimator(bentennyson_model);
    GltfAnimator foreverknightAnimator(foreverknight_model);
    // keep swampfire_state alive for the main loop (defined above)

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        if (keys[GLFW_KEY_ESCAPE])
            glfwSetWindowShouldClose(window, GL_TRUE);

        float agora = (float)glfwGetTime();
        delta_t = agora - anterior;
        anterior = agora;

        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(0.9f, 0.9f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        float height_offset = 1.5f;

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::vec4 camera_lookat_l    = glm::vec4(player.position.x, player.position.y + height_offset, player.position.z, 1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        glm::vec4 camera_position_c  = camera_lookat_l + glm::vec4(x, y + 0.5, z, 0.0f); // Ponto "c", centro da câmera
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -200.0f; // Posição do "far plane"

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

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

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
        #define AXES_DEBUG 100
        #define BBOX_DEBUG 101
        #define GROUND 12

        
        // O personagem só pode se mover se não estiver no meio de um ataque, morto ou sofrendo flinch
        bool can_move = !is_attacking && !player.is_dead && !player.is_flinching;
        UpdatePosition(can_move);

        UpdateEnemies();
        ProcessEnemyMeleeHitboxes();

        // Re-bind all previously loaded textures/samplers to their texture units
        for (GLuint tu = 0; tu < g_NumLoadedTextures; ++tu)
        {
            glActiveTexture(GL_TEXTURE0 + tu);
            glBindTexture(GL_TEXTURE_2D, g_LoadedTextureIDs[tu]);
            glBindSampler(tu, g_LoadedSamplerIDs[tu]);
        }

        // Draw controlled BigChill if visible
        if (player.active_character == 0)
        {
            model = Matrix_Translate(player.position.x, player.position.y, player.position.z)
                    * Matrix_Scale(player.characters[0].scale, player.characters[0].scale, player.characters[0].scale)
                    * Matrix_Rotate_Y(player.rotate);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, CHILL);
            // Draw axes in BigChill model space (origin-centered debug)
            if (g_AxesVAO != 0) {
                glUniform1i(g_object_id_uniform, AXES_DEBUG);
                glBindVertexArray(g_AxesVAO);
                glLineWidth(2.0f);
                glDrawArrays(GL_LINES, 0, 6);
                glBindVertexArray(0);
                glUniform1i(g_object_id_uniform, CHILL);
            }
            glDisable(GL_CULL_FACE); // Manto precisa dupla-face para não "sumir" por dentro.
            DrawVirtualObject("the_bigchill");
            DrawBoundingBox(player.characters[0].bbox, CHILL);
            glEnable(GL_CULL_FACE);
        }

        // Compute swampfire animation via modular function (keeps local state in swampfire_state)
        SwampfireAnimResult animRes = computeSwampfireAnimation(gltfmodel, keys, player.jumping, delta_t, agora, swampfire_state);
        
        BenAnimResult benRes = computeBenAnimation(bentennyson_model, keys, player.jumping, delta_t, agora, ben_state);


        // Draw Swampfire instances if visible
        if (player.active_character == 1)
        {
            int current_anim_index = animRes.current_anim_index;
            is_attacking = animRes.is_attacking;
            ProcessMeleeHitboxes(animRes, swampfire_state, SWAMPFIRE);
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
                    // ----- CORREÇÃO DE DESNÍVEL DO IDLE -----
                    float anim_y_offset = 0.0f;
                    // Se for a animação Idle (6), aplicamos a compensação.
                    if (current_anim_index == 6) {
                        anim_y_offset = 0.085f; 
                    }

                    // A matriz model agora soma o offset no eixo Y
                    model = Matrix_Translate(player.position.x, player.position.y + anim_y_offset, player.position.z)
                          * Matrix_Scale(player.characters[1].scale, player.characters[1].scale, player.characters[1].scale)
                          * Matrix_Rotate_Y(player.rotate - (3.14159265f / 6))
                          * Matrix_Rotate_X(0.175f);
                    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, g_VirtualScene[name].texture_id);
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
                    glUniform1i(g_object_id_uniform, SWAMPFIRE);
                    // Draw axes in model space (debug)
                    if (g_AxesVAO != 0) {
                        glUniform1i(g_object_id_uniform, AXES_DEBUG); // axes id -> handled in fragment shader
                        glBindVertexArray(g_AxesVAO);
                        glLineWidth(2.0f);
                        glDrawArrays(GL_LINES, 0, 6);
                        glBindVertexArray(0);
                        glUniform1i(g_object_id_uniform, SWAMPFIRE);
                    }
                    DrawVirtualObject(name.c_str());
                    DrawBoundingBox(player.characters[1].bbox, SWAMPFIRE);
                }
            }
        }

        // Draw Ben Tennyson instances if visible
        if (player.active_character == 2)
        {
            is_attacking = benRes.is_attacking;
            ProcessBenMeleeHitboxes(benRes, ben_state, BENTENNYSON);
            float ben_y_offset = player.is_flinching ? -0.110f : 0.0f;
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


        // Desenhar o inimigo
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].visible) continue;

            // Se estiver piscando (morto), ignorar o draw em frames alternados
            if (g_enemies[i].is_flashing && fmod(agora, 0.2f) < 0.1f) {
                continue;
            }

            // Calcula a rotação LÓGICA (exata para o jogador) que será usada pela Hitbox de ataque.
            if (!g_enemies[i].is_dead) {
                float dx = player.position.x - g_enemies[i].position.x;
                float dz = player.position.z - g_enemies[i].position.z;
                g_enemies[i].rotate = atan2(dx, dz);
            }

            // Determina a animação baseada no estado do inimigo
            int current_enemy_anim = 0; // 0 = Idle
            float anim_time = agora;

            if (g_enemies[i].is_dead) {
                if (g_enemies[i].death_timer < g_enemies[i].death_anim_duration) {
                    current_enemy_anim = 16; // Fall
                    anim_time = std::min(g_enemies[i].death_timer, 2.65f);
                } else {
                    current_enemy_anim = 17; // Stand / Faint
                    anim_time = 0.0f; // Pausa no primeiro frame
                }
            } else if (g_enemies[i].is_flinching) {
                current_enemy_anim = g_enemies[i].flinch_anim; // 16 or 17
                anim_time = g_enemies[i].flinch_timer;
            } else if (g_enemies[i].is_attacking) {
                current_enemy_anim = 21; // 21 = Taunt
                anim_time = g_enemies[i].attack_timer; // Toca do começo
            } else {
                float dist_to_player = glm::distance(
                    glm::vec3(g_enemies[i].position.x, 0.0f, g_enemies[i].position.z),
                    glm::vec3(player.position.x, 0.0f, player.position.z));
                if (dist_to_player > g_enemies[i].attack_range) {
                    current_enemy_anim = 34; // ForeverKnight_Run
                    anim_time = agora * 2.0f; // Speed up run animation
                }
            }

            // Atualiza o animador para este inimigo específico
            bool loop_anim = !(g_enemies[i].is_dead || g_enemies[i].is_flinching);
            foreverknightAnimator.update(foreverknight_model, current_enemy_anim, anim_time, loop_anim);

            // Aplica offset Y se estiver correndo (anim 34)
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

            for (const auto& pair : g_VirtualScene) {
                if (pair.first.find("the_foreverknight_") == 0) {
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_2D, pair.second.texture_id);
                    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
                    
                    // Desabilitar culling para esse modelo para corrigir faces viradas do avesso
                    glDisable(GL_CULL_FACE);
                    DrawVirtualObject(pair.first.c_str());
                    glEnable(GL_CULL_FACE);
                }
            }

            DrawBoundingBox(g_enemies[i].bbox, BUNNY);
        }


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
    
        // 4. Desenhar o Objeto Visual
        // Mude a string abaixo EXATAMENTE para o nome do objeto (mesh) salvo dentro do seu arquivo .obj
        glDisable(GL_CULL_FACE); // Desabilita culling para o chão, que é duplo-face
        DrawVirtualObject("Plane"); 
        glEnable(GL_CULL_FACE); // Reabilita culling para os próximos objetos
        // ==== DESENHAR OS COLLIDERS (MUITO ÚTIL PARA DEBUG) ====
        for (int i = 0; i < MAX_PLATFORMS; i++) {
            // Desenha a caixa delimitadora usando a função existente
            // O segundo parâmetro (GROUND) diz para a GPU voltar para o ID do chão após desenhar a linha
            DrawBoundingBox(map[i].bbox, GROUND);
        }


        // // Desenhamos o plano do chão
        // model = Matrix_Translate(0.0f, -1.0f, 0.0f)
        //         * Matrix_Scale(20.0f, 1.0f, 20.0f);
        // glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        // glUniform1i(g_object_id_uniform, PLANE);
        // DrawVirtualObject("the_plane");

        // Desenhamos o Castelo
        // Scaled down by 0.01 so it's realistically sized, and placed within view at Z = -15
        // model = Matrix_Translate(0.0f, -1.0f, -15.0f) * Matrix_Scale(0.01f, 0.01f, 0.01f);
        // glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        // glUniform1i(g_object_id_uniform, CASTLE);
        // for (const auto& pair : g_VirtualScene) {
        //     if (pair.first.find("the_castle_") == 0) {
        //         glActiveTexture(GL_TEXTURE8);
        //         glBindTexture(GL_TEXTURE_2D, pair.second.texture_id);
        //         glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
                
        //         // Desabilitar culling para garantir que o castelo seja visível por dentro e por fora
        //         glDisable(GL_CULL_FACE);
        //         DrawVirtualObject(pair.first.c_str());
        //         glEnable(GL_CULL_FACE);
        //     }
        // }


        // // Desenhamos os blocos do mapa
        // for (int i = 0; i < MAX_PLATFORMS; i++) {
        //     model = Matrix_Translate(map[i].position.x, map[i].position.y, map[i].position.z)
        //           * Matrix_Scale(map[i].scale.x, map[i].scale.y, map[i].scale.z);
        //     glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        //     glUniform1i(g_object_id_uniform, BLOCO);
        //     DrawVirtualObject("TNT");
        //     DrawBoundingBox(map[i].bbox, BLOCO);
        // }


        // Draw particles (after opaque geometry)
        Particles_Draw(g_VirtualScene, g_GpuProgramID, g_model_uniform, g_object_id_uniform, 1.0f);
        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // Draw Health
        char health_buf[64];
        snprintf(health_buf, sizeof(health_buf), "Health: %.0f", player.health);
        TextRendering_PrintString(window, health_buf, -0.9f, -0.9f, 2.0f);

        // Respawn Logic & Death Message
        if (player.is_dead) {
            TextRendering_PrintString(window, "Voce morreu!", -0.2f, 0.0f, 3.0f);
            player.death_timer += delta_t;
            if (player.death_timer >= 3.0f) {
                player.position = glm::vec3(0.0f, -1.0f, 0.0f);
                player.health = player.max_health;
                player.is_dead = false;
                player.death_timer = 0.0f;

                for (int i = 0; i < MAX_ENEMIES; i++) {
                    g_enemies[i].visible = false;
                }
                SpawnEnemy(glm::vec3(5.0f, 2.0f, -5.0f));
            }
        } else if (player.is_flinching) {
            player.flinch_timer += delta_t;
            if (player.flinch_timer >= 0.5f) {
                player.is_flinching = false;
            }
        }

        // Imprimimos a vida dos inimigos em cima das suas cabeças
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].visible || g_enemies[i].is_dead) continue;
            glm::vec4 enemy_pos_world = glm::vec4(g_enemies[i].position.x, g_enemies[i].position.y + 1.2f, g_enemies[i].position.z, 1.0f);
            glm::vec4 enemy_pos_ndc = projection * view * enemy_pos_world;
            if (enemy_pos_ndc.w > 0.0f) {
                enemy_pos_ndc /= enemy_pos_ndc.w;
                // Só desenha se estiver na frente da câmera (z NDC entre -1 e 1)
                if (enemy_pos_ndc.z >= -1.0f && enemy_pos_ndc.z <= 1.0f) {
                    char hp_buf[32];
                    snprintf(hp_buf, sizeof(hp_buf), "%.0f", g_enemies[i].health);
                    // Desloca um pouco o X para centralizar melhor
                    TextRendering_PrintString(window, hp_buf, enemy_pos_ndc.x - 0.05f, enemy_pos_ndc.y, 1.5f);
                }
            }
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

    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

void DrawBoundingBox(AABB& aabb, int restore_object_id) {
    if (g_BBoxVAO == 0) return;

    glm::mat4 identity = Matrix_Identity();
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(identity));

    const GLfloat bbox_vertices[8 * 4] = {
        aabb.min.x, aabb.min.y, aabb.min.z, 1.0f, // V0
        aabb.max.x, aabb.min.y, aabb.min.z, 1.0f, // V1
        aabb.max.x, aabb.max.y, aabb.min.z, 1.0f, // V2
        aabb.min.x, aabb.max.y, aabb.min.z, 1.0f, // V3
        aabb.min.x, aabb.min.y, aabb.max.z, 1.0f, // V4
        aabb.max.x, aabb.min.y, aabb.max.z, 1.0f, // V5
        aabb.max.x, aabb.max.y, aabb.max.z, 1.0f, // V6
        aabb.min.x, aabb.max.y, aabb.max.z, 1.0f  // V7
    };

    glBindVertexArray(g_BBoxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_BBoxVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bbox_vertices), bbox_vertices);

    glUniform1i(g_object_id_uniform, 101);
    glLineWidth(2.0f);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glUniform1i(g_object_id_uniform, restore_object_id);
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
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
    glUseProgram(0);
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
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        if (mod & GLFW_MOD_SHIFT)
            g_AngleZ -= delta;
        else if (mod == 0 && !(mod & GLFW_MOD_SHIFT) && !player.is_dead) {
            // Swap active character
            float old_max = player.max_health;
            player.active_character = (player.active_character + 1) % 3;
            float new_max = player.active_character == 0 ? 200.0f : (player.active_character == 1 ? 250.0f : 100.0f);
            player.health = player.health * (new_max / old_max);
            player.max_health = new_max;
            // Sync position to current player position
            glm::vec3 size = player.active_character == 0 ? bigchill_size : (player.active_character == 1 ? swampfire_size : bentennyson_size);
            printf("Switched to character %d\n", player.active_character);
            player.characters[player.active_character].bbox = makeAABBFromGround(player.position, size);
            ResolvePlayerMapCollisions();

            for (int i = 0; i < 3; ++i)
                // g_characters[g_active_character].pos[i] = player_pos[i];
                    // Spawn green transform particles at player position (use ParticleOptions)
                    {
                        ParticleOptions popts;
                        popts.color = HexToRgb("#06b800");
                        popts.life = 0.25f + 0.15f * 1.0f;
                        popts.scale = 0.15f + 0.01f * 6.0f;
                        popts.speed = 0.1f + 0.8f * 3.0f;
                        popts.count = std::max(2, (int)std::round(8.0f * 6.0f));
                        Particles_Spawn(glm::vec3(player.position.x, player.position.y, player.position.z), popts);
                    }
        }
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

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Position = Z(%.2f)*Y(%.2f)*X(%.2f)\n", player.position.z, player.position.y, player.position.x);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
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