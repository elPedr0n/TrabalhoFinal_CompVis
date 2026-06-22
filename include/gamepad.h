#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <GLFW/glfw3.h>

void InitGamepadMappings();
void ProcessGamepadInput(GLFWwindow* window);
bool IsGamepadConnected();

#endif // GAMEPAD_H
