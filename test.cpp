#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#if PHYSX_ENABLED
#include <PxPhysicsAPI.h>
using namespace physx;
#endif

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

// Material properties structure
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    std::string name;
};

// Raytracing material structure
struct RTMaterial {
    glm::vec3 albedo;
    glm::vec3 specular;
    float shininess;
    float metallic;
    float roughness;
    float ior;
    int type; // 0 = diffuse, 1 = metal, 2 = glass
};

// Raytracing sphere structure
struct RTSphere {
    glm::vec3 center;
    float radius;
    RTMaterial material;
};

// Predefined materials
std::vector<Material> materials = {
    { glm::vec3(0.1745f, 0.01175f, 0.01175f), glm::vec3(0.61424f, 0.04136f, 0.04136f), glm::vec3(0.727811f, 0.626959f, 0.626959f), 76.8f, "Ruby" },
    { glm::vec3(0.329412f, 0.223529f, 0.027451f), glm::vec3(0.780392f, 0.568627f, 0.113725f), glm::vec3(0.992157f, 0.941176f, 0.807843f), 27.8974f, "Gold" },
    { glm::vec3(0.2125f, 0.1275f, 0.054f), glm::vec3(0.714f, 0.4284f, 0.18144f), glm::vec3(0.393548f, 0.271906f, 0.166721f), 25.6f, "Bronze" },
    { glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.774597f, 0.774597f, 0.774597f), 76.8f, "Silver" },
    { glm::vec3(0.19125f, 0.0735f, 0.0225f), glm::vec3(0.7038f, 0.27048f, 0.0828f), glm::vec3(0.256777f, 0.137622f, 0.086014f), 12.8f, "Copper" },
    { glm::vec3(0.0f, 0.05f, 0.0f), glm::vec3(0.4f, 0.5f, 0.4f), glm::vec3(0.04f, 0.7f, 0.04f), 10.0f, "Emerald" },
    { glm::vec3(0.02f, 0.02f, 0.02f), glm::vec3(0.01f, 0.01f, 0.01f), glm::vec3(0.4f, 0.4f, 0.4f), 10.0f, "Black Plastic" },
    { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.7f, 0.6f, 0.6f), 32.0f, "Red Plastic" },
    { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.1f, 0.35f, 0.1f), glm::vec3(0.45f, 0.55f, 0.45f), 32.0f, "Green Plastic" },
    { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.6f, 0.6f, 0.7f), 32.0f, "Blue Plastic" }
};

int currentMaterialIndex = 0;
bool materialKeyPressed = false;

std::vector<RTMaterial> rtMaterials;

// Raytracing settings
bool useRaytracing = true;
bool raytracingKeyPressed = false;
int maxBounces = 5;
int numSamples = 1;
float exposure = 1.0f;
bool enableToneMapping = true;

// Camera variables
glm::vec3 cameraPos   = glm::vec3(0.0f, 1.8f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));
float cameraSpeed = 10.0f;
float lastX = 400.0f;
float lastY = 300.0f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float mouseSensitivity = 0.15f;

// Physics variables
float gravity = -9.81f;
float jumpForce = 5.0f;
float verticalVelocity = 0.0f;
bool isGrounded = true;

// Simple physics for fallback
struct SimplePhysicsObject {
    glm::vec3 position;
    glm::vec3 velocity;
    float mass = 1.0f;
    float size = 0.5f;
    bool isActive = false;
};

// Bullet system
struct Bullet {
    glm::vec3 position;
    glm::vec3 direction;
    float speed = 30.0f;
    float lifetime = 5.0f;
    float timeAlive = 0.0f;
    bool active = false;
#if PHYSX_ENABLED
    PxRigidDynamic* actor = nullptr;
#endif
    SimplePhysicsObject simplePhysics;
};

std::vector<Bullet> bullets;
float bulletRadius = 0.05f;
float lastShotTime = 0.0f;
float shootCooldown = 0.15f;

#if PHYSX_ENABLED
// PhysX global variables
PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;
PxFoundation* gFoundation = nullptr;
PxPhysics* gPhysics = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
PxScene* gScene = nullptr;
PxMaterial* gMaterial = nullptr;
PxPvd* gPvd = nullptr;
#endif

// Cube structure and variables
struct Cube {
    glm::vec3 position;
#if PHYSX_ENABLED
    PxRigidDynamic* actor = nullptr;
#endif
    bool isActive = false;
    SimplePhysicsObject simplePhysics;
};

std::vector<Cube> cubes;
const int MAX_CUBES = 50;
const float CUBE_MASS = 1.0f;
const float CUBE_SIZE = 0.5f;
const float CUBE_INITIAL_SPEED = 5.0f;
float lastCubeSpawnTime = 0.0f;
const float CUBE_SPAWN_COOLDOWN = 1.0f;

// Raytracing variables
std::vector<RTSphere> rtSpheres;
GLuint raytracingTexture;
GLuint computeShader;
GLuint fullscreenShader;
GLuint fullscreenVAO;

// Settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Global variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
glm::vec3 cubePos(0.0f, 2.0f, 0.0f);

#if PHYSX_ENABLED
PxRigidDynamic* dynamicActor = nullptr;
#endif

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void initPhysX();
void cleanupPhysX();
void updatePhysics(float deltaTime);
void updateSimplePhysics(float deltaTime);
void spawnCube();
GLuint loadShaders(const char* vertex_file_path, const char* fragment_file_path);
GLuint loadComputeShader(const char* compute_file_path);
void createFloorMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices);
void createSphereMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int segments);
void shootBullet();
void cycleMaterial();
void initRaytracing();
void updateRaytracingScene();
void renderRaytraced();
void createFullscreenQuad();
RTMaterial convertToRTMaterial(const Material& mat, int type = 0);

// OpenGL function pointers for compute shaders (in case GLAD doesn't load them)
typedef void (APIENTRY *PFNGLDISPATCHCOMPUTEPROC)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void (APIENTRY *PFNGLBINDIMAGETEXTUREPROC)(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
typedef void (APIENTRY *PFNGLMEMORYBARRIERPROC)(GLbitfield barriers);

PFNGLDISPATCHCOMPUTEPROC glDispatchCompute = nullptr;
PFNGLBINDIMAGETEXTUREPROC glBindImageTexture = nullptr;
PFNGLMEMORYBARRIERPROC glMemoryBarrier = nullptr;

int main()
{
    // glfw: initialize and configure
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vibe3D Game - Raytracing Edition", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Load OpenGL function pointers for compute shaders
    glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)glfwGetProcAddress("glDispatchCompute");
    glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)glfwGetProcAddress("glBindImageTexture");
    glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)glfwGetProcAddress("glMemoryBarrier");

    // Check OpenGL version and compute shader support
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // Check if compute shaders are supported
    int major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    bool computeShaderSupported = (major > 4) || (major == 4 && minor >= 3);
    
    if (computeShaderSupported) {
        // Try to query compute shader capabilities
        GLint maxComputeWorkGroupInvocations = 0;
        glGetIntegerv(0x90EB, &maxComputeWorkGroupInvocations); // GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
        
        if (glGetError() == GL_NO_ERROR && maxComputeWorkGroupInvocations > 0) {
            std::cout << "Max compute work group invocations: " << maxComputeWorkGroupInvocations << std::endl;
            std::cout << "Compute shaders supported - raytracing available!" << std::endl;
        } else {
            std::cout << "Compute shader capability query failed" << std::endl;
            computeShaderSupported = false;
        }
    } else {
        std::cout << "Compute shaders not supported (OpenGL 4.3+ required)" << std::endl;
        std::cout << "Current version: " << major << "." << minor << std::endl;
        std::cout << "Using enhanced rasterization mode instead" << std::endl;
        useRaytracing = false;
    }
    
    // Force enable raytracing if we have OpenGL 4.3+
    if (computeShaderSupported) {
        useRaytracing = true;
    } else {
        useRaytracing = false;
    }

    // Print PhysX status
#if PHYSX_ENABLED
    std::cout << "PhysX enabled - using hardware physics" << std::endl;
#else
    std::cout << "PhysX not available - using simple physics fallback" << std::endl;
#endif

    // Print material system info
    std::cout << "Material System Initialized with " << materials.size() << " materials" << std::endl;
    std::cout << "Current material: " << materials[currentMaterialIndex].name << std::endl;
    std::cout << "Press M to cycle through materials" << std::endl;
    if (useRaytracing) {
        std::cout << "Press R to toggle raytracing mode" << std::endl;
    } else {
        std::cout << "Raytracing not available - using enhanced rasterization with PBR-like features" << std::endl;
    }
    std::cout << "Press + and - to adjust exposure" << std::endl;
    std::cout << "Enhanced Features:" << std::endl;
    std::cout << "- PBR-like material rendering" << std::endl;
    std::cout << "- Environment reflections" << std::endl;
    std::cout << "- Metallic/Roughness workflow" << std::endl;
    std::cout << "- Fresnel reflections" << std::endl;
    std::cout << "- Ambient occlusion" << std::endl;

    // Configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // Build and compile our shader program
    GLuint shaderProgram = loadShaders("vertex.glsl", "fragment.glsl"); 
    if (shaderProgram == 0) {
         glfwTerminate();
         return -1;
    }

    GLuint floorShaderProgram = loadShaders("floor_vertex.glsl", "floor_fragment.glsl");
    if (floorShaderProgram == 0) {
        glfwTerminate();
        return -1;
    }

    // Load raytracing shaders
    if (useRaytracing) {
        computeShader = loadComputeShader("raytracing.comp");
        if (computeShader == 0) {
            std::cerr << "Failed to load raytracing compute shader" << std::endl;
            useRaytracing = false;
        }

        fullscreenShader = loadShaders("fullscreen_vertex.glsl", "fullscreen_fragment.glsl");
        if (fullscreenShader == 0) {
            std::cerr << "Failed to load fullscreen shader" << std::endl;
            useRaytracing = false;
        }

        // Initialize raytracing
        if (useRaytracing) {
            initRaytracing();
            createFullscreenQuad();
            std::cout << "Raytracing initialized successfully!" << std::endl;
        }
    }

    // Create sphere mesh
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    createSphereMesh(sphereVertices, sphereIndices, 0.5f, 32); // radius = 0.5, 32 segments

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Store sphere index count for rendering
    const int sphereIndexCount = static_cast<int>(sphereIndices.size());
    
    // Floor mesh
    std::vector<float> floorVertices;
    std::vector<unsigned int> floorIndices;
    createFloorMesh(floorVertices, floorIndices);

    unsigned int floorVAO, floorVBO, floorEBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, floorVertices.size() * sizeof(float), floorVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, floorIndices.size() * sizeof(unsigned int), floorIndices.data(), GL_STATIC_DRAW);

    // Floor vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Initialize PhysX
    initPhysX();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        if (useRaytracing) {
            // Raytracing mode
            updateRaytracingScene();
            renderRaytraced();
        } else {
            // Traditional rasterization mode
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            updatePhysics(deltaTime);

            // Update camera
            glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

            // Draw floor
            glUseProgram(floorShaderProgram);
            glm::mat4 floorModel = glm::mat4(1.0f);
            floorModel = glm::translate(floorModel, glm::vec3(0.0f, -0.5f, 0.0f));
            floorModel = glm::scale(floorModel, glm::vec3(20.0f, 0.1f, 20.0f));
            glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "model"), 1, GL_FALSE, &floorModel[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
            glUniform3f(glGetUniformLocation(floorShaderProgram, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
            glUniform3f(glGetUniformLocation(floorShaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
            glUniform3f(glGetUniformLocation(floorShaderProgram, "lightColor"), lightColor.x, lightColor.y, lightColor.z);

            glBindVertexArray(floorVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Draw spheres using material-based rendering
            glUseProgram(shaderProgram);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
            glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
            glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
            glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), lightColor.x, lightColor.y, lightColor.z);

            // Draw main sphere with current material
            Material& currentMaterial = materials[currentMaterialIndex];
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
            
            // Set material properties
            glUniform3f(glGetUniformLocation(shaderProgram, "material.ambient"), currentMaterial.ambient.x, currentMaterial.ambient.y, currentMaterial.ambient.z);
            glUniform3f(glGetUniformLocation(shaderProgram, "material.diffuse"), currentMaterial.diffuse.x, currentMaterial.diffuse.y, currentMaterial.diffuse.z);
            glUniform3f(glGetUniformLocation(shaderProgram, "material.specular"), currentMaterial.specular.x, currentMaterial.specular.y, currentMaterial.specular.z);
            glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), currentMaterial.shininess);
            
            // Enhanced rendering features
            bool isMetallic = (currentMaterial.name.find("Gold") != std::string::npos ||
                              currentMaterial.name.find("Silver") != std::string::npos ||
                              currentMaterial.name.find("Copper") != std::string::npos ||
                              currentMaterial.name.find("Bronze") != std::string::npos);
            
            float metallicFactor = isMetallic ? 0.9f : 0.1f;
            float roughnessFactor = isMetallic ? 0.1f : 0.8f;
            float ambientOcclusion = 0.1f; // Subtle AO
            
            glUniform1i(glGetUniformLocation(shaderProgram, "enableReflections"), true);
            glUniform1i(glGetUniformLocation(shaderProgram, "enableSSAO"), false); // Keep simple for now
            glUniform1f(glGetUniformLocation(shaderProgram, "ambientOcclusion"), ambientOcclusion);
            glUniform1f(glGetUniformLocation(shaderProgram, "metallicFactor"), metallicFactor);
            glUniform1f(glGetUniformLocation(shaderProgram, "roughnessFactor"), roughnessFactor);
            glUniform1i(glGetUniformLocation(shaderProgram, "hasEnvironmentMap"), false); // No cubemap for now
            
            glUniform1i(glGetUniformLocation(shaderProgram, "useMaterial"), 1); // Enable material mode
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

            // Draw spawned spheres with enhanced simple shading
            for (const auto& cube : cubes) {
                if (cube.isActive) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, cube.position);
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
                    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.3f, 0.8f, 0.3f);
                    glUniform1i(glGetUniformLocation(shaderProgram, "shadingModel"), 0); // Lambert
                    glUniform1i(glGetUniformLocation(shaderProgram, "useMaterial"), 0); // Disable material mode
                    
                    // Enhanced features for spawned cubes
                    glUniform1i(glGetUniformLocation(shaderProgram, "enableReflections"), false);
                    glUniform1f(glGetUniformLocation(shaderProgram, "ambientOcclusion"), 0.2f);
                    
                    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
                }
            }

            // Draw bullet spheres with enhanced shiny rendering
            for (const auto& bullet : bullets) {
                if (bullet.active) {
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, bullet.position);
                    model = glm::scale(model, glm::vec3(bulletRadius));
                    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
                    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 0.0f);
                    glUniform1i(glGetUniformLocation(shaderProgram, "shadingModel"), 1); // Blinn-Phong
                    glUniform1i(glGetUniformLocation(shaderProgram, "useMaterial"), 0); // Disable material mode
                    
                    // Enhanced features for bullets (make them shiny)
                    glUniform1i(glGetUniformLocation(shaderProgram, "enableReflections"), true);
                    glUniform1f(glGetUniformLocation(shaderProgram, "ambientOcclusion"), 0.0f);
                    
                    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanupPhysX();

    // Cleanup
    if (useRaytracing) {
        glDeleteTextures(1, &raytracingTexture);
        glDeleteProgram(computeShader);
        glDeleteProgram(fullscreenShader);
        glDeleteVertexArrays(1, &fullscreenVAO);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(floorShaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Material cycling
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !materialKeyPressed) {
        cycleMaterial();
        materialKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
        materialKeyPressed = false;
    }

    // Raytracing toggle
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !raytracingKeyPressed) {
        useRaytracing = !useRaytracing;
        std::cout << "Raytracing " << (useRaytracing ? "enabled" : "disabled") << std::endl;
        raytracingKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        raytracingKeyPressed = false;
    }

    // Exposure controls
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) { // + key
        exposure *= 1.02f;
        std::cout << "Exposure: " << exposure << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) { // - key
        exposure *= 0.98f;
        std::cout << "Exposure: " << exposure << std::endl;
    }

    // Camera movement
    float cameraSpeed = 6.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    // Jumping mechanics
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
        verticalVelocity = jumpForce;
        isGrounded = false;
    }

    // Shooting
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        shootBullet();
    }

    // Apply gravity (only in non-raytracing mode for simplicity)
    if (!useRaytracing) {
        verticalVelocity += gravity * deltaTime;
        cameraPos.y += verticalVelocity * deltaTime;

        // Ground collision
        if (cameraPos.y <= 1.0f) {
            cameraPos.y = 1.0f;
            verticalVelocity = 0.0f;
            isGrounded = true;
        }
    }

    // Physics-related input
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        spawnCube();
    }
}

void cycleMaterial() {
    currentMaterialIndex = (currentMaterialIndex + 1) % materials.size();
    std::cout << "Material changed to: " << materials[currentMaterialIndex].name << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
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

GLuint loadShaders(const char * vertex_file_path,const char * fragment_file_path){
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open()){
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}else{
		std::cerr << "Impossible to open " << vertex_file_path << std::endl;
        return 0;
	}

	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	} else {
        std::cerr << "Impossible to open " << fragment_file_path << std::endl;
        return 0;
    }

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		std::cerr << "Vertex Shader Error: " << &VertexShaderErrorMessage[0] << std::endl;
		return 0;
	}

	// Compile Fragment Shader
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		std::cerr << "Fragment Shader Error: " << &FragmentShaderErrorMessage[0] << std::endl;
		return 0;
	}

	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		std::cerr << "Linking Error: " << &ProgramErrorMessage[0] << std::endl;
		return 0;
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

GLuint loadComputeShader(const char * compute_file_path) {
    // Check if compute shaders are supported first
    int major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    if (major < 4 || (major == 4 && minor < 3)) {
        std::cerr << "Compute shaders require OpenGL 4.3+, current version: " << major << "." << minor << std::endl;
        return 0;
    }

    GLuint ComputeShaderID = glCreateShader(0x91B9); // GL_COMPUTE_SHADER

    std::string ComputeShaderCode;
    std::ifstream ComputeShaderStream(compute_file_path, std::ios::in);
    if(ComputeShaderStream.is_open()){
        std::stringstream sstr;
        sstr << ComputeShaderStream.rdbuf();
        ComputeShaderCode = sstr.str();
        ComputeShaderStream.close();
    }else{
        std::cerr << "Impossible to open " << compute_file_path << std::endl;
        return 0;
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Compute Shader
    char const* ComputeSourcePointer = ComputeShaderCode.c_str();
    glShaderSource(ComputeShaderID, 1, &ComputeSourcePointer, NULL);
    glCompileShader(ComputeShaderID);

    glGetShaderiv(ComputeShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(ComputeShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> ComputeShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(ComputeShaderID, InfoLogLength, NULL, &ComputeShaderErrorMessage[0]);
        std::cerr << "Compute Shader Error: " << &ComputeShaderErrorMessage[0] << std::endl;
        return 0;
    }

    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, ComputeShaderID);
    glLinkProgram(ProgramID);

    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        std::vector<char> ProgramErrorMessage(InfoLogLength+1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        std::cerr << "Compute Shader Linking Error: " << &ProgramErrorMessage[0] << std::endl;
        return 0;
    }

    glDetachShader(ProgramID, ComputeShaderID);
    glDeleteShader(ComputeShaderID);

    return ProgramID;
}

void createFloorMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    vertices = {
        // positions         // colors (grey)      // normals
        -10.0f, 0.0f, -10.0f,  0.3f, 0.3f, 0.3f,  0.0f, 1.0f, 0.0f,
         10.0f, 0.0f, -10.0f,  0.3f, 0.3f, 0.3f,  0.0f, 1.0f, 0.0f,
         10.0f, 0.0f,  10.0f,  0.3f, 0.3f, 0.3f,  0.0f, 1.0f, 0.0f,
        -10.0f, 0.0f,  10.0f,  0.3f, 0.3f, 0.3f,  0.0f, 1.0f, 0.0f
    };

    indices = {
        0, 1, 2,
        2, 3, 0
    };
}

void createSphereMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int segments) {
    vertices.clear();
    indices.clear();

    // Generate vertices
    for (int i = 0; i <= segments; i++) {
        float lat = glm::pi<float>() * (-0.5f + (float)i / segments);
        float y = radius * sin(lat);
        float r = radius * cos(lat);

        for (int j = 0; j <= segments; j++) {
            float lon = 2.0f * glm::pi<float>() * (float)j / segments;
            float x = r * cos(lon);
            float z = r * sin(lon);

            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            // Normal (same as position for unit sphere, then scaled)
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            
            // Texture coordinates
            vertices.push_back((float)j / segments);
            vertices.push_back((float)i / segments);
        }
    }

    // Generate indices
    for (int i = 0; i < segments; i++) {
        for (int j = 0; j < segments; j++) {
            int first = i * (segments + 1) + j;
            int second = first + segments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
}

void shootBullet() {
    float currentTime = glfwGetTime();
    if (currentTime - lastShotTime < shootCooldown) {
        return;
    }
    lastShotTime = currentTime;

    Bullet bullet;
    bullet.position = cameraPos + cameraFront * 0.5f;
    bullet.direction = glm::normalize(cameraFront);
    bullet.active = true;
    bullet.timeAlive = 0.0f;

#if PHYSX_ENABLED
    // PhysX implementation would go here when PhysX is available
#else
    // Simple physics fallback
    bullet.simplePhysics.position = bullet.position;
    bullet.simplePhysics.velocity = bullet.direction * bullet.speed;
    bullet.simplePhysics.isActive = true;
#endif

    bullets.push_back(bullet);
}

void initPhysX() {
#if PHYSX_ENABLED
    // PhysX initialization would go here when PhysX is available
#else
    // Simple physics - no initialization needed
    std::cout << "Using simple physics fallback" << std::endl;
#endif
}

void cleanupPhysX() {
#if PHYSX_ENABLED
    // PhysX cleanup would go here when PhysX is available
#endif
}

void spawnCube() {
    if (cubes.size() >= MAX_CUBES) {
        return;
    }

    Cube cube;
    glm::vec3 spawnPos = cameraPos + cameraFront * 3.0f;
    cube.position = spawnPos;
    cube.isActive = true;

#if PHYSX_ENABLED
    // PhysX implementation would go here when PhysX is available
#else
    // Simple physics fallback
    cube.simplePhysics.position = spawnPos;
    cube.simplePhysics.velocity = cameraFront * CUBE_INITIAL_SPEED;
    cube.simplePhysics.isActive = true;
#endif

    cubes.push_back(cube);
}

void updateSimplePhysics(float deltaTime) {
    const float floorY = -0.5f;
    const float bounceRestitution = 0.3f;
    
    // Update bullets
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->timeAlive += deltaTime;
        
        if (it->timeAlive >= it->lifetime) {
            it = bullets.erase(it);
            continue;
        }

        // Apply gravity
        it->simplePhysics.velocity.y += gravity * deltaTime;
        
        // Update position
        it->simplePhysics.position += it->simplePhysics.velocity * deltaTime;
        it->position = it->simplePhysics.position;
        
        // Floor collision
        if (it->position.y < floorY) {
            it->position.y = floorY;
            it->simplePhysics.position.y = floorY;
            it->simplePhysics.velocity.y *= -bounceRestitution;
        }
        
        ++it;
    }
    
    // Update cubes
    for (auto& cube : cubes) {
        if (cube.isActive) {
            // Apply gravity
            cube.simplePhysics.velocity.y += gravity * deltaTime;
            
            // Update position
            cube.simplePhysics.position += cube.simplePhysics.velocity * deltaTime;
            cube.position = cube.simplePhysics.position;
            
            // Floor collision
            if (cube.position.y < floorY + CUBE_SIZE/2) {
                cube.position.y = floorY + CUBE_SIZE/2;
                cube.simplePhysics.position.y = cube.position.y;
                cube.simplePhysics.velocity.y *= -bounceRestitution;
                
                // Add some friction
                cube.simplePhysics.velocity.x *= 0.95f;
                cube.simplePhysics.velocity.z *= 0.95f;
            }
        }
    }

    // Update main cube (simple falling)
    static float mainCubeVelocity = 0.0f;
    mainCubeVelocity += gravity * deltaTime;
    cubePos.y += mainCubeVelocity * deltaTime;
    
    if (cubePos.y < floorY + CUBE_SIZE/2) {
        cubePos.y = floorY + CUBE_SIZE/2;
        mainCubeVelocity *= -bounceRestitution;
    }
}

void updatePhysics(float deltaTime) {
#if PHYSX_ENABLED
    // PhysX physics update would go here when PhysX is available
#else
    // Use simple physics fallback
    updateSimplePhysics(deltaTime);
#endif
}

void initRaytracing() {
    // Create raytracing texture
    glGenTextures(1, &raytracingTexture);
    glBindTexture(GL_TEXTURE_2D, raytracingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, 0x8814, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL); // GL_RGBA32F = 0x8814
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Bind as image texture for compute shader
    glBindImageTexture(0, raytracingTexture, 0, GL_FALSE, 0, 0x8CA3, 0x8814); // GL_READ_WRITE = 0x8CA3, GL_RGBA32F = 0x8814
}

void createFullscreenQuad() {
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int quadVBO;
    glGenVertexArrays(1, &fullscreenVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(fullscreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

RTMaterial convertToRTMaterial(const Material& mat, int type) {
    RTMaterial rtMat;
    rtMat.albedo = mat.diffuse;
    rtMat.specular = mat.specular;
    rtMat.shininess = mat.shininess;
    rtMat.metallic = (type == 1) ? 0.8f : 0.0f;
    rtMat.roughness = 1.0f / sqrt(mat.shininess);
    rtMat.ior = 1.5f;
    rtMat.type = type;
    return rtMat;
}

void updateRaytracingScene() {
    updatePhysics(deltaTime);
    
    // Update RT spheres based on current scene
    rtSpheres.clear();
    
    // Add main sphere
    RTSphere mainSphere;
    mainSphere.center = cubePos;
    mainSphere.radius = 0.5f;
    
    // Determine material type based on current material
    int materialType = 0; // Default to diffuse
    if (materials[currentMaterialIndex].name.find("Gold") != std::string::npos ||
        materials[currentMaterialIndex].name.find("Silver") != std::string::npos ||
        materials[currentMaterialIndex].name.find("Copper") != std::string::npos ||
        materials[currentMaterialIndex].name.find("Bronze") != std::string::npos) {
        materialType = 1; // Metal
    }
    if (materials[currentMaterialIndex].name.find("Emerald") != std::string::npos) {
        materialType = 2; // Glass
    }
    
    mainSphere.material = convertToRTMaterial(materials[currentMaterialIndex], materialType);
    rtSpheres.push_back(mainSphere);
    
    // Add spawned cubes
    for (const auto& cube : cubes) {
        if (cube.isActive) {
            RTSphere sphere;
            sphere.center = cube.position;
            sphere.radius = CUBE_SIZE * 0.5f;
            sphere.material = convertToRTMaterial(materials[2], 0); // Green diffuse
            sphere.material.albedo = glm::vec3(0.3f, 0.8f, 0.3f);
            rtSpheres.push_back(sphere);
        }
    }
    
    // Add bullets
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            RTSphere sphere;
            sphere.center = bullet.position;
            sphere.radius = bulletRadius;
            sphere.material = convertToRTMaterial(materials[1], 1); // Gold metal
            sphere.material.albedo = glm::vec3(1.0f, 1.0f, 0.0f);
            rtSpheres.push_back(sphere);
        }
    }
}

void renderRaytraced() {
    // Use compute shader for raytracing
    glUseProgram(computeShader);
    
    // Set camera uniforms
    glUniform3f(glGetUniformLocation(computeShader, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(glGetUniformLocation(computeShader, "cameraFront"), cameraFront.x, cameraFront.y, cameraFront.z);
    glUniform3f(glGetUniformLocation(computeShader, "cameraUp"), cameraUp.x, cameraUp.y, cameraUp.z);
    glUniform3f(glGetUniformLocation(computeShader, "cameraRight"), cameraRight.x, cameraRight.y, cameraRight.z);
    glUniform1f(glGetUniformLocation(computeShader, "fov"), glm::radians(45.0f));
    glUniform1f(glGetUniformLocation(computeShader, "aspectRatio"), (float)SCR_WIDTH / (float)SCR_HEIGHT);
    glUniform1i(glGetUniformLocation(computeShader, "maxBounces"), maxBounces);
    glUniform1i(glGetUniformLocation(computeShader, "numSamples"), numSamples);
    glUniform3f(glGetUniformLocation(computeShader, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(computeShader, "lightColor"), lightColor.x, lightColor.y, lightColor.z);
    glUniform1f(glGetUniformLocation(computeShader, "time"), static_cast<float>(glfwGetTime()));
    
    // Set scene uniforms
    glUniform1i(glGetUniformLocation(computeShader, "numSpheres"), static_cast<int>(rtSpheres.size()));
    
    // Upload sphere data (simplified - in a real implementation you'd use uniform buffers)
    for (int i = 0; i < std::min(static_cast<int>(rtSpheres.size()), 20); i++) {
        std::string prefix = "spheres[" + std::to_string(i) + "]";
        glUniform3f(glGetUniformLocation(computeShader, (prefix + ".center").c_str()), 
                   rtSpheres[i].center.x, rtSpheres[i].center.y, rtSpheres[i].center.z);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".radius").c_str()), 
                   rtSpheres[i].radius);
        glUniform3f(glGetUniformLocation(computeShader, (prefix + ".material.albedo").c_str()), 
                   rtSpheres[i].material.albedo.x, rtSpheres[i].material.albedo.y, rtSpheres[i].material.albedo.z);
        glUniform3f(glGetUniformLocation(computeShader, (prefix + ".material.specular").c_str()), 
                   rtSpheres[i].material.specular.x, rtSpheres[i].material.specular.y, rtSpheres[i].material.specular.z);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".material.shininess").c_str()), 
                   rtSpheres[i].material.shininess);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".material.metallic").c_str()), 
                   rtSpheres[i].material.metallic);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".material.roughness").c_str()), 
                   rtSpheres[i].material.roughness);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".material.ior").c_str()), 
                   rtSpheres[i].material.ior);
        glUniform1i(glGetUniformLocation(computeShader, (prefix + ".material.type").c_str()), 
                   rtSpheres[i].material.type);
    }
    
    // Set floor uniforms
    glUniform3f(glGetUniformLocation(computeShader, "floorNormal"), 0.0f, 1.0f, 0.0f);
    glUniform1f(glGetUniformLocation(computeShader, "floorDistance"), 0.5f);
    
    RTMaterial floorMat;
    floorMat.albedo = glm::vec3(0.3f, 0.3f, 0.3f);
    floorMat.specular = glm::vec3(0.2f, 0.2f, 0.2f);
    floorMat.shininess = 16.0f;
    floorMat.metallic = 0.0f;
    floorMat.roughness = 0.8f;
    floorMat.ior = 1.0f;
    floorMat.type = 0;
    
    glUniform3f(glGetUniformLocation(computeShader, "floorMaterial.albedo"), 
               floorMat.albedo.x, floorMat.albedo.y, floorMat.albedo.z);
    glUniform3f(glGetUniformLocation(computeShader, "floorMaterial.specular"), 
               floorMat.specular.x, floorMat.specular.y, floorMat.specular.z);
    glUniform1f(glGetUniformLocation(computeShader, "floorMaterial.shininess"), floorMat.shininess);
    glUniform1f(glGetUniformLocation(computeShader, "floorMaterial.metallic"), floorMat.metallic);
    glUniform1f(glGetUniformLocation(computeShader, "floorMaterial.roughness"), floorMat.roughness);
    glUniform1f(glGetUniformLocation(computeShader, "floorMaterial.ior"), floorMat.ior);
    glUniform1i(glGetUniformLocation(computeShader, "floorMaterial.type"), floorMat.type);
    
    // Dispatch compute shader
    glDispatchCompute((SCR_WIDTH + 15) / 16, (SCR_HEIGHT + 15) / 16, 1);
    glMemoryBarrier(0x00000008); // GL_SHADER_IMAGE_ACCESS_BARRIER_BIT = 0x00000008
    
    // Render fullscreen quad with the raytraced result
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(fullscreenShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, raytracingTexture);
    glUniform1i(glGetUniformLocation(fullscreenShader, "screenTexture"), 0);
    glUniform1f(glGetUniformLocation(fullscreenShader, "exposure"), exposure);
    glUniform1i(glGetUniformLocation(fullscreenShader, "enableToneMapping"), enableToneMapping);
    
    glBindVertexArray(fullscreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}