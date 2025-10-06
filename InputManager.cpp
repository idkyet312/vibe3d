#include "InputManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream> // For std::cout

InputManager* InputManager::instance = nullptr;

InputManager::InputManager() 
    : cameraFront(0.0f, 0.0f, -1.0f)
    , cameraUp(0.0f, 1.0f, 0.0f)
    , lastX(400.0f), lastY(300.0f)
    , yaw(-90.0f), pitch(0.0f)
    , firstMouse(true)
    , mouseSensitivity(0.15f)
    , cameraSpeed(6.5f)
    , materialKeyPressed(false)
    , raytracingKeyPressed(false)
    , shadowDebugKeyPressed(false)
{
    instance = this;
}

void InputManager::initialize(GLFWwindow* window) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
}

void InputManager::processInput(GLFWwindow* window, float deltaTime) {
    // This function can be used for any general input processing
    // Most specific input handling is done through the individual functions
}

glm::vec3 InputManager::getCameraMovement(GLFWwindow* window, float deltaTime) const {
    glm::vec3 movement(0.0f);
    float speed = cameraSpeed * deltaTime;
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movement += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movement -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movement -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movement += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
        
    return movement;
}

bool InputManager::shouldJump(GLFWwindow* window) const {
    return glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
}

bool InputManager::shouldShoot(GLFWwindow* window) const {
    return glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}

bool InputManager::shouldSpawnCube(GLFWwindow* window) const {
    return glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
}

bool InputManager::shouldCycleMaterial(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !materialKeyPressed) {
        materialKeyPressed = true;
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
        materialKeyPressed = false;
    }
    return false;
}

bool InputManager::shouldToggleRaytracing(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !raytracingKeyPressed) {
        raytracingKeyPressed = true;
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        raytracingKeyPressed = false;
    }
    return false;
}

bool InputManager::shouldToggleShadowDebug(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !shadowDebugKeyPressed) {
        shadowDebugKeyPressed = true;
        std::cout << "[DEBUG] B key pressed - toggling shadow debug" << std::endl;
        return true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE) {
        shadowDebugKeyPressed = false;
    }
    return false;
}

bool InputManager::shouldIncreaseExposure(GLFWwindow* window) const {
    return glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS; // + key
}

bool InputManager::shouldDecreaseExposure(GLFWwindow* window) const {
    return glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS; // - key
}

bool InputManager::shouldExit(GLFWwindow* window) const {
    return glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

void InputManager::handleMouseMovement(double xpos, double ypos) {
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void InputManager::setCameraProperties(float sensitivity, float speed) {
    mouseSensitivity = sensitivity;
    cameraSpeed = speed;
}

void InputManager::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (instance) {
        instance->handleMouseMovement(xpos, ypos);
    }
}