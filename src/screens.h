#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Initialize the screen quad VAO/VBO
void InitScreens();

// Draw the full screen title
void DrawTitleScreen(GLint model_uniform, GLint view_uniform, GLint proj_uniform, GLint object_id_uniform);

// Draw the loading spinner
void DrawLoadingSpinner(GLint model_uniform, GLint view_uniform, GLint proj_uniform, GLint object_id_uniform, float current_time);
void DrawTextWindowBox(GLint model_uniform, GLint view_uniform, GLint proj_uniform, GLint object_id_uniform);
