#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Model.h"

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

// Параметры камеры
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float cameraSpeed = 0.005f;
float mouseSensitivity = 0.05f;

// Переменные для аффинных преобразований частей станка
float offsetTableX = 0.0f;        // Смещение стола по X
float offsetCarriageY = 0.0f;     // Смещение каретки по Y
float offsetHeadZ = 0.0f;         // Смещение головы по Z

const float MAX_OFFSET = 0.6f;
const float MOVE_SPEED = 0.0005f;

// Координаты центров частей (из Blender скрипта)
glm::vec3 tableCenter = glm::vec3(0.0f, 0.15f, 0.425f);
glm::vec3 carriageCenter = glm::vec3(0.0f, -0.45f, 2.20f);
glm::vec3 headCenter = glm::vec3(0.0f, 0.05f, 2.55f);

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Lab7 - Affine Transformations", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    glEnable(GL_DEPTH_TEST);

    Model ourModel("models/cnc_machine.obj");
    Shader ourShader("vertex.glsl", "fragment.glsl");

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window)) {
        // Управление камерой
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraUp;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraUp;

        // Управление частями станка
        // Стол: Q/E - движение по X
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            offsetTableX = glm::max(-MAX_OFFSET, offsetTableX - MOVE_SPEED);
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            offsetTableX = glm::min(MAX_OFFSET, offsetTableX + MOVE_SPEED);
        }

        // Каретка: R/F - движение по Y
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            offsetCarriageY = glm::max(-MAX_OFFSET, offsetCarriageY - MOVE_SPEED);
        }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            offsetCarriageY = glm::min(MAX_OFFSET, offsetCarriageY + MOVE_SPEED);
        }

        // Голова: T/G - движение по Z
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
            offsetHeadZ = glm::max(-MAX_OFFSET, offsetHeadZ - MOVE_SPEED);
        }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            offsetHeadZ = glm::min(MAX_OFFSET, offsetHeadZ + MOVE_SPEED);
        }

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        ourShader.setMat4("projection", glm::value_ptr(projection));
        ourShader.setMat4("view", glm::value_ptr(view));
        ourShader.setVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);

        // Параметры освещения
        ourShader.setVec3("material.ambient", 0.2f, 0.1f, 0.0f);
        ourShader.setVec3("material.diffuse", 0.8f, 0.6f, 0.2f);
        ourShader.setVec3("material.specular", 0.9f, 0.9f, 0.9f);
        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setVec3("light.position", 0.0f, 2.0f, 3.0f);
        ourShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
        ourShader.setVec3("light.diffuse", 0.8f, 0.8f, 0.8f);
        ourShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // Построение матриц для каждой части
        // Предполагаем порядок мешей: Base(0), Table(1), Carriage(2), Head(3)
        vector<glm::mat4> transforms(4, glm::mat4(1.0f));

        // Масштаб для всей модели
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));

        // Mesh 0: Base (статичный)
        transforms[0] = scaleMatrix;

        // Mesh 1: Table (движется по X)
        // Сначала масштабируем, потом смещаем
        transforms[1] = scaleMatrix * glm::translate(glm::mat4(1.0f), glm::vec3(offsetTableX, 0.0f, 0.0f));

        // Mesh 2: Carriage (движется по Y, зависит от Table)
        transforms[2] = transforms[1] * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, offsetCarriageY, 0.0f));

        // Mesh 3: Head (движется по Z, зависит от Carriage)
        transforms[3] = transforms[2] * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, offsetHeadZ));

        ourModel.Draw(ourShader, transforms);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}