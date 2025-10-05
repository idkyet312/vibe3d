#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#ifdef VIBE3D_VULKAN_SUPPORT
#include "vulkan/VulkanRenderer.h"
#include "vulkan/ForwardPlusRenderer.h"
#endif

#include "GraphicsManager.h"
#include "MaterialSystem.h"
#include "PhysicsManager.h"
#include "InputManager.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

int main() {
    std::cout << "=== Vibe3D Engine - C++20 Forward+ Renderer ===" << std::endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    bool useVulkan = false;
    GLFWwindow* window = nullptr;
    std::unique_ptr<vibe::vk::ForwardPlusRenderer> vulkanRenderer;

#ifdef VIBE3D_VULKAN_SUPPORT
    std::cout << "\nAttempting Vulkan initialization..." << std::endl;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vibe3D - Vulkan Forward+", nullptr, nullptr);
    
    if (window) {
        try {
            vibe::vk::ForwardPlusRenderer::Config config{
                .width = SCR_WIDTH,
                .height = SCR_HEIGHT,
                .maxLights = 1024,
                .tileSize = 16,
                .enableMSAA = false,
                .msaaSamples = VK_SAMPLE_COUNT_1_BIT
            };
            
            vulkanRenderer = std::make_unique<vibe::vk::ForwardPlusRenderer>(config);
            if (vulkanRenderer->initialize(window)) {
                useVulkan = true;
                std::cout << "\n" << std::string(50, '=') << std::endl;
                std::cout << "??? Vulkan Forward+ Renderer ACTIVE!" << std::endl;
                std::cout << std::string(50, '=') << std::endl;
                std::cout << "Renderer: Vulkan API 1.4" << std::endl;
                std::cout << "Architecture: Forward+ (Tiled Forward)" << std::endl;
                std::cout << "Resolution: " << config.width << "x" << config.height << std::endl;
                std::cout << "Tiles: " << (config.width / config.tileSize) << "x" 
                          << (config.height / config.tileSize) << std::endl;
                std::cout << std::string(50, '=') << "\n" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Vulkan error: " << e.what() << std::endl;
            useVulkan = false;
        }
    }
#endif

    // Fallback to OpenGL if Vulkan failed
    if (!useVulkan) {
        std::cout << "\nUsing OpenGL fallback..." << std::endl;
        if (window) {
            glfwDestroyWindow(window);
        }
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vibe3D - OpenGL", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create window" << std::endl;
            glfwTerminate();
            return -1;
        }
        
        glfwMakeContextCurrent(window);
        glfwSwapInterval(0);
        
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return -1;
        }
        
        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    }

    // Initialize systems
    auto graphics = std::make_unique<GraphicsManager>();
    auto materials = std::make_unique<MaterialSystem>();
    auto physics = std::make_unique<PhysicsManager>();
    auto input = std::make_unique<InputManager>();
    
    if (!useVulkan) {
        if (!graphics->initialize(SCR_WIDTH, SCR_HEIGHT)) {
            std::cerr << "Failed to initialize graphics" << std::endl;
            return -1;
        }
        glEnable(GL_DEPTH_TEST);
    }
    
    if (!physics->initialize()) {
        std::cerr << "Failed to initialize physics" << std::endl;
        return -1;
    }
    
    input->initialize(window);
    
    std::cout << "\n=== CONTROLS ===" << std::endl;
    std::cout << "WASD   - Move camera" << std::endl;
    std::cout << "Mouse  - Look around" << std::endl;
    std::cout << "Space  - Jump" << std::endl;
    std::cout << "Click  - Shoot" << std::endl;
    std::cout << "E      - Spawn objects" << std::endl;
    std::cout << "M      - Cycle materials" << std::endl;
    std::cout << "ESC    - Exit" << std::endl;
    std::cout << "\nRenderer: " << (useVulkan ? "Vulkan Forward+" : "OpenGL") << std::endl;
    std::cout << "====================================\n" << std::endl;

    float lastFrame = 0.0f;
    float fpsTimer = 0.0f;
    int frameCount = 0;
    glm::vec3 cameraPos(0, 1.8f, 3);
    glm::vec3 mainObjPos(0, 2, 0);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        frameCount++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 1.0f) {
            float fps = frameCount / fpsTimer;
            std::cout << "FPS: " << static_cast<int>(fps) << " | Frame: " 
                      << (deltaTime * 1000.0f) << "ms | " 
                      << (useVulkan ? "Vulkan" : "OpenGL") << std::endl;
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        if (input->shouldExit(window)) {
            break;
        }

        cameraPos += input->getCameraMovement(window, deltaTime);
        physics->updatePhysics(deltaTime);
        physics->updateMainObject(mainObjPos, deltaTime);

        if (useVulkan && vulkanRenderer) {
            vulkanRenderer->beginFrame();
            
            vibe::vk::CameraUBO camera{};
            glm::vec3 cameraFront = input->getCameraFront();
            camera.view = glm::lookAt(cameraPos, cameraPos + cameraFront, glm::vec3(0, 1, 0));
            camera.projection = glm::perspective(glm::radians(45.0f), 
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
            camera.position = glm::vec4(cameraPos, 1.0f);
            
            std::vector<vibe::vk::PointLight> lights = {
                { glm::vec3(2, 2, 2), 10.0f, glm::vec3(1, 1, 1), 1.0f }
            };
            
            vulkanRenderer->renderScene(camera, lights);
            vulkanRenderer->endFrame();
        } else {
            graphics->beginFrame();
            
            glm::vec3 cameraFront = input->getCameraFront();
            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, glm::vec3(0, 1, 0));
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                                    (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
            
            std::vector<RTSphere> dummy;
            graphics->renderForwardPlusPass(view, projection, dummy, physics->getCubes(), 
                                          physics->getBullets(), mainObjPos, materials->getCurrentMaterial());
            
            float fps = frameCount / std::max(fpsTimer, 0.001f);
            graphics->renderFPS(fps, deltaTime);
            graphics->endFrame();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    vulkanRenderer.reset();
    graphics.reset();
    materials.reset();
    physics.reset();
    input.reset();
    
    glfwTerminate();
    return 0;
}