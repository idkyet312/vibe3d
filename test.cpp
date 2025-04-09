#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <PxPhysicsAPI.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

using namespace physx;

// Camera variables
glm::vec3 cameraPos   = glm::vec3(0.0f, 2.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float cameraSpeed = 2.5f;
float lastX = 400.0f;
float lastY = 300.0f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float mouseSensitivity = 0.1f;

// Physics variables
float gravity = -9.81f;
float jumpForce = 5.0f;
float verticalVelocity = 0.0f;
bool isGrounded = true;

// Bullet system
struct Bullet {
    glm::vec3 position;
    glm::vec3 direction;
    float speed = 20.0f;
    float lifetime = 3.0f;
    float timeAlive = 0.0f;
    bool active = false;
};

std::vector<Bullet> bullets;
float bulletRadius = 0.1f;
float lastShotTime = 0.0f;
float shootCooldown = 0.5f;

// PhysX global variables
PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;
PxFoundation* gFoundation = nullptr;
PxPhysics* gPhysics = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
PxScene* gScene = nullptr;
PxMaterial* gMaterial = nullptr;
PxPvd* gPvd = nullptr;

// Cube structure and variables
struct Cube {
    glm::vec3 position;
    PxRigidDynamic* actor;
    bool isActive;
};

std::vector<Cube> cubes;
const int MAX_CUBES = 50;
const float CUBE_MASS = 1.0f;  // 1 kg
const float CUBE_SIZE = 0.5f;  // 0.5 meters
const float CUBE_INITIAL_SPEED = 5.0f;  // 5 m/s
float lastCubeSpawnTime = 0.0f;
const float CUBE_SPAWN_COOLDOWN = 1.0f;  // 1 second between spawns

// Settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Global variables
GLFWwindow* window;
unsigned int shaderProgram;
unsigned int floorShaderProgram;
unsigned int VAO, VBO, floorVAO, floorVBO, floorEBO;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float fov = 45.0f;
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
glm::vec3 cubePos(0.0f, 0.0f, 0.0f);
PxRigidDynamic* dynamicActor = nullptr;

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, float deltaTime);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void initPhysX();
void cleanupPhysX();
void updatePhysics(float deltaTime);
void spawnCube();
GLuint loadShaders(const char* vertex_file_path, const char* fragment_file_path);
void createFloorMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices);
void shootBullet();

// Physics-related structures
struct PhysicsObject {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 rotation;
    glm::vec3 angularVelocity;
    glm::vec3 angularAcceleration;
    float mass;
    float size;
    bool isActive;
    glm::mat3 inertiaTensor;
    
    PhysicsObject() : 
        position(0.0f), 
        velocity(0.0f), 
        acceleration(0.0f),
        rotation(0.0f),
        angularVelocity(0.0f),
        angularAcceleration(0.0f),
        mass(1.0f),
        size(1.0f),
        isActive(false) {
        // Initialize inertia tensor for a cube
        float s = size * size;
        inertiaTensor = glm::mat3(
            (mass * s) / 6.0f, 0.0f, 0.0f,
            0.0f, (mass * s) / 6.0f, 0.0f,
            0.0f, 0.0f, (mass * s) / 6.0f
        );
    }
};

// Physics constants
const float GRAVITY = -9.81f;  // Real gravity in m/s²
const float RESTITUTION = 0.3f;  // More realistic bounce (wood/plastic on concrete)
const float FRICTION = 0.99f;  // Air friction coefficient
const float FLOOR_FRICTION = 0.8f;  // Concrete-like friction
const float AIR_RESISTANCE = 0.01f;  // Realistic air resistance
const float FLOOR_Y = -1.0f;

// Physics objects
std::vector<PhysicsObject> physicsObjects;

// Add these variables with other timing variables
float cubeSpawnCooldown = 0.5f;  // Half second cooldown between spawns

bool checkCubeCollision(const PhysicsObject& cube1, const PhysicsObject& cube2) {
    // Get the half-size of each cube
    float halfSize1 = cube1.size * 0.5f;
    float halfSize2 = cube2.size * 0.5f;
    
    // Calculate the distance between cube centers
    glm::vec3 diff = cube1.position - cube2.position;
    
    // Check for overlap in each axis
    bool xOverlap = std::abs(diff.x) < (halfSize1 + halfSize2);
    bool yOverlap = std::abs(diff.y) < (halfSize1 + halfSize2);
    bool zOverlap = std::abs(diff.z) < (halfSize1 + halfSize2);
    
    // Collision occurs if there's overlap in all three axes
    return xOverlap && yOverlap && zOverlap;
}

void resolveCollision(PhysicsObject& cube1, PhysicsObject& cube2) {
    // Calculate collision normal (direction from cube2 to cube1)
    glm::vec3 normal = glm::normalize(cube1.position - cube2.position);
    
    // Calculate relative velocity
    glm::vec3 relativeVelocity = cube1.velocity - cube2.velocity;
    
    // Calculate relative velocity along the normal
    float velocityAlongNormal = glm::dot(relativeVelocity, normal);
    
    // Don't resolve if objects are moving apart
    if (velocityAlongNormal > 0) return;
    
    // Calculate restitution (bounciness)
    float restitution = 0.5f;
    
    // Calculate impulse scalar
    float j = -(1.0f + restitution) * velocityAlongNormal;
    j /= 2.0f; // Assuming equal mass for simplicity
    
    // Apply impulse
    glm::vec3 impulse = j * normal;
    cube1.velocity += impulse;
    cube2.velocity -= impulse;
    
    // Add angular velocity based on collision
    glm::vec3 r1 = cube1.position - (cube1.position + cube2.position) * 0.5f;
    glm::vec3 r2 = cube2.position - (cube1.position + cube2.position) * 0.5f;
    
    // Calculate torque
    glm::vec3 torque1 = glm::cross(r1, impulse);
    glm::vec3 torque2 = glm::cross(r2, -impulse);
    
    // Apply angular velocity (scaled by inverse mass)
    cube1.angularVelocity += torque1 * 0.1f;
    cube2.angularVelocity += torque2 * 0.1f;
    
    // Separate the cubes to prevent sticking
    float overlap = (cube1.size + cube2.size) * 0.5f - glm::length(cube1.position - cube2.position);
    glm::vec3 separation = normal * overlap * 0.5f;
    cube1.position += separation;
    cube2.position -= separation;
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vibe3D Game", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST); // Enable depth testing for 3D

    // Build and compile our shader program
    // ------------------------------------
    // Note: CMakeLists.txt should copy shaders to build directory
    GLuint shaderProgram = loadShaders("vertex.glsl", "fragment.glsl"); 
    if (shaderProgram == 0) {
         glfwTerminate();
         return -1; // Shader loading failed
    }

    // Add this after the other shader loading
    GLuint floorShaderProgram = loadShaders("floor_vertex.glsl", "floor_fragment.glsl");

    // Set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coord attribute (or color, etc.) - location 2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Unbind VBO and VAO 
    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 

    // Timing
    float deltaTime = 0.0f; // Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame

    // In main(), after creating the cube mesh, add the floor mesh
    std::vector<float> floorVertices;
    std::vector<unsigned int> floorIndices;
    createFloorMesh(floorVertices, floorIndices);

    // Create and bind floor VAO/VBO/EBO
    unsigned int floorVAO, floorVBO, floorEBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);

    glBindVertexArray(floorVAO);

    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, floorVertices.size() * sizeof(float), floorVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, floorIndices.size() * sizeof(unsigned int), floorIndices.data(), GL_STATIC_DRAW);

    // Set up vertex attributes for floor
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float))); // Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float))); // Normal
    glEnableVertexAttribArray(2);

    // In main(), after creating the window, add:
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    // After creating the shader program, add:
    glUseProgram(shaderProgram);
    // Set lighting uniforms
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 2.0f, 2.0f, 2.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.5f, 0.5f, 0.5f);

    // Initialize PhysX
    initPhysX();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window, deltaTime);

        // Clear the screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update physics
        updatePhysics(deltaTime);

        // Update cube position from physics
        if (dynamicActor) {
            PxTransform transform = dynamicActor->getGlobalPose();
            cubePos = glm::vec3(transform.p.x, transform.p.y, transform.p.z);
        }

        // Spawn new cubes
        if (glfwGetTime() - lastCubeSpawnTime >= CUBE_SPAWN_COOLDOWN) {
            spawnCube();
        }

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

        // Draw the cube
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), lightColor.x, lightColor.y, lightColor.z);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePos);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.8f, 0.3f, 0.3f);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Draw bullets
        for (const auto& bullet : bullets) {
            if (bullet.active) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, bullet.position);
                model = glm::scale(model, glm::vec3(bulletRadius));
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
                glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 0.0f);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    cleanupPhysX();

    // Optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(floorShaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera movement
    float cameraSpeed = 2.5f * deltaTime;
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

    // Apply gravity
    verticalVelocity += gravity * deltaTime;
    cameraPos.y += verticalVelocity * deltaTime;

    // Ground collision
    if (cameraPos.y <= 1.0f) {  // 1.0f is the height of the camera
        cameraPos.y = 1.0f;
        verticalVelocity = 0.0f;
        isGrounded = true;
    }

    // Physics-related input
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        spawnCube();
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// Add this function before main()
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
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

// Utility function for loading shaders
// Reads shader files, compiles them, links them into a shader program.
GLuint loadShaders(const char * vertex_file_path,const char * fragment_file_path){

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open()){
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}else{
		std::cerr << "Impossible to open " << vertex_file_path << ". Check path relative to executable." << std::endl;
		getchar(); // Keep console window open
        return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	} else {
        std::cerr << "Impossible to open " << fragment_file_path << ". Check path relative to executable." << std::endl;
		getchar(); // Keep console window open
        return 0;
    }

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		std::cerr << "Vertex Shader Error: " << &VertexShaderErrorMessage[0] << std::endl;
        glDeleteShader(VertexShaderID); // Don't leak the shader.
        glDeleteShader(FragmentShaderID);
		return 0;
	}

	// Compile Fragment Shader
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		std::cerr << "Fragment Shader Error: " << &FragmentShaderErrorMessage[0] << std::endl;
        glDeleteShader(VertexShaderID); // Don't leak the shader.
		glDeleteShader(FragmentShaderID); // Don't leak the shader.
		return 0;
	}

	// Link the program
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		std::cerr << "Linking Error: " << &ProgramErrorMessage[0] << std::endl;
        glDeleteShader(VertexShaderID); // Don't leak the shaders.
	    glDeleteShader(FragmentShaderID);
        glDeleteProgram(ProgramID); // Don't leak the program.
		return 0;
	}

	// Detach and delete the shaders as they are linked into our program now and no longer necessary
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

// Add this function before main()
void createFloorMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    // Floor vertices (grey color)
    vertices = {
        // positions         // colors (grey)      // normals
        -10.0f, 0.0f, -10.0f,  0.3f, 0.3f, 0.3f,  0.0f, 1.0f, 0.0f,  // bottom left
         10.0f, 0.0f, -10.0f,  0.3f, 0.3f, 0.3f,  0.0f, 1.0f, 0.0f,  // bottom right
         10.0f, 0.0f,  10.0f,  0.3f, 0.3f, 0.3f,  0.0f, 1.0f, 0.0f,  // top right
        -10.0f, 0.0f,  10.0f,  0.3f, 0.3f, 0.3f,  0.0f, 1.0f, 0.0f   // top left
    };

    // Floor indices
    indices = {
        0, 1, 2,  // first triangle
        2, 3, 0   // second triangle
    };
}

// Remove lighting variables and update the createSphereMesh function
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
            // Color (bright white)
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
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
    if (currentTime - lastShotTime < shootCooldown) return;

    Bullet bullet;
    bullet.position = cameraPos;
    bullet.direction = cameraFront;
    bullet.active = true;
    bullet.timeAlive = 0.0f;
    bullets.push_back(bullet);
    lastShotTime = currentTime;
}

void initPhysX() {
    // Create foundation
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!gFoundation) {
        std::cerr << "PxCreateFoundation failed!" << std::endl;
        return;
    }

    // Create physics
    bool recordMemoryAllocations = true;
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), recordMemoryAllocations);
    if (!gPhysics) {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return;
    }

    // Create scene
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(sceneDesc);

    // Create material
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);  // Static friction, dynamic friction, restitution

    // Create ground plane
    PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    gScene->addActor(*groundPlane);

    // Create initial cube
    PxShape* shape = gPhysics->createShape(PxBoxGeometry(0.5f, 0.5f, 0.5f), *gMaterial);
    PxTransform transform(PxVec3(0.0f, 5.0f, 0.0f));
    dynamicActor = gPhysics->createRigidDynamic(transform);
    dynamicActor->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*dynamicActor, 10.0f);
    gScene->addActor(*dynamicActor);
    shape->release();
}

void cleanupPhysX() {
    // Release PhysX objects in reverse order
    for (auto& cube : cubes) {
        if (cube.actor) {
            cube.actor->release();
        }
    }
    cubes.clear();

    if (gScene) gScene->release();
    if (gDispatcher) gDispatcher->release();
    if (gMaterial) gMaterial->release();
    if (gPhysics) gPhysics->release();
    if (gFoundation) gFoundation->release();
}

void spawnCube() {
    float currentTime = glfwGetTime();
    if (currentTime - lastCubeSpawnTime < CUBE_SPAWN_COOLDOWN) return;
    lastCubeSpawnTime = currentTime;

    // Create new cube at camera position
    PxShape* shape = gPhysics->createShape(PxBoxGeometry(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE), *gMaterial);
    PxTransform transform(PxVec3(cameraPos.x, cameraPos.y, cameraPos.z));
    PxRigidDynamic* body = gPhysics->createRigidDynamic(transform);
    body->attachShape(*shape);
    shape->release();

    // Set mass and inertia
    PxRigidBodyExt::updateMassAndInertia(*body, CUBE_MASS);
    
    // Set initial velocity in camera direction
    PxVec3 velocity(cameraFront.x, cameraFront.y, cameraFront.z);
    velocity.normalize();
    velocity *= CUBE_INITIAL_SPEED;
    body->setLinearVelocity(velocity);

    // Add to scene
    gScene->addActor(*body);

    // Add to cubes vector
    Cube cube;
    cube.position = cameraPos;
    cube.actor = body;
    cube.isActive = true;
    cubes.push_back(cube);

    // Remove oldest cube if we've exceeded the limit
    if (cubes.size() > MAX_CUBES) {
        if (cubes[0].actor) {
            cubes[0].actor->release();
        }
        cubes.erase(cubes.begin());
    }
}

void updatePhysics(float deltaTime) {
    if (!gScene) return;

    // Step PhysX simulation
    gScene->simulate(deltaTime);
    gScene->fetchResults(true);

    // Update cube positions from PhysX
    for (auto& cube : cubes) {
        if (cube.isActive && cube.actor) {
            PxTransform transform = cube.actor->getGlobalPose();
            cube.position = glm::vec3(transform.p.x, transform.p.y, transform.p.z);
            
            // Deactivate cubes that have fallen too far
            if (cube.position.y < -50.0f) {
                cube.isActive = false;
                cube.actor->setGlobalPose(PxTransform(PxVec3(0.0f, -100.0f, 0.0f))); // Move far away
            }
        }
    }
}