#include "ferris_wheel.h"
#include "sceneobject.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <string>
#include <vector>

#define FERRIS_WHEEL_ID 55

// --------------------------------------------------------------------------
// Helpers internos
// --------------------------------------------------------------------------

static glm::mat4 nodeLocalTransform(const tinygltf::Node& n)
{
    if (n.matrix.size() == 16)
        return glm::make_mat4(n.matrix.data());

    glm::mat4 T(1.0f), R(1.0f), S(1.0f);

    if (n.translation.size() == 3)
        T = glm::translate(glm::mat4(1.0f),
            glm::vec3((float)n.translation[0], (float)n.translation[1], (float)n.translation[2]));

    if (n.rotation.size() == 4) {
        glm::quat q((float)n.rotation[3], (float)n.rotation[0],
                    (float)n.rotation[1], (float)n.rotation[2]);
        R = glm::toMat4(q);
    }

    if (n.scale.size() == 3)
        S = glm::scale(glm::mat4(1.0f),
            glm::vec3((float)n.scale[0], (float)n.scale[1], (float)n.scale[2]));

    return T * R * S;
}

// Acumula transforms desde a raiz até o nó ni (inclusive)
static glm::mat4 nodeWorldTransform(const tinygltf::Model& model,
                                    const std::vector<int>& parent,
                                    int ni)
{
    glm::mat4 m = nodeLocalTransform(model.nodes[ni]);
    int p = parent[ni];
    while (p >= 0) {
        m = nodeLocalTransform(model.nodes[p]) * m;
        p = parent[p];
    }
    return m;
}

// Acumula transforms desde a raiz até o nó ni (EXCLUSIVE — não inclui ni)
static glm::mat4 nodeWorldTransformExclusive(const tinygltf::Model& model,
                                              const std::vector<int>& parent,
                                              int ni)
{
    std::vector<int> chain;
    int p = parent[ni];
    while (p >= 0) { chain.push_back(p); p = parent[p]; }

    glm::mat4 m(1.0f);
    for (int i = (int)chain.size() - 1; i >= 0; i--)
        m = m * nodeLocalTransform(model.nodes[chain[i]]);
    return m;
}

glm::vec3 get_mesh_center(const tinygltf::Model& model, int mesh_idx) {
    if (mesh_idx < 0 || mesh_idx >= (int)model.meshes.size()) return glm::vec3(0.0f);
    const auto& mesh = model.meshes[mesh_idx];
    if (mesh.primitives.empty()) return glm::vec3(0.0f);
    const auto& primitive = mesh.primitives[0];
    auto it = primitive.attributes.find("POSITION");
    if (it == primitive.attributes.end()) return glm::vec3(0.0f);
    const tinygltf::Accessor& accessor = model.accessors[it->second];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
    float min_x = 1e9, max_x = -1e9, min_y = 1e9, max_y = -1e9, min_z = 1e9, max_z = -1e9;
    for (size_t i = 0; i < accessor.count; i++) {
        float x = positions[i*3 + 0]; float y = positions[i*3 + 1]; float z = positions[i*3 + 2];
        if (x < min_x) min_x = x; if (x > max_x) max_x = x;
        if (y < min_y) min_y = y; if (y > max_y) max_y = y;
        if (z < min_z) min_z = z; if (z > max_z) max_z = z;
    }
    return glm::vec3((min_x+max_x)/2.0f, (min_y+max_y)/2.0f, (min_z+max_z)/2.0f);
}

// --------------------------------------------------------------------------
// DrawFerrisWheel
// --------------------------------------------------------------------------
void DrawFerrisWheel(
    const tinygltf::Model& model,
    const std::string&     base_name,
    GLuint                 prog,
    GLint                  model_uniform,
    GLint                  obj_id_uniform,
    const glm::mat4&       world,
    float                  time,
    float                  spin_speed)
{
    if (model.nodes.empty()) return;

    // --- 1. Mapa de pais ---
    std::vector<int> parent(model.nodes.size(), -1);
    for (int i = 0; i < (int)model.nodes.size(); i++)
        for (int child : model.nodes[i].children)
            parent[child] = i;

    // --- 2. Localizar nós chave ---
    int wheel_node_idx  = -1;
    int cabin_node_idx  = -1;
    for (int i = 0; i < (int)model.nodes.size(); i++) {
        if (model.nodes[i].name == "wheel")  wheel_node_idx  = i;
        if (model.nodes[i].name == "cabin")  cabin_node_idx  = i;
    }

    // --- 3. Centro de rotação da roda (translação do nó wheel em espaço RootNode) ---
    // Usamos apenas a translação, ignorando a rotação base do nó wheel,
    // para que o spin aconteça em torno do centro geométrico.
    glm::vec3 wheel_hub(0.0f);
    if (wheel_node_idx >= 0 && model.nodes[wheel_node_idx].translation.size() == 3) {
        wheel_hub = glm::vec3(
            (float)model.nodes[wheel_node_idx].translation[0],
            (float)model.nodes[wheel_node_idx].translation[1],
            (float)model.nodes[wheel_node_idx].translation[2]
        );
    }

    // --- 4. Matrizes de spin ---
    glm::mat4 spin_z = glm::rotate(glm::mat4(1.0f), time * spin_speed, glm::vec3(0.0f, 0.0f, 1.0f));

    // --- 5. Shader ---
    glUniform1i(obj_id_uniform, FERRIS_WHEEL_ID);
    glDisable(GL_CULL_FACE);

    // --- 6. Renderizar cada nó com malha ---
    for (int ni = 0; ni < (int)model.nodes.size(); ni++) {
        int mesh_idx = model.nodes[ni].mesh;
        if (mesh_idx < 0) continue;

        std::string obj_name = base_name + "_" + std::to_string(mesh_idx);
        if (g_VirtualScene.find(obj_name) == g_VirtualScene.end()) continue;

        const std::string& node_name = model.nodes[ni].name;
        glm::mat4 final_model;

        // ----------------------------------------------------------------
        // Caso A: Roda estrutural (mesh 16 — wheel_lambert1_0)
        // Gira em torno do seu próprio centro (wheel_hub).
        // Fórmula: world * root_correction * T(hub) * spin * R_base * mesh_local
        // onde root_correction = transforms acima do RootNode (Sketchfab + fbx)
        // ----------------------------------------------------------------
        if (node_name == "wheel_lambert1_0") {
            // root_correction = transforms acumulados acima do nó wheel (excluindo o próprio wheel)
            // = Sketchfab * fbx * Object_2 * RootNode
            glm::mat4 root_correction = nodeWorldTransformExclusive(model, parent, wheel_node_idx);

            // Translação do hub e rotação base do nó wheel (R já embutida no nodeLocal sem T)
            glm::mat4 to_hub   = glm::translate(glm::mat4(1.0f),  wheel_hub);

            // Extrai só a rotação base do wheel (sem translação)
            glm::mat4 wheel_rot_only(1.0f);
            if (model.nodes[wheel_node_idx].rotation.size() == 4) {
                const auto& r = model.nodes[wheel_node_idx].rotation;
                glm::quat q((float)r[3], (float)r[0], (float)r[1], (float)r[2]);
                wheel_rot_only = glm::toMat4(q);
            }

            // mesh_local do nó wheel_lambert1_0 (sem transform = identidade)
            glm::mat4 mesh_local = nodeLocalTransform(model.nodes[ni]);

            final_model = world * root_correction * to_hub * wheel_rot_only * spin_z * mesh_local;
        }
        // ----------------------------------------------------------------
        // Caso B: Cabines (filhas do nó "cabin")
        // A POSIÇÃO orbita a roda, mas a ORIENTAÇÃO permanece reta.
        // Só rotacionamos o vetor de offset (translação do polySurface pai),
        // sem aplicar qualquer rotação à cabine em si.
        // ----------------------------------------------------------------
        else {
            bool is_cabin = false;
            if (cabin_node_idx >= 0) {
                // Verifica se cabin_node_idx é ancestral de ni
                int p = parent[ni];
                while (p >= 0) {
                    if (p == cabin_node_idx) { is_cabin = true; break; }
                    p = parent[p];
                }
            }

            if (is_cabin) {
                // polySurface_node = pai direto do mesh node = quem tem a translação de órbita
                int poly_node = parent[ni];

                // A posição original ABSOLUTA do centro visual da cabine
                glm::vec3 orbit_offset(0.0f);
                const auto& pn = model.nodes[poly_node];
                if (pn.translation.size() == 3) {
                    orbit_offset = glm::vec3((float)pn.translation[0],
                                             (float)pn.translation[1],
                                             (float)pn.translation[2]);
                }
                
                glm::vec3 mesh_center = get_mesh_center(model, model.nodes[ni].mesh);
                glm::vec3 hinge_orig = orbit_offset + mesh_center;

                // Rotacionar a dobradiça (hinge) ao redor do centro geométrico da roda (wheel_hub)
                glm::vec3 rotated_hinge = wheel_hub + glm::vec3(spin_z * glm::vec4(hinge_orig - wheel_hub, 1.0f));

                // O deslocamento que precisa ser aplicado à malha inteira para que a dobradiça vá para rotated_hinge
                // Delta = rotated_hinge - hinge_orig
                glm::vec3 delta = rotated_hinge - hinge_orig;

                // root_correction acima do grupo cabin (Sketchfab * fbx * ... * cabin_parent)
                glm::mat4 above_cabin = nodeWorldTransformExclusive(model, parent, cabin_node_idx);

                // A malha se move por Delta, mantendo a orientação inalterada (rotação original da modelagem)
                glm::mat4 mesh_local = nodeLocalTransform(model.nodes[ni]);
                final_model = world * above_cabin
                              * glm::translate(glm::mat4(1.0f), delta)
                              * glm::translate(glm::mat4(1.0f), orbit_offset)
                              * mesh_local;
            }
            // ----------------------------------------------------------------
            // Caso C: Estáticos (mount, block, stairs, fence, trash, plane)
            // ----------------------------------------------------------------
            else {
                glm::mat4 node_transform = nodeWorldTransform(model, parent, ni);
                final_model = world * node_transform;
            }
        }

        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(final_model));

        const SceneObject& obj = g_VirtualScene.at(obj_name);
        if (obj.texture_id != 0) {
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, obj.texture_id);
            glUniform1i(glGetUniformLocation(prog, "TextureImage8"), 8);
        }

        glBindVertexArray(obj.vertex_array_object_id);
        glDrawElements(obj.rendering_mode,
                       (GLsizei)obj.num_indices,
                       GL_UNSIGNED_INT,
                       (void*)(obj.first_index * sizeof(GLuint)));
        glBindVertexArray(0);
    }

    glEnable(GL_CULL_FACE);
}

// --------------------------------------------------------------------------
// DrawHierarchicalGLTF
// --------------------------------------------------------------------------
void DrawHierarchicalGLTF(
    const tinygltf::Model& model,
    const std::string&     base_name,
    GLuint                 prog,
    GLint                  model_uniform,
    GLint                  obj_id_uniform,
    int                    obj_id,
    const glm::mat4&       world)
{
    if (model.nodes.empty()) return;

    std::vector<int> parent(model.nodes.size(), -1);
    for (int i = 0; i < (int)model.nodes.size(); i++)
        for (int child : model.nodes[i].children)
            parent[child] = i;

    glUniform1i(obj_id_uniform, obj_id);
    glDisable(GL_CULL_FACE);

    for (int ni = 0; ni < (int)model.nodes.size(); ni++) {
        int mesh_idx = model.nodes[ni].mesh;
        if (mesh_idx < 0) continue;

        std::string obj_name = base_name + "_" + std::to_string(mesh_idx);
        if (g_VirtualScene.find(obj_name) == g_VirtualScene.end()) continue;

        glm::mat4 node_transform = nodeWorldTransform(model, parent, ni);
        glm::mat4 final_model = world * node_transform;

        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(final_model));

        const SceneObject& obj = g_VirtualScene.at(obj_name);
        if (obj.texture_id != 0) {
            glActiveTexture(GL_TEXTURE8);
            glBindTexture(GL_TEXTURE_2D, obj.texture_id);
            glUniform1i(glGetUniformLocation(prog, "TextureImage8"), 8);
        }

        glBindVertexArray(obj.vertex_array_object_id);
        glDrawElements(obj.rendering_mode,
                       (GLsizei)obj.num_indices,
                       GL_UNSIGNED_INT,
                       (void*)(obj.first_index * sizeof(GLuint)));
        glBindVertexArray(0);
    }

    glEnable(GL_CULL_FACE);
}
