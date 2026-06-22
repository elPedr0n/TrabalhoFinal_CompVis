#include "gamepad.h"
#include "globals.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

extern void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
extern float g_CameraTheta;
extern float g_CameraPhi;

void InitGamepadMappings() {
    std::ifstream file("../../data/gamecontrollerdb.txt");
    if (!file.is_open()) {
        std::cerr << "[Gamepad] Aviso: Nao foi possivel abrir ../../data/gamecontrollerdb.txt" << std::endl;
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string mappings = buffer.str();
    
    if (glfwUpdateGamepadMappings(mappings.c_str()) == GLFW_FALSE) {
        std::cerr << "[Gamepad] Erro ao carregar mapeamentos de gamepad do GLFW!" << std::endl;
    } else {
        std::cout << "[Gamepad] Base de dados de mapeamento (gamecontrollerdb.txt) carregada com sucesso." << std::endl;
    }
}


bool GetRawJoystickAsGamepadState(int jid, GLFWgamepadstate* state) {
    if (!glfwJoystickPresent(jid)) return false;

    int buttonCount;
    const unsigned char* buttons = glfwGetJoystickButtons(jid, &buttonCount);
    
    int axisCount;
    const float* axes = glfwGetJoystickAxes(jid, &axisCount);

    if (!buttons || !axes) return false;

    for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; i++) state->buttons[i] = GLFW_RELEASE;
    for (int i = 0; i <= GLFW_GAMEPAD_AXIS_LAST; i++) state->axes[i] = 0.0f;

    if (buttonCount >= 11) {
        state->buttons[GLFW_GAMEPAD_BUTTON_A] = buttons[0];
        state->buttons[GLFW_GAMEPAD_BUTTON_B] = buttons[1];
        state->buttons[GLFW_GAMEPAD_BUTTON_X] = buttons[2];
        state->buttons[GLFW_GAMEPAD_BUTTON_Y] = buttons[3];
        state->buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] = buttons[4];
        state->buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] = buttons[5];
        state->buttons[GLFW_GAMEPAD_BUTTON_BACK] = buttons[6];
        state->buttons[GLFW_GAMEPAD_BUTTON_START] = buttons[7];
        state->buttons[GLFW_GAMEPAD_BUTTON_GUIDE] = buttons[8];
        state->buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB] = buttons[9];
        state->buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB] = buttons[10];
    }
    
    if (buttonCount >= 15) {
        state->buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] = buttons[11];
        state->buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] = buttons[12];
        state->buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] = buttons[13];
        state->buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] = buttons[14];
    }

    if (axisCount >= 6) {
        state->axes[GLFW_GAMEPAD_AXIS_LEFT_X] = axes[0];
        state->axes[GLFW_GAMEPAD_AXIS_LEFT_Y] = axes[1];
        state->axes[GLFW_GAMEPAD_AXIS_RIGHT_X] = axes[3];
        state->axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] = axes[4];
        state->axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = axes[2];
        state->axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = axes[5];
    }

    if (axisCount >= 8) {
        if (axes[6] < -0.5f) state->buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] = GLFW_PRESS;
        if (axes[6] > 0.5f) state->buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] = GLFW_PRESS;
        if (axes[7] < -0.5f) state->buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] = GLFW_PRESS;
        if (axes[7] > 0.5f) state->buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] = GLFW_PRESS;
    }

    return true;
}

void ProcessGamepadInput(GLFWwindow* window) {
    GLFWgamepadstate state;
    static GLFWgamepadstate lastState;
    static bool first_frame = true;
    static int last_gamepad_id = -2;
    int gamepad_id = -1;
    int first_joystick = -1;
    bool is_raw = false;
    
    for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_16; i++) {
        if (glfwJoystickPresent(i)) {
            if (first_joystick == -1) first_joystick = i;
            if (glfwJoystickIsGamepad(i)) {
                gamepad_id = i;
                break;
            } else {
                static bool warned[16] = {false};
                if (!warned[i]) {
                    printf("[Gamepad DEBUG] Joystick no slot %d detectado, mas NAO possui mapeamento padrao de Gamepad. Nome: %s\n", i, glfwGetJoystickName(i));
                    warned[i] = true;
                }
            }
        }
    }

    if (gamepad_id == -1 && first_joystick != -1) {
        gamepad_id = first_joystick;
        is_raw = true;
    }

    if (gamepad_id != last_gamepad_id) {
        if (gamepad_id == -1) {
            printf("[Gamepad DEBUG] Nenhum controle detectado.\n");
        } else {
            if (is_raw) {
                printf("[Gamepad DEBUG] Utilizando fallback RAW para o controle no slot %d: %s\n", gamepad_id, glfwGetJoystickName(gamepad_id));
            } else {
                printf("[Gamepad DEBUG] Controle conectado e reconhecido no slot %d: %s\n", gamepad_id, glfwGetGamepadName(gamepad_id));
            }
        }
        last_gamepad_id = gamepad_id;
    }

    bool got_state = false;
    if (gamepad_id != -1) {
        if (is_raw) {
            got_state = GetRawJoystickAsGamepadState(gamepad_id, &state);
        } else {
            got_state = glfwGetGamepadState(gamepad_id, &state);
        }
    }

    if (got_state) {
        if (first_frame) {
            lastState = state;
            first_frame = false;
        }

        auto simulateKey = [&](int glfw_gamepad_btn, int glfw_key) {
            if (state.buttons[glfw_gamepad_btn] == GLFW_PRESS && lastState.buttons[glfw_gamepad_btn] == GLFW_RELEASE) {
                KeyCallback(window, glfw_key, 0, GLFW_PRESS, 0);
            } else if (state.buttons[glfw_gamepad_btn] == GLFW_RELEASE && lastState.buttons[glfw_gamepad_btn] == GLFW_PRESS) {
                KeyCallback(window, glfw_key, 0, GLFW_RELEASE, 0);
            }
        };

        simulateKey(GLFW_GAMEPAD_BUTTON_Y, GLFW_KEY_Q);
        simulateKey(GLFW_GAMEPAD_BUTTON_X, GLFW_KEY_E);
        simulateKey(GLFW_GAMEPAD_BUTTON_A, GLFW_KEY_SPACE);
        simulateKey(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, GLFW_KEY_Z);
        simulateKey(GLFW_GAMEPAD_BUTTON_START, GLFW_KEY_ENTER);
        simulateKey(GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, GLFW_KEY_G);
        simulateKey(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, GLFW_KEY_X); // Mapped alien swap to right d-pad

        bool triggers_pressed = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.0f || state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.0f;
        bool last_triggers_pressed = lastState.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.0f || lastState.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.0f;
        
        bool x_pressed = triggers_pressed;
        bool last_x_pressed = last_triggers_pressed;

        if (x_pressed && !last_x_pressed) {
            KeyCallback(window, GLFW_KEY_X, 0, GLFW_PRESS, 0);
        } else if (!x_pressed && last_x_pressed) {
            KeyCallback(window, GLFW_KEY_X, 0, GLFW_RELEASE, 0);
        }

        float lx = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
        float ly = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        float deadzone = 0.2f;

        auto simulateAnalogKey = [&](bool pressed, bool last_pressed, int glfw_key) {
            if (pressed && !last_pressed) {
                KeyCallback(window, glfw_key, 0, GLFW_PRESS, 0);
            } else if (!pressed && last_pressed) {
                KeyCallback(window, glfw_key, 0, GLFW_RELEASE, 0);
            }
        };

        simulateAnalogKey(ly < -deadzone, lastState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -deadzone, GLFW_KEY_W);
        simulateAnalogKey(ly > deadzone, lastState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > deadzone, GLFW_KEY_S);
        simulateAnalogKey(lx < -deadzone, lastState.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -deadzone, GLFW_KEY_A);
        simulateAnalogKey(lx > deadzone, lastState.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > deadzone, GLFW_KEY_D);

        if (!player.is_dead && !player.has_won) {
            float rx = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
            float ry = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
            if (fabs(rx) > deadzone) {
                g_CameraTheta -= 0.15f * rx;
            }
            if (fabs(ry) > deadzone) {
                g_CameraPhi += 0.15f * ry;
                float phimax = 3.141592f/2;
                float phimin = -phimax;
                if (g_CameraPhi > phimax) g_CameraPhi = phimax;
                if (g_CameraPhi < phimin) g_CameraPhi = phimin;
            }
        }

        lastState = state;
    }
}

bool IsGamepadConnected() {
    for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_16; i++) {
        if (glfwJoystickPresent(i)) {
            return true;
        }
    }
    return false;
}

