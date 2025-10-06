#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class InputManager {
public:
    InputManager();
    
    // Initialization
    void initialize(GLFWwindow* window);
    
    // Input processing
    void processInput(GLFWwindow* window, float deltaTime);
    
    // Camera controls
    glm::vec3 getCameraMovement(GLFWwindow* window, float deltaTime) const;
    bool shouldJump(GLFWwindow* window) const;
    
    // Action inputs
    bool shouldShoot(GLFWwindow* window) const;
    bool shouldSpawnCube(GLFWwindow* window) const;
    bool shouldCycleMaterial(GLFWwindow* window);
    bool shouldToggleRaytracing(GLFWwindow* window);
    bool shouldToggleShadowDebug(GLFWwindow* window);
    bool shouldIncreaseExposure(GLFWwindow* window) const;
    bool shouldDecreaseExposure(GLFWwindow* window) const;
    bool shouldExit(GLFWwindow* window) const;
    
    // Camera freeze toggle
    bool shouldToggleCameraFreeze(GLFWwindow* window);
    bool isCameraFrozen() const { return cameraFrozen; }
    void setCursorMode(GLFWwindow* window);
    
    // Mouse handling
    void handleMouseMovement(double xpos, double ypos);
    glm::vec3 getCameraFront() const { return cameraFront; }
    
    // Camera settings
    void setCameraProperties(float sensitivity, float speed);
    
private:
    // Camera variables
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float lastX, lastY;
    float yaw, pitch;
    bool firstMouse;
    float mouseSensitivity;
    float cameraSpeed;
    
    // Input state tracking
    bool materialKeyPressed;
    bool raytracingKeyPressed;
    bool shadowDebugKeyPressed;
    bool cameraFreezeKeyPressed;
    bool cameraFrozen;
    
    // Static callback functions
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static InputManager* instance;
};