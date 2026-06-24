#include "screens.h"
#include "matrices.h"

static GLuint g_ScreenVAO = 0;
static GLuint g_ScreenVBO = 0;

void InitScreens() {
    if (g_ScreenVAO != 0) return;

    // 2D Quad for screen rendering
    // x, y, u, v
    float quadVertices[] = {
        // Positions   // TexCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &g_ScreenVAO);
    glGenBuffers(1, &g_ScreenVBO);
    glBindVertexArray(g_ScreenVAO);

    glBindBuffer(GL_ARRAY_BUFFER, g_ScreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    // vertex positions (location = 0)
    glEnableVertexAttribArray(0);
    // Note: the vertex shader expects vec4 for model_coefficients
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // texture coords (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void DrawTitleScreen(GLint model_uniform, GLint view_uniform, GLint proj_uniform, GLint object_id_uniform) {
    if (g_ScreenVAO == 0) InitScreens();

    glm::mat4 identity = glm::mat4(1.0f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(identity));
    glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(identity));
    glUniformMatrix4fv(proj_uniform, 1, GL_FALSE, glm::value_ptr(identity));
    glUniform1i(object_id_uniform, 30); // TITLE_SCREEN

    glBindVertexArray(g_ScreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void DrawLoadingSpinner(GLint model_uniform, GLint view_uniform, GLint proj_uniform, GLint object_id_uniform, float current_time) {
    if (g_ScreenVAO == 0) InitScreens();

    glm::mat4 identity = glm::mat4(1.0f);
    glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(identity));
    glUniformMatrix4fv(proj_uniform, 1, GL_FALSE, glm::value_ptr(identity));

    // Position at bottom right corner, scale down, and rotate
    glm::mat4 model = glm::mat4(1.0f);
    model = model * Matrix_Translate(0.8f, -0.8f, 0.0f);
    
    // Adjust aspect ratio if needed, but since it's an icon, we'll keep it square-ish.
    // Screen is 800x600 (ratio 4/3) so scale X by 3/4 to keep it square.
    model = model * Matrix_Scale(0.15f * 0.75f, 0.15f, 1.0f); 
    
    model = model * Matrix_Rotate_Z(-current_time * 5.0f);

    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(object_id_uniform, 31); // LOADING_SPINNER

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(g_ScreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void DrawTextWindowBox(GLint model_uniform, GLint view_uniform, GLint proj_uniform, GLint object_id_uniform) {
    if (g_ScreenVAO == 0) InitScreens();

    glm::mat4 identity = glm::mat4(1.0f);
    glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(identity));
    glUniformMatrix4fv(proj_uniform, 1, GL_FALSE, glm::value_ptr(identity));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // We want the window on top of everything, disable depth test
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(g_ScreenVAO);

    // Draw border (slightly larger, opaque)
    glm::mat4 border_model = Matrix_Scale(0.55f, 0.40f, 1.0f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(border_model));
    glUniform1i(object_id_uniform, 41); // UI_WINDOW_BORDER
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw background (semi-transparent)
    glm::mat4 bg_model = Matrix_Scale(0.54f, 0.39f, 1.0f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(bg_model));
    glUniform1i(object_id_uniform, 40); // UI_WINDOW_BG
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
