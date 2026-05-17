#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <string>
#include <map>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct SceneObject {
    std::string  name;
    size_t       first_index;
    size_t       num_indices;
    GLenum       rendering_mode;
    GLuint       vertex_array_object_id;
    glm::vec3    bbox_min;
    glm::vec3    bbox_max;
    GLuint       texture_id = 0; // OpenGL texture ID
    bool         hidden = false; // if true, skip drawing this object
};

extern std::map<std::string, SceneObject> g_VirtualScene;

#endif // SCENEOBJECT_H