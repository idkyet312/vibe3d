#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, float deltaTime);
GLuint loadShaders(const char * vertex_file_path, const char * fragment_file_path);
void createFloorMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void createSphereMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int segments);
void shootBullet();
void spawnCube();
void updatePhysics(float deltaTime);

// Settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Basic Camera
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float cameraSpeed = 2.5f; // adjust accordingly

// Add these variables at the top with other variables
float gravity = -9.8f;
float jumpForce = 5.0f;
float verticalVelocity = 0.0f;
bool isGrounded = true;
float lastX = 400.0f;
float lastY = 300.0f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float mouseSensitivity = 0.1f;

// Bullet system
struct Bullet {
    glm::vec3 position;
    glm::vec3 direction;
    float speed = 20.0f;
    float lifetime = 3.0f; // seconds
    float timeAlive = 0.0f;
    bool active = false;
};

std::vector<Bullet> bullets;
float bulletRadius = 0.1f;
float lastShotTime = 0.0f;
float shootCooldown = 0.5f; // seconds between shots

// Physics-related structures
struct PhysicsObject {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 rotation;
    glm::vec3 angularVelocity;
    float mass;
    bool isActive;
    float size;  // Add size parameter for collision detection
};

// Physics constants
const float GRAVITY = -9.81f;
const float FRICTION = 0.98f;
const float RESTITUTION = 0.5f;
const float FLOOR_Y = 0.0f;  // Update to match floor mesh
const float CUBE_SIZE = 0.3f;
const float FLOOR_FRICTION = 0.95f;
const int MAX_CUBES = 10;  // Define MAX_CUBES constant

// Physics objects
std::vector<PhysicsObject> physicsObjects;

// Add these variables with other timing variables
float lastCubeSpawnTime = 0.0f;
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

    // Render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        // -------------------- 
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        // -----
        processInput(window, deltaTime); // Pass deltaTime here

        // Update bullets
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;
            
            bullet.position += bullet.direction * bullet.speed * deltaTime;
            bullet.timeAlive += deltaTime;
            
            if (bullet.timeAlive >= bullet.lifetime) {
                bullet.active = false;
            }
        }

        // Update physics
        updatePhysics(deltaTime);

        // Render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Also clear the depth buffer!

        // Activate shader
        glUseProgram(shaderProgram);

        // Create transformations
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);
        // You can add transformations to the model matrix here (e.g., rotation, translation)
        // model = glm::rotate(model, (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
        
        // Pass matrices to the shader
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // In the render loop, update the viewPos uniform:
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

        // Draw the cube
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Draw bullets
        for (const auto& bullet : bullets) {
            if (!bullet.active) continue;

            glm::mat4 bulletModel = glm::mat4(1.0f);
            bulletModel = glm::translate(bulletModel, bullet.position);
            bulletModel = glm::scale(bulletModel, glm::vec3(bulletRadius));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(bulletModel));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Draw physics objects
        for (const auto& obj : physicsObjects) {
            if (!obj.isActive) continue;
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, obj.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, obj.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, obj.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(obj.size)); // Scale the cube based on its size
            
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.8f, 0.2f, 0.2f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // In the render loop, before the cube rendering, add floor rendering
        glUseProgram(floorShaderProgram);
        glm::mat4 floorModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "model"), 1, GL_FALSE, &floorModel[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

        glBindVertexArray(floorVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Switch back to the main shader for the cube
        glUseProgram(shaderProgram);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

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

void spawnCube() {
    float currentTime = glfwGetTime();
    if (currentTime - lastCubeSpawnTime < cubeSpawnCooldown) return;

    // Find an inactive cube or create a new one
    auto it = std::find_if(physicsObjects.begin(), physicsObjects.end(),
                          [](const PhysicsObject& obj) { return !obj.isActive; });
    
    if (it != physicsObjects.end()) {
        // Reuse inactive cube
        it->isActive = true;
        it->position = cameraPos + cameraFront * 2.0f;  // Spawn 2 units in front of camera
        it->velocity = cameraFront * 5.0f;  // Initial velocity in camera direction
        it->acceleration = glm::vec3(0.0f, -9.8f, 0.0f);  // Gravity
        it->rotation = glm::vec3(0.0f);  // No rotation
        it->angularVelocity = glm::vec3(0.0f);  // No angular velocity
        it->size = 0.5f;  // Cube size
    } else if (physicsObjects.size() < MAX_CUBES) {
        // Create new cube
        PhysicsObject newCube;
        newCube.isActive = true;
        newCube.position = cameraPos + cameraFront * 2.0f;  // Spawn 2 units in front of camera
        newCube.velocity = cameraFront * 5.0f;  // Initial velocity in camera direction
        newCube.acceleration = glm::vec3(0.0f, -9.8f, 0.0f);  // Gravity
        newCube.rotation = glm::vec3(0.0f);  // No rotation
        newCube.angularVelocity = glm::vec3(0.0f);  // No angular velocity
        newCube.size = 0.5f;  // Cube size
        physicsObjects.push_back(newCube);
    }

    lastCubeSpawnTime = currentTime;
}

void updatePhysics(float deltaTime) {
    for (auto& obj : physicsObjects) {
        if (!obj.isActive) continue;

        // Update velocity and position
        obj.velocity += obj.acceleration * deltaTime;
        obj.position += obj.velocity * deltaTime;

        // Floor collision with proper response
        if (obj.position.y < FLOOR_Y + obj.size * 0.5f) {
            // Reset position to floor level
            obj.position.y = FLOOR_Y + obj.size * 0.5f;
            
            // Only bounce if moving downward significantly
            if (obj.velocity.y < -0.1f) {
                obj.velocity.y = -obj.velocity.y * RESTITUTION;
                
                // Add rotation based on impact
                float impactForce = glm::length(obj.velocity);
                glm::vec3 impactPoint = obj.position + glm::vec3(0.0f, -obj.size * 0.5f, 0.0f);
                glm::vec3 r = impactPoint - obj.position;
                glm::vec3 torque = glm::cross(r, glm::vec3(0.0f, -impactForce, 0.0f));
                obj.angularVelocity += torque * 0.1f;
            } else {
                obj.velocity.y = 0.0f;  // Stop vertical movement if nearly stopped
            }

            // Apply floor friction to horizontal movement
            obj.velocity.x *= FLOOR_FRICTION;
            obj.velocity.z *= FLOOR_FRICTION;
            
            // Dampen angular velocity when on floor
            obj.angularVelocity *= FLOOR_FRICTION;
        }

        // Update rotation based on velocity and angular velocity
        if (glm::length(obj.velocity) > 0.1f) {
            // Rolling effect based on horizontal movement
            obj.angularVelocity.x = obj.velocity.z * 2.0f;
            obj.angularVelocity.z = -obj.velocity.x * 2.0f;
        }
        
        // Update rotation
        obj.rotation += obj.angularVelocity * deltaTime;

        // Apply air friction
        obj.velocity *= FRICTION;
        obj.angularVelocity *= FRICTION;

        // Limit maximum velocity to prevent extreme speeds
        const float MAX_VELOCITY = 20.0f;
        if (glm::length(obj.velocity) > MAX_VELOCITY) {
            obj.velocity = glm::normalize(obj.velocity) * MAX_VELOCITY;
        }

        // Limit maximum angular velocity
        const float MAX_ANGULAR_VELOCITY = 10.0f;
        if (glm::length(obj.angularVelocity) > MAX_ANGULAR_VELOCITY) {
            obj.angularVelocity = glm::normalize(obj.angularVelocity) * MAX_ANGULAR_VELOCITY;
        }
    }

    // Check cube-to-cube collisions
    for (size_t i = 0; i < physicsObjects.size(); i++) {
        for (size_t j = i + 1; j < physicsObjects.size(); j++) {
            if (!physicsObjects[i].isActive || !physicsObjects[j].isActive) continue;
            
            if (checkCubeCollision(physicsObjects[i], physicsObjects[j])) {
                resolveCollision(physicsObjects[i], physicsObjects[j]);
            }
        }
    }
} 