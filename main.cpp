#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "GraphicsManager.h"
#include "MaterialSystem.h"
#include "PhysicsManager.h"
#include "InputManager.h"

// Application settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Application state
struct AppState {
    // Rendering
    bool useRaytracing = true;
    int maxBounces = 5;
    int numSamples = 1;
    float exposure = 1.0f;
    bool enableToneMapping = true;
    
    // Camera
    glm::vec3 cameraPos = glm::vec3(0.0f, 1.8f, 3.0f);
    float verticalVelocity = 0.0f;
    bool isGrounded = true;
    
    // Scene objects
    glm::vec3 mainObjectPos = glm::vec3(0.0f, 2.0f, 0.0f);
    
    // Lighting
    glm::vec3 lightPos = glm::vec3(1.2f, 1.0f, 2.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    // Timing
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    
    // FPS tracking
    float fps = 0.0f;
    float fpsUpdateTimer = 0.0f;
    int frameCount = 0;
};

// Manager instances
GraphicsManager* graphics = nullptr;
MaterialSystem* materials = nullptr;
PhysicsManager* physics = nullptr;
InputManager* input = nullptr;

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
bool initializeApplication();
void cleanupApplication();
void updateApplication(GLFWwindow* window, AppState& state);
void renderApplication(const AppState& state);
void printApplicationInfo();
std::vector<RTSphere> buildRaytracingScene(const AppState& state);

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vibe3D Game - Raytracing Edition", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Initialize application
    if (!initializeApplication()) {
        cleanupApplication();
        glfwTerminate();
        return -1;
    }

    // Setup input
    input->initialize(window);
    
    // Uncap framerate - disable V-Sync
    glfwSwapInterval(0);
    
    // Print application info
    printApplicationInfo();

    // Configure OpenGL state
    glEnable(GL_DEPTH_TEST);

    // Application state
    AppState state;
    
    // Check if raytracing is supported
    if (!graphics->isRaytracingSupported()) {
        state.useRaytracing = false;
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = static_cast<float>(glfwGetTime());
        state.deltaTime = currentFrame - state.lastFrame;
        state.lastFrame = currentFrame;
        
        // Calculate FPS
        state.frameCount++;
        state.fpsUpdateTimer += state.deltaTime;
        
        if (state.fpsUpdateTimer >= 1.0f) { // Update FPS every second
            state.fps = state.frameCount / state.fpsUpdateTimer;
            state.frameCount = 0;
            state.fpsUpdateTimer = 0.0f;
        }

        // Update application
        updateApplication(window, state);
        
        // Render application
        renderApplication(state);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    cleanupApplication();
    glfwTerminate();
    return 0;
}

bool initializeApplication() {
    // Create manager instances
    graphics = new GraphicsManager();
    materials = new MaterialSystem();
    physics = new PhysicsManager();
    input = new InputManager();
    
    // Initialize managers
    if (!graphics->initialize(SCR_WIDTH, SCR_HEIGHT)) {
        std::cerr << "Failed to initialize graphics manager" << std::endl;
        return false;
    }
    
    if (!physics->initialize()) {
        std::cerr << "Failed to initialize physics manager" << std::endl;
        return false;
    }
    
    return true;
}

void cleanupApplication() {
    delete graphics;
    delete materials;
    delete physics;
    delete input;
}

void updateApplication(GLFWwindow* window, AppState& state) {
    // Handle input
    if (input->shouldExit(window)) {
        glfwSetWindowShouldClose(window, true);
        return;
    }
    
    // Material cycling
    if (input->shouldCycleMaterial(window)) {
        materials->cycleMaterial();
    }
    
    // Raytracing toggle
    if (input->shouldToggleRaytracing(window)) {
        if (graphics->isRaytracingSupported()) {
            state.useRaytracing = !state.useRaytracing;
            std::cout << "Raytracing " << (state.useRaytracing ? "enabled" : "disabled") << std::endl;
        } else {
            std::cout << "Raytracing not supported on this system" << std::endl;
        }
    }
    
    // Exposure controls
    if (input->shouldIncreaseExposure(window)) {
        state.exposure *= 1.02f;
        std::cout << "Exposure: " << state.exposure << std::endl;
    }
    if (input->shouldDecreaseExposure(window)) {
        state.exposure *= 0.98f;
        std::cout << "Exposure: " << state.exposure << std::endl;
    }
    
    // Camera movement
    glm::vec3 cameraMovement = input->getCameraMovement(window, state.deltaTime);
    state.cameraPos += cameraMovement;
    
    // Jumping mechanics
    if (input->shouldJump(window) && state.isGrounded) {
        state.verticalVelocity = 5.0f; // Jump force
        state.isGrounded = false;
    }
    
    // Shooting
    if (input->shouldShoot(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        if (physics->canShoot(currentTime)) {
            glm::vec3 bulletPos = state.cameraPos + input->getCameraFront() * 0.5f;
            physics->shootBullet(bulletPos, input->getCameraFront());
            physics->updateLastShotTime(currentTime);
        }
    }
    
    // Spawn cubes
    if (input->shouldSpawnCube(window)) {
        glm::vec3 spawnPos = state.cameraPos + input->getCameraFront() * 3.0f;
        glm::vec3 spawnVel = input->getCameraFront() * 5.0f;
        physics->spawnCube(spawnPos, spawnVel);
    }
    
    // Update physics
    physics->updatePhysics(state.deltaTime);
    
    // Update camera physics (only in non-raytracing mode for simplicity)
    if (!state.useRaytracing) {
        physics->updateCameraPhysics(state.cameraPos, state.verticalVelocity, state.isGrounded, state.deltaTime);
    }
    
    // Update main object physics
    physics->updateMainObject(state.mainObjectPos, state.deltaTime);
    
    // Update graphics lighting
    graphics->setLightProperties(state.lightPos, state.lightColor);
}

void renderApplication(const AppState& state) {
    if (graphics->useModernRenderer()) {
        // Modern Vulkan-based Forward+ with Ray Tracing
        std::vector<RTSphere> rtSpheres = buildRaytracingScene(state);
        
        glm::vec3 cameraFront = input->getCameraFront();
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));
        
        graphics->renderModern(rtSpheres, state.cameraPos, cameraFront, cameraUp, cameraRight,
                              state.lightPos, state.lightColor, state.lastFrame,
                              physics->getCubes(), physics->getBullets(),
                              state.mainObjectPos, materials->getCurrentMaterial());
    } else if (state.useRaytracing && graphics->isRaytracingSupported()) {
        // Legacy OpenGL raytracing mode
        std::vector<RTSphere> rtSpheres = buildRaytracingScene(state);
        
        glm::vec3 cameraFront = input->getCameraFront();
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));
        
        graphics->renderRaytraced(rtSpheres, state.cameraPos, cameraFront, cameraUp, cameraRight,
                                 state.lightPos, state.lightColor, state.lastFrame,
                                 state.maxBounces, state.numSamples, state.exposure, state.enableToneMapping);
    } else {
        // Legacy Forward+ rendering mode (with fallback to traditional forward if not supported)
        graphics->beginFrame();
        
        // Calculate view and projection matrices
        glm::vec3 cameraFront = input->getCameraFront();
        glm::mat4 view = glm::lookAt(state.cameraPos, state.cameraPos + cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        
        // Use the Forward+ rendering pipeline (automatically falls back to traditional forward if not supported)
        std::vector<RTSphere> rtSpheres = buildRaytracingScene(state); // Reuse for object data
        graphics->renderForwardPlusPass(view, projection, rtSpheres, physics->getCubes(), physics->getBullets(),
                                       state.mainObjectPos, materials->getCurrentMaterial());
        
        graphics->endFrame();
    }
    
    // Render FPS overlay (always visible in all modes)
    graphics->renderFPS(state.fps, state.deltaTime);
}

std::vector<RTSphere> buildRaytracingScene(const AppState& state) {
    std::vector<RTSphere> rtSpheres;
    
    // Add main sphere
    RTSphere mainSphere;
    mainSphere.center = state.mainObjectPos;
    mainSphere.radius = 0.5f;
    
    // Determine material type
    int materialType = 0; // Default to diffuse
    if (materials->isMaterialMetallic(materials->getCurrentMaterial())) {
        materialType = 1; // Metal
    }
    if (materials->isMaterialGlass(materials->getCurrentMaterial())) {
        materialType = 2; // Glass
    }
    
    mainSphere.material = materials->convertToRTMaterial(materials->getCurrentMaterial(), materialType);
    rtSpheres.push_back(mainSphere);
    
    // Add spawned cubes
    for (const auto& cube : physics->getCubes()) {
        if (cube.isActive) {
            RTSphere sphere;
            sphere.center = cube.position;
            sphere.radius = 0.25f; // Half of cube size
            sphere.material = materials->convertToRTMaterial(materials->getAllMaterials()[2], 0); // Bronze diffuse
            sphere.material.albedo = glm::vec3(0.3f, 0.8f, 0.3f);
            rtSpheres.push_back(sphere);
        }
    }
    
    // Add bullets
    for (const auto& bullet : physics->getBullets()) {
        if (bullet.active) {
            RTSphere sphere;
            sphere.center = bullet.position;
            sphere.radius = 0.05f;
            sphere.material = materials->convertToRTMaterial(materials->getAllMaterials()[1], 1); // Gold metal
            sphere.material.albedo = glm::vec3(1.0f, 1.0f, 0.0f);
            rtSpheres.push_back(sphere);
        }
    }
    
    return rtSpheres;
}

void printApplicationInfo() {
    // Check OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // Print material system info
    std::cout << "Material System Initialized with " << materials->getMaterialCount() << " materials" << std::endl;
    std::cout << "Current material: " << materials->getCurrentMaterial().name << std::endl;
    
    // Print controls
    std::cout << "\n=== CONTROLS ===" << std::endl;
    std::cout << "WASD - Camera movement" << std::endl;
    std::cout << "Mouse - Look around" << std::endl;
    std::cout << "Space - Jump" << std::endl;
    std::cout << "Left Click - Shoot bullets" << std::endl;
    std::cout << "E - Spawn spheres" << std::endl;
    std::cout << "M - Cycle through materials" << std::endl;
    
    if (graphics->isRaytracingSupported()) {
        std::cout << "R - Toggle raytracing mode" << std::endl;
    } else {
        std::cout << "Raytracing not available - using enhanced rasterization with PBR-like features" << std::endl;
    }
    
    std::cout << "+ and - - Adjust exposure" << std::endl;
    std::cout << "Escape - Exit" << std::endl;
    
    std::cout << "\n=== ENHANCED FEATURES ===" << std::endl;
    std::cout << "- PBR-like material rendering" << std::endl;
    std::cout << "- Environment reflections" << std::endl;
    std::cout << "- Metallic/Roughness workflow" << std::endl;
    std::cout << "- Fresnel reflections" << std::endl;
    std::cout << "- Ambient occlusion" << std::endl;
    
    if (graphics->isRaytracingSupported()) {
        std::cout << "- Hardware raytracing" << std::endl;
        std::cout << "- Physically-based lighting" << std::endl;
        std::cout << "- Global illumination" << std::endl;
    }
    std::cout << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}