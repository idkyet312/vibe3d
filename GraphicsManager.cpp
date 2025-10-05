#include "GraphicsManager.h"
#include "MaterialSystem.h"
#include "PhysicsManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

GraphicsManager::GraphicsManager() 
    : sphereVAO(0), sphereVBO(0), sphereEBO(0)
    , floorVAO(0), floorVBO(0), floorEBO(0)
    , fullscreenVAO(0)
    , mainShaderProgram(0), floorShaderProgram(0)
    , computeShader(0), fullscreenShader(0)
    , raytracingTexture(0), raytracingSupported(false)
    , sphereIndexCount(0)
    , screenWidth(800), screenHeight(600)
    , currentLightPos(1.2f, 1.0f, 2.0f)
    , currentLightColor(1.0f, 1.0f, 1.0f)
    , pglDispatchCompute(nullptr)
    , pglBindImageTexture(nullptr)
    , pglMemoryBarrier(nullptr)
    , fpsShaderProgram(0)
    , fpsVAO(0), fpsVBO(0)
    , fpsDisplayInitialized(false)
    , depthPrepassShader(0)
    , lightCullingComputeShader(0)
    , tiledForwardShader(0)
    , depthTexture(0)
    , lightListBuffer(0)
    , visibleLightIndicesBuffer(0)
    , lightDataBuffer(0)
    , forwardPlusSupported(false)
    , numTilesX(0), numTilesY(0)
    , maxLightsPerTile(1024)
    , useVulkanRenderer(false)
{
}

GraphicsManager::~GraphicsManager() {
    cleanup();
}

bool GraphicsManager::initialize(unsigned int width, unsigned int height) {
    screenWidth = width;
    screenHeight = height;
    
    // Check for compute shader support
    raytracingSupported = checkComputeShaderSupport();
    
    // Load compute shader functions if supported
    if (raytracingSupported) {
        raytracingSupported = loadComputeShaderFunctions();
        if (!raytracingSupported) {
            std::cerr << "Failed to load compute shader functions" << std::endl;
        }
    }
    
    // Load shaders
    mainShaderProgram = loadShaders("vertex.glsl", "fragment.glsl");
    if (mainShaderProgram == 0) {
        std::cerr << "Failed to load main shaders" << std::endl;
        return false;
    }
    
    floorShaderProgram = loadShaders("floor_vertex.glsl", "floor_fragment.glsl");
    if (floorShaderProgram == 0) {
        std::cerr << "Failed to load floor shaders" << std::endl;
        return false;
    }
    
    if (raytracingSupported) {
        computeShader = loadComputeShader("raytracing.comp");
        if (computeShader == 0) {
            std::cerr << "Failed to load compute shader, disabling raytracing" << std::endl;
            raytracingSupported = false;
        }
        
        fullscreenShader = loadShaders("fullscreen_vertex.glsl", "fullscreen_fragment.glsl");
        if (fullscreenShader == 0) {
            std::cerr << "Failed to load fullscreen shader, disabling raytracing" << std::endl;
            raytracingSupported = false;
        }
        
        if (raytracingSupported) {
            if (!initRaytracing()) {
                raytracingSupported = false;
            } else {
                createFullscreenQuad();
                std::cout << "Raytracing initialized successfully!" << std::endl;
            }
        }
    }
    
    // Create sphere mesh
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    createSphereMesh(sphereVertices, sphereIndices, 0.5f, 32);
    setupSphereBuffers(sphereVertices, sphereIndices);
    
    // Create floor mesh
    std::vector<float> floorVertices;
    std::vector<unsigned int> floorIndices;
    createFloorMesh(floorVertices, floorIndices);
    setupFloorBuffers(floorVertices, floorIndices);
    
    // Initialize FPS display
    initFPSDisplay();
    
    // Initialize Forward+ rendering if supported
    if (raytracingSupported) { // Forward+ requires compute shader support
        forwardPlusSupported = false; // Temporarily disable Forward+ to test basic rendering
        /*
        forwardPlusSupported = initForwardPlus();
        if (forwardPlusSupported) {
            std::cout << "Forward+ (Tiled Forward) rendering initialized successfully!" << std::endl;
        } else {
            std::cout << "Forward+ rendering initialization failed, using traditional forward rendering" << std::endl;
        }
        */
        std::cout << "Forward+ temporarily disabled - using traditional forward rendering" << std::endl;
    }
    
    return true;
}

void GraphicsManager::cleanup() {
    if (sphereVAO) {
        glDeleteVertexArrays(1, &sphereVAO);
        glDeleteBuffers(1, &sphereVBO);
        glDeleteBuffers(1, &sphereEBO);
    }
    
    if (floorVAO) {
        glDeleteVertexArrays(1, &floorVAO);
        glDeleteBuffers(1, &floorVBO);
        glDeleteBuffers(1, &floorEBO);
    }
    
    if (fullscreenVAO) {
        glDeleteVertexArrays(1, &fullscreenVAO);
    }
    
    if (raytracingTexture) {
        glDeleteTextures(1, &raytracingTexture);
    }
    
    if (mainShaderProgram) glDeleteProgram(mainShaderProgram);
    if (floorShaderProgram) glDeleteProgram(floorShaderProgram);
    if (computeShader) glDeleteProgram(computeShader);
    if (fullscreenShader) glDeleteProgram(fullscreenShader);
    if (fpsShaderProgram) glDeleteProgram(fpsShaderProgram);
    
    if (fpsVAO) {
        glDeleteVertexArrays(1, &fpsVAO);
        glDeleteBuffers(1, &fpsVBO);
    }
    
    // Cleanup Forward+ resources
    cleanupForwardPlus();
}

GLuint GraphicsManager::loadShaders(const char* vertex_file_path, const char* fragment_file_path) {
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << VertexShaderStream.rdbuf();
        VertexShaderCode = sstr.str();
        VertexShaderStream.close();
    } else {
        std::cerr << "Impossible to open " << vertex_file_path << std::endl;
        return 0;
    }

    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open()) {
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
    char const* VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        std::cerr << "Vertex Shader Error: " << &VertexShaderErrorMessage[0] << std::endl;
        return 0;
    }

    // Compile Fragment Shader
    char const* FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
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
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
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

GLuint GraphicsManager::loadComputeShader(const char* compute_file_path) {
    // Check if compute shaders are supported first
    int major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    if (major < 4 || (major == 4 && minor < 3)) {
        std::cerr << "Compute shaders require OpenGL 4.3+, current version: " << major << "." << minor << std::endl;
        return 0;
    }

    GLuint ComputeShaderID = glCreateShader(0x91B9); // GL_COMPUTE_SHADER = 0x91B9

    std::string ComputeShaderCode;
    std::ifstream ComputeShaderStream(compute_file_path, std::ios::in);
    if (ComputeShaderStream.is_open()) {
        std::stringstream sstr;
        sstr << ComputeShaderStream.rdbuf();
        ComputeShaderCode = sstr.str();
        ComputeShaderStream.close();
    } else {
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
    if (InfoLogLength > 0) {
        std::vector<char> ComputeShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(ComputeShaderID, InfoLogLength, NULL, &ComputeShaderErrorMessage[0]);
        std::cerr << "Compute Shader Error: " << &ComputeShaderErrorMessage[0] << std::endl;
        return 0;
    }

    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, ComputeShaderID);
    glLinkProgram(ProgramID);

    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        std::cerr << "Compute Shader Linking Error: " << &ProgramErrorMessage[0] << std::endl;
        return 0;
    }

    glDetachShader(ProgramID, ComputeShaderID);
    glDeleteShader(ComputeShaderID);

    return ProgramID;
}

void GraphicsManager::createSphereMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int segments) {
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

void GraphicsManager::createFloorMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices) {
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

void GraphicsManager::beginFrame() {
    // Clear framebuffer for forward rendering
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing for proper forward rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable alpha blending for transparent objects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GraphicsManager::endFrame() {
    // Nothing specific needed here for now
}

void GraphicsManager::renderSphere(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, 
                                  const Material& material, bool useEnhancedFeatures) {
    // Bind main shader program (optimized - only bind if different)
    static GLuint lastProgram = 0;
    if (lastProgram != mainShaderProgram) {
        glUseProgram(mainShaderProgram);
        lastProgram = mainShaderProgram;
    }
    
    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Set lighting (cache to avoid redundant calls)
    static glm::vec3 lastLightPos(-999.0f);
    static glm::vec3 lastLightColor(-999.0f);
    if (lastLightPos != currentLightPos || lastLightColor != currentLightColor) {
        glUniform3f(glGetUniformLocation(mainShaderProgram, "lightPos"), currentLightPos.x, currentLightPos.y, currentLightPos.z);
        glUniform3f(glGetUniformLocation(mainShaderProgram, "lightColor"), currentLightColor.x, currentLightColor.y, currentLightColor.z);
        lastLightPos = currentLightPos;
        lastLightColor = currentLightColor;
    }
    
    // Set material
    setMaterialUniforms(mainShaderProgram, material, useEnhancedFeatures);
    
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

void GraphicsManager::renderFloor(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection) {
    // Bind floor shader program (optimized - only bind if different)
    static GLuint lastProgram = 0;
    if (lastProgram != floorShaderProgram) {
        glUseProgram(floorShaderProgram);
        lastProgram = floorShaderProgram;
    }
    
    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Set lighting
    glUniform3f(glGetUniformLocation(floorShaderProgram, "lightPos"), currentLightPos.x, currentLightPos.y, currentLightPos.z);
    glUniform3f(glGetUniformLocation(floorShaderProgram, "lightColor"), currentLightColor.x, currentLightColor.y, currentLightColor.z);
    
    glBindVertexArray(floorVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void GraphicsManager::renderBullet(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(mainShaderProgram);
    
    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Set lighting
    glUniform3f(glGetUniformLocation(mainShaderProgram, "lightPos"), currentLightPos.x, currentLightPos.y, currentLightPos.z);
    glUniform3f(glGetUniformLocation(mainShaderProgram, "lightColor"), currentLightColor.x, currentLightColor.y, currentLightColor.z);
    
    // Set bullet properties
    glUniform3f(glGetUniformLocation(mainShaderProgram, "objectColor"), 1.0f, 1.0f, 0.0f);
    glUniform1i(glGetUniformLocation(mainShaderProgram, "shadingModel"), 1); // Blinn-Phong
    glUniform1i(glGetUniformLocation(mainShaderProgram, "useMaterial"), 0); // Disable material mode
    glUniform1i(glGetUniformLocation(mainShaderProgram, "enableReflections"), 1);
    glUniform1f(glGetUniformLocation(mainShaderProgram, "ambientOcclusion"), 0.0f);
    
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

void GraphicsManager::renderSpawned(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection) {
    glUseProgram(mainShaderProgram);
    
    // Set matrices
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Set lighting
    glUniform3f(glGetUniformLocation(mainShaderProgram, "lightPos"), currentLightPos.x, currentLightPos.y, currentLightPos.z);
    glUniform3f(glGetUniformLocation(mainShaderProgram, "lightColor"), currentLightColor.x, currentLightColor.y, currentLightColor.z);
    
    // Set spawned sphere properties
    glUniform3f(glGetUniformLocation(mainShaderProgram, "objectColor"), 0.3f, 0.8f, 0.3f);
    glUniform1i(glGetUniformLocation(mainShaderProgram, "shadingModel"), 0); // Lambert
    glUniform1i(glGetUniformLocation(mainShaderProgram, "useMaterial"), 0); // Disable material mode
    glUniform1i(glGetUniformLocation(mainShaderProgram, "enableReflections"), 0);
    glUniform1f(glGetUniformLocation(mainShaderProgram, "ambientOcclusion"), 0.2f);
    
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

bool GraphicsManager::initRaytracing() {
    // Create raytracing texture
    glGenTextures(1, &raytracingTexture);
    glBindTexture(GL_TEXTURE_2D, raytracingTexture);
    
    // Use GL_RGBA32F if available, otherwise fall back to GL_RGBA
    GLenum internalFormat = GL_RGBA32F;
    GLenum error = glGetError(); // Clear any existing errors
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    
    error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "Error creating raytracing texture: " << error << std::endl;
        // Try with a simpler format
        internalFormat = GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "Failed to create raytracing texture with fallback format: " << error << std::endl;
            return false;
        }
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Bind as image texture for compute shader
    if (pglBindImageTexture) {
        pglBindImageTexture(0, raytracingTexture, 0, GL_FALSE, 0, GL_READ_WRITE, internalFormat);
        error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "Error binding image texture: " << error << std::endl;
            return false;
        }
    } else {
        std::cerr << "glBindImageTexture function not available" << std::endl;
        return false;
    }
    
    std::cout << "Raytracing texture created successfully with format: " << internalFormat << std::endl;
    return true;
}

void GraphicsManager::createFullscreenQuad() {
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

void GraphicsManager::renderRaytraced(const std::vector<RTSphere>& spheres, const glm::vec3& cameraPos, 
                                     const glm::vec3& cameraFront, const glm::vec3& cameraUp, const glm::vec3& cameraRight,
                                     const glm::vec3& lightPos, const glm::vec3& lightColor, float time,
                                     int maxBounces, int numSamples, float exposure, bool enableToneMapping) {
    // Debug output
    static bool firstCall = true;
    if (firstCall) {
        std::cout << "First raytraced render call - Spheres: " << spheres.size() << std::endl;
        std::cout << "Camera pos: " << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << std::endl;
        std::cout << "Screen size: " << screenWidth << "x" << screenHeight << std::endl;
        firstCall = false;
    }
    
    // Clear any existing OpenGL errors
    while (glGetError() != GL_NO_ERROR);
    
    // Use compute shader for raytracing
    glUseProgram(computeShader);
    
    // Set camera uniforms
    glUniform3f(glGetUniformLocation(computeShader, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(glGetUniformLocation(computeShader, "cameraFront"), cameraFront.x, cameraFront.y, cameraFront.z);
    glUniform3f(glGetUniformLocation(computeShader, "cameraUp"), cameraUp.x, cameraUp.y, cameraUp.z);
    glUniform3f(glGetUniformLocation(computeShader, "cameraRight"), cameraRight.x, cameraRight.y, cameraRight.z);
    glUniform1f(glGetUniformLocation(computeShader, "fov"), glm::radians(45.0f));
    glUniform1f(glGetUniformLocation(computeShader, "aspectRatio"), (float)screenWidth / (float)screenHeight);
    glUniform1i(glGetUniformLocation(computeShader, "maxBounces"), maxBounces);
    glUniform1i(glGetUniformLocation(computeShader, "numSamples"), numSamples);
    glUniform3f(glGetUniformLocation(computeShader, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(computeShader, "lightColor"), lightColor.x, lightColor.y, lightColor.z);
    glUniform1f(glGetUniformLocation(computeShader, "time"), time);
    
    // Set scene uniforms
    glUniform1i(glGetUniformLocation(computeShader, "numSpheres"), static_cast<int>(spheres.size()));
    
    // Upload sphere data (simplified - in a real implementation you'd use uniform buffers)
    for (int i = 0; i < std::min(static_cast<int>(spheres.size()), 20); i++) {
        std::string prefix = "spheres[" + std::to_string(i) + "]";
        glUniform3f(glGetUniformLocation(computeShader, (prefix + ".center").c_str()), 
                   spheres[i].center.x, spheres[i].center.y, spheres[i].center.z);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".radius").c_str()), 
                   spheres[i].radius);
        glUniform3f(glGetUniformLocation(computeShader, (prefix + ".material.albedo").c_str()), 
                   spheres[i].material.albedo.x, spheres[i].material.albedo.y, spheres[i].material.albedo.z);
        glUniform3f(glGetUniformLocation(computeShader, (prefix + ".material.specular").c_str()), 
                   spheres[i].material.specular.x, spheres[i].material.specular.y, spheres[i].material.specular.z);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".material.shininess").c_str()), 
                   spheres[i].material.shininess);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".material.metallic").c_str()), 
                   spheres[i].material.metallic);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".material.roughness").c_str()), 
                   spheres[i].material.roughness);
        glUniform1f(glGetUniformLocation(computeShader, (prefix + ".material.ior").c_str()), 
                   spheres[i].material.ior);
        glUniform1i(glGetUniformLocation(computeShader, (prefix + ".material.type").c_str()), 
                   spheres[i].material.type);
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
    
    // Bind the raytracing texture as an image for writing - this should be done once during initialization
    // But we'll do it here to ensure it's properly bound
    if (pglBindImageTexture) {
        pglBindImageTexture(0, raytracingTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    }
    
    // Dispatch compute shader
    if (pglDispatchCompute && pglMemoryBarrier) {
        GLuint workGroupsX = (screenWidth + 15) / 16;
        GLuint workGroupsY = (screenHeight + 15) / 16;
        pglDispatchCompute(workGroupsX, workGroupsY, 1);
        
        // Wait for compute shader to finish
        pglMemoryBarrier(0x00000008); // GL_SHADER_IMAGE_ACCESS_BARRIER_BIT = 0x00000008
        
        // Debug: Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            static int errorCount = 0;
            if (errorCount < 5) { // Only show first 5 errors
                std::cerr << "OpenGL error after compute dispatch: " << error << std::endl;
                errorCount++;
            }
        }
    } else {
        std::cerr << "Compute shader dispatch functions not available" << std::endl;
        return;
    }
    
    // Render fullscreen quad with the raytraced result
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(fullscreenShader);
    
    // Bind the raytracing texture for reading
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, raytracingTexture);
    glUniform1i(glGetUniformLocation(fullscreenShader, "screenTexture"), 0);
    glUniform1f(glGetUniformLocation(fullscreenShader, "exposure"), exposure);
    glUniform1i(glGetUniformLocation(fullscreenShader, "enableToneMapping"), enableToneMapping ? 1 : 0);
    
    glBindVertexArray(fullscreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Debug: Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        static int errorCount = 0;
        if (errorCount < 5) { // Only show first 5 errors
            std::cerr << "OpenGL error after fullscreen render: " << error << std::endl;
            errorCount++;
        }
    }
}

void GraphicsManager::setLightProperties(const glm::vec3& lightPos, const glm::vec3& lightColor) {
    currentLightPos = lightPos;
    currentLightColor = lightColor;
}

void GraphicsManager::setupSphereBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    sphereIndexCount = static_cast<int>(indices.size());
    
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

void GraphicsManager::setupFloorBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Floor vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

bool GraphicsManager::checkComputeShaderSupport() {
    int major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    
    return (major > 4) || (major == 4 && minor >= 3);
}

bool GraphicsManager::loadComputeShaderFunctions() {
    // Load compute shader function pointers
    pglDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)glfwGetProcAddress("glDispatchCompute");
    pglBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)glfwGetProcAddress("glBindImageTexture");
    pglMemoryBarrier = (PFNGLMEMORYBARRIERPROC)glfwGetProcAddress("glMemoryBarrier");
    
    if (!pglDispatchCompute) {
        std::cerr << "Failed to load glDispatchCompute" << std::endl;
        return false;
    }
    
    if (!pglBindImageTexture) {
        std::cerr << "Failed to load glBindImageTexture" << std::endl;
        return false;
    }
    
    if (!pglMemoryBarrier) {
        std::cerr << "Failed to load glMemoryBarrier" << std::endl;
        return false;
    }
    
    std::cout << "Compute shader functions loaded successfully!" << std::endl;
    return true;
}

bool GraphicsManager::initForwardPlus() {
    // Calculate number of tiles
    numTilesX = (screenWidth + TILE_SIZE - 1) / TILE_SIZE;
    numTilesY = (screenHeight + TILE_SIZE - 1) / TILE_SIZE;
    
    std::cout << "Forward+ tiling: " << numTilesX << "x" << numTilesY << " tiles (" << TILE_SIZE << "x" << TILE_SIZE << " pixels each)" << std::endl;
    
    // Try to load Forward+ shaders
    depthPrepassShader = loadShaders("depth_prepass_vertex.glsl", "depth_prepass_fragment.glsl");
    if (depthPrepassShader == 0) {
        std::cerr << "Failed to load depth prepass shaders" << std::endl;
        return false;
    }
    
    lightCullingComputeShader = loadComputeShader("light_culling.comp");
    if (lightCullingComputeShader == 0) {
        std::cerr << "Failed to load light culling compute shader" << std::endl;
        return false;
    }
    
    tiledForwardShader = loadShaders("tiled_forward_vertex.glsl", "tiled_forward_fragment.glsl");
    if (tiledForwardShader == 0) {
        std::cerr << "Failed to load tiled forward shaders" << std::endl;
        return false;
    }
    
    setupForwardPlusBuffers();
    return true;
}

void GraphicsManager::setupForwardPlusBuffers() {
    // Create depth texture for depth prepass
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Define GL_SHADER_STORAGE_BUFFER if not available
    const GLenum GL_SHADER_STORAGE_BUFFER_LOCAL = 0x90D2;
    
    // Create light data buffer (for all lights in scene)
    glGenBuffers(1, &lightDataBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER_LOCAL, lightDataBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER_LOCAL, sizeof(float) * 8 * 256, nullptr, GL_DYNAMIC_DRAW); // Max 256 lights, 8 floats each (pos.xyz, color.rgb, radius, intensity)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER_LOCAL, 0, lightDataBuffer);
    
    // Create visible light indices buffer (per tile)
    int totalTiles = numTilesX * numTilesY;
    glGenBuffers(1, &visibleLightIndicesBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER_LOCAL, visibleLightIndicesBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER_LOCAL, sizeof(GLuint) * totalTiles * maxLightsPerTile, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER_LOCAL, 1, visibleLightIndicesBuffer);
    
    // Create light list buffer (count of lights per tile)
    glGenBuffers(1, &lightListBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER_LOCAL, lightListBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER_LOCAL, sizeof(GLuint) * totalTiles, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER_LOCAL, 2, lightListBuffer);
    
    std::cout << "Forward+ buffers created: " << totalTiles << " tiles, max " << maxLightsPerTile << " lights per tile" << std::endl;
}

void GraphicsManager::cleanupForwardPlus() {
    if (depthTexture) glDeleteTextures(1, &depthTexture);
    if (lightDataBuffer) glDeleteBuffers(1, &lightDataBuffer);
    if (visibleLightIndicesBuffer) glDeleteBuffers(1, &visibleLightIndicesBuffer);
    if (lightListBuffer) glDeleteBuffers(1, &lightListBuffer);
    if (depthPrepassShader) glDeleteProgram(depthPrepassShader);
    if (lightCullingComputeShader) glDeleteProgram(lightCullingComputeShader);
    if (tiledForwardShader) glDeleteProgram(tiledForwardShader);
}

void GraphicsManager::renderForwardPlusPass(const glm::mat4& view, const glm::mat4& projection,
                                           const std::vector<RTSphere>& spheres,
                                           const std::vector<Cube>& cubes,
                                           const std::vector<Bullet>& bullets,
                                           const glm::vec3& mainObjectPos,
                                           const Material& currentMaterial) {
    if (!forwardPlusSupported) {
        // Fallback to traditional forward rendering
        renderForwardPass(view, projection, spheres, cubes, bullets, mainObjectPos, currentMaterial);
        return;
    }
    
    // Simplified Forward+ without depth prepass for now
    // Just do light culling and tiled shading
    
    // 1. Light culling (compute shader) 
    performLightCulling(view, projection);
    
    // 2. Regular forward rendering with tiled lighting
    // Skip the depth prepass and just render normally but with tiled lighting
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    
    // Use tiled forward shader
    glUseProgram(tiledForwardShader);
    
    // Set common uniforms
    glUniformMatrix4fv(glGetUniformLocation(tiledForwardShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(tiledForwardShader, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform2i(glGetUniformLocation(tiledForwardShader, "screenSize"), screenWidth, screenHeight);
    glUniform2i(glGetUniformLocation(tiledForwardShader, "numTiles"), numTilesX, numTilesY);
    
    // Set view position for specular calculations
    glm::vec3 viewPos = glm::vec3(glm::inverse(view)[3]);
    glUniform3f(glGetUniformLocation(tiledForwardShader, "viewPos"), viewPos.x, viewPos.y, viewPos.z);
    
    // Render floor
    glm::mat4 floorModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, 0.0f));
    floorModel = glm::scale(floorModel, glm::vec3(20.0f, 0.1f, 20.0f));
    glUniformMatrix4fv(glGetUniformLocation(tiledForwardShader, "model"), 1, GL_FALSE, &floorModel[0][0]);
    glUniform3f(glGetUniformLocation(tiledForwardShader, "objectColor"), 0.3f, 0.3f, 0.3f);
    glUniform1i(glGetUniformLocation(tiledForwardShader, "useMaterial"), 0);
    glBindVertexArray(floorVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // Render main sphere
    glm::mat4 mainModel = glm::translate(glm::mat4(1.0f), mainObjectPos);
    glUniformMatrix4fv(glGetUniformLocation(tiledForwardShader, "model"), 1, GL_FALSE, &mainModel[0][0]);
    
    // Set material properties for main sphere
    glUniform3f(glGetUniformLocation(tiledForwardShader, "material.ambient"), 
               currentMaterial.ambient.x, currentMaterial.ambient.y, currentMaterial.ambient.z);
    glUniform3f(glGetUniformLocation(tiledForwardShader, "material.diffuse"), 
               currentMaterial.diffuse.x, currentMaterial.diffuse.y, currentMaterial.diffuse.z);
    glUniform3f(glGetUniformLocation(tiledForwardShader, "material.specular"), 
               currentMaterial.specular.x, currentMaterial.specular.y, currentMaterial.specular.z);
    glUniform1f(glGetUniformLocation(tiledForwardShader, "material.shininess"), currentMaterial.shininess);
    glUniform1i(glGetUniformLocation(tiledForwardShader, "useMaterial"), 1);
    
    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
    
    // Render spawned cubes
    for (const auto& cube : cubes) {
        if (cube.isActive) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cube.position);
            glUniformMatrix4fv(glGetUniformLocation(tiledForwardShader, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(tiledForwardShader, "objectColor"), 0.3f, 0.8f, 0.3f);
            glUniform1i(glGetUniformLocation(tiledForwardShader, "useMaterial"), 0);
            glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
        }
    }
    
    // Render bullets
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), bullet.position);
            model = glm::scale(model, glm::vec3(0.05f));
            glUniformMatrix4fv(glGetUniformLocation(tiledForwardShader, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(tiledForwardShader, "objectColor"), 1.0f, 1.0f, 0.0f);
            glUniform1i(glGetUniformLocation(tiledForwardShader, "useMaterial"), 0);
            glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
        }
    }
    
    static bool firstCall = true;
    if (firstCall) {
        std::cout << "Forward+ rendering pipeline executed successfully!" << std::endl;
        firstCall = false;
    }
}

void GraphicsManager::performDepthPrepass(const glm::mat4& view, const glm::mat4& projection,
                                         const std::vector<RTSphere>& spheres,
                                         const std::vector<Cube>& cubes,
                                         const std::vector<Bullet>& bullets,
                                         const glm::vec3& mainObjectPos) {
    // This function is currently unused in the simplified Forward+ approach
}

void GraphicsManager::performLightCulling(const glm::mat4& view, const glm::mat4& projection) {
    // Update light data (for now, just use the main light)
    std::vector<glm::vec3> lightPositions = {currentLightPos};
    std::vector<glm::vec3> lightColors = {currentLightColor};
    updateLightData(lightPositions, lightColors);
    
    // Use light culling compute shader
    glUseProgram(lightCullingComputeShader);
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(lightCullingComputeShader, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(lightCullingComputeShader, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform2i(glGetUniformLocation(lightCullingComputeShader, "screenSize"), screenWidth, screenHeight);
    glUniform2i(glGetUniformLocation(lightCullingComputeShader, "numTiles"), numTilesX, numTilesY);
    glUniform1i(glGetUniformLocation(lightCullingComputeShader, "numLights"), static_cast<int>(lightPositions.size()));
    
    // Bind depth texture (we don't have proper depth prepass, so this might be empty)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(lightCullingComputeShader, "depthTexture"), 0);
    
    // Dispatch compute shader (one thread group per tile)
    if (pglDispatchCompute) {
        pglDispatchCompute(numTilesX, numTilesY, 1);
        pglMemoryBarrier(0x00000002); // GL_SHADER_STORAGE_BARRIER_BIT = 0x00000002
    }
}

void GraphicsManager::renderTiledObjects(const glm::mat4& view, const glm::mat4& projection,
                                        const std::vector<RTSphere>& spheres,
                                        const std::vector<Cube>& cubes,
                                        const std::vector<Bullet>& bullets,
                                        const glm::vec3& mainObjectPos,
                                        const Material& currentMaterial) {
    // This function is replaced by the simplified approach in renderForwardPlusPass
}

void GraphicsManager::updateLightData(const std::vector<glm::vec3>& lightPositions, const std::vector<glm::vec3>& lightColors) {
    struct LightData {
        glm::vec3 position;
        float radius;
        glm::vec3 color;
        float intensity;
    };
    
    std::vector<LightData> lights;
    for (size_t i = 0; i < lightPositions.size() && i < lightColors.size(); i++) {
        LightData light;
        light.position = lightPositions[i];
        light.radius = 10.0f; // Default light radius
        light.color = lightColors[i];
        light.intensity = 1.0f; // Default intensity
        lights.push_back(light);
    }
    
    // Update light data buffer
    const GLenum GL_SHADER_STORAGE_BUFFER_LOCAL = 0x90D2;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER_LOCAL, lightDataBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER_LOCAL, 0, lights.size() * sizeof(LightData), lights.data());
}

void GraphicsManager::renderModern(const std::vector<RTSphere>& spheres, const glm::vec3& cameraPos, 
                                  const glm::vec3& cameraFront, const glm::vec3& cameraUp, const glm::vec3& cameraRight,
                                  const glm::vec3& lightPos, const glm::vec3& lightColor, float time,
                                  const std::vector<Cube>& cubes, const std::vector<Bullet>& bullets,
                                  const glm::vec3& mainObjectPos, const Material& currentMaterial) {
    // Modern renderer integration is prepared but not yet active
    // Fall back to existing Forward+ implementation for now
    
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    
    renderForwardPlusPass(view, projection, spheres, cubes, bullets, mainObjectPos, currentMaterial);
    
    static bool notified = false;
    if (!notified) {
        std::cout << "Modern renderer placeholder - using optimized Forward+ for now" << std::endl;
        notified = true;
    }
}

void GraphicsManager::initFPSDisplay() {
    fpsDisplayInitialized = true;
    
    // Create VAO and VBO for FPS display quads
    glGenVertexArrays(1, &fpsVAO);
    glGenBuffers(1, &fpsVBO);
    
    glBindVertexArray(fpsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fpsVBO);
    
    // Reserve space for quad vertices (will be updated per frame)
    // Using position (3D) + normal (3D) + texcoord (2D) to match existing vertex format
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 48, nullptr, GL_DYNAMIC_DRAW); // 6 vertices * 8 components
    
    // Position attribute (3D to match sphere shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    std::cout << "FPS display initialized successfully!" << std::endl;
}

void GraphicsManager::renderFPS(float fps, float deltaTime) {
    if (!fpsDisplayInitialized) {
        return;
    }
    
    // Save current OpenGL state
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    
    // Setup for overlay rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use the main shader with orthographic projection for overlay
    glUseProgram(mainShaderProgram);
    
    // Set up orthographic projection for 2D overlay
    glm::mat4 orthoProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight, -1.0f, 1.0f);
    glm::mat4 orthoView = glm::mat4(1.0f);
    glm::mat4 orthoModel = glm::mat4(1.0f);
    
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "projection"), 1, GL_FALSE, &orthoProjection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "view"), 1, GL_FALSE, &orthoView[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "model"), 1, GL_FALSE, &orthoModel[0][0]);
    
    // Disable material mode and set up for colored rendering
    glUniform1i(glGetUniformLocation(mainShaderProgram, "useMaterial"), 0);
    glUniform1i(glGetUniformLocation(mainShaderProgram, "shadingModel"), 0);
    
    glBindVertexArray(fpsVAO);
    
    // Render a simple colored rectangle for FPS (simplified approach)
    float bgX = screenWidth - 120.0f;
    float bgY = screenHeight - 60.0f;
    float bgWidth = 110.0f;
    float bgHeight = 50.0f;
    
    // Background quad (dark semi-transparent)
    glUniform3f(glGetUniformLocation(mainShaderProgram, "objectColor"), 0.0f, 0.0f, 0.0f);
    
    float bgVertices[] = {
        // Triangle 1
        bgX,          bgY,          0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        bgX + bgWidth, bgY,          0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
        bgX,          bgY + bgHeight, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
        
        // Triangle 2
        bgX + bgWidth, bgY,          0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
        bgX + bgWidth, bgY + bgHeight, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        bgX,          bgY + bgHeight, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f
    };
    
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgVertices), bgVertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Restore OpenGL state
    glUseProgram(currentProgram);
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    if (!blendEnabled) glDisable(GL_BLEND);
    
    // Print FPS to console periodically for precise measurement
    static float printTimer = 0.0f;
    printTimer += deltaTime;
    if (printTimer >= 1.0f) { // Print every second
        std::string renderMode = "Legacy OpenGL";
        if (useVulkanRenderer) {
            renderMode = "Modern Vulkan (Forward+)";
        }
        std::cout << "FPS: " << static_cast<int>(fps) << " | Frame time: " 
                  << std::fixed << std::setprecision(2) << (deltaTime * 1000.0f) << "ms | Renderer: " << renderMode << std::endl;
        printTimer = 0.0f;
    }
}

void GraphicsManager::setMaterialUniforms(GLuint program, const Material& material, bool useEnhancedFeatures) {
    // Set material properties
    glUniform3f(glGetUniformLocation(program, "material.ambient"), material.ambient.x, material.ambient.y, material.ambient.z);
    glUniform3f(glGetUniformLocation(program, "material.diffuse"), material.diffuse.x, material.diffuse.y, material.diffuse.z);
    glUniform3f(glGetUniformLocation(program, "material.specular"), material.specular.x, material.specular.y, material.specular.z);
    glUniform1f(glGetUniformLocation(program, "material.shininess"), material.shininess);
    
    if (useEnhancedFeatures) {
        // Enhanced rendering features
        bool isMetallic = (material.name.find("Gold") != std::string::npos ||
                          material.name.find("Silver") != std::string::npos ||
                          material.name.find("Copper") != std::string::npos ||
                          material.name.find("Bronze") != std::string::npos);
        
        float metallicFactor = isMetallic ? 0.9f : 0.1f;
        float roughnessFactor = isMetallic ? 0.1f : 0.8f;
        float ambientOcclusion = 0.1f; // Subtle AO
        
        glUniform1i(glGetUniformLocation(program, "enableReflections"), 1);
        glUniform1i(glGetUniformLocation(program, "enableSSAO"), 0); // Keep simple for now
        glUniform1f(glGetUniformLocation(program, "ambientOcclusion"), ambientOcclusion);
        glUniform1f(glGetUniformLocation(program, "metallicFactor"), metallicFactor);
        glUniform1f(glGetUniformLocation(program, "roughnessFactor"), roughnessFactor);
        glUniform1i(glGetUniformLocation(program, "hasEnvironmentMap"), 0); // No cubemap for now
        
        glUniform1i(glGetUniformLocation(program, "useMaterial"), 1); // Enable material mode
    }
}

void GraphicsManager::renderForwardPass(const glm::mat4& view, const glm::mat4& projection, 
                                        const std::vector<RTSphere>& spheres,
                                        const std::vector<Cube>& cubes,
                                        const std::vector<Bullet>& bullets,
                                        const glm::vec3& mainObjectPos,
                                        const Material& currentMaterial) {
    // Forward rendering: render objects front-to-back for early Z rejection
    
    // 1. Render opaque objects first (front to back for performance)
    renderOpaqueObjects(view, projection, spheres, cubes, bullets, mainObjectPos, currentMaterial);
    
    // 2. Render transparent objects last (back to front for correct blending)
    renderTransparentObjects(view, projection, spheres, cubes, bullets);
}

void GraphicsManager::renderOpaqueObjects(const glm::mat4& view, const glm::mat4& projection,
                                         const std::vector<RTSphere>& spheres,
                                         const std::vector<Cube>& cubes,
                                         const std::vector<Bullet>& bullets,
                                         const glm::vec3& mainObjectPos,
                                         const Material& currentMaterial) {
    // Disable blending for opaque objects
    glDisable(GL_BLEND);
    
    // Set global render state once for all objects
    setGlobalRenderState(view, projection);
    
    // Render floor first (usually largest/farthest object)
    glm::mat4 floorModel = glm::mat4(1.0f);
    floorModel = glm::translate(floorModel, glm::vec3(0.0f, -0.5f, 0.0f));
    floorModel = glm::scale(floorModel, glm::vec3(20.0f, 0.1f, 20.0f));
    renderFloor(floorModel, view, projection);
    
    // Render main sphere
    glm::mat4 mainModel = glm::mat4(1.0f);
    mainModel = glm::translate(mainModel, mainObjectPos);
    renderSphere(mainModel, view, projection, currentMaterial, true);
    
    // Render spawned cubes (opaque)
    for (const auto& cube : cubes) {
        if (cube.isActive) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cube.position);
            renderSpawned(model, view, projection);
        }
    }
    
    // Render bullets (small objects, render last among opaques)
    for (const auto& bullet : bullets) {
        if (bullet.active) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, bullet.position);
            model = glm::scale(model, glm::vec3(0.05f));
            renderBullet(model, view, projection);
        }
    }
}

void GraphicsManager::renderTransparentObjects(const glm::mat4& view, const glm::mat4& projection,
                                              const std::vector<RTSphere>& spheres,
                                              const std::vector<Cube>& cubes,
                                              const std::vector<Bullet>& bullets) {
    // Enable blending for transparent objects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth writing for transparent objects (but keep depth testing)
    glDepthMask(GL_FALSE);
    
    // TODO: Sort transparent objects back-to front and render
    // For now, no transparent objects in this scene
    
    // Re-enable depth writing
    glDepthMask(GL_TRUE);
}

void GraphicsManager::setGlobalRenderState(const glm::mat4& view, const glm::mat4& projection) {
    // Set shared uniforms that don't change per object in forward rendering
    
    // Update view position for all shaders (for specular calculations)
    glm::vec3 viewPos = glm::vec3(glm::inverse(view)[3]);
    
    // Set for main shader
    glUseProgram(mainShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3f(glGetUniformLocation(mainShaderProgram, "viewPos"), viewPos.x, viewPos.y, viewPos.z);
    glUniform3f(glGetUniformLocation(mainShaderProgram, "lightPos"), currentLightPos.x, currentLightPos.y, currentLightPos.z);
    glUniform3f(glGetUniformLocation(mainShaderProgram, "lightColor"), currentLightColor.x, currentLightColor.y, currentLightColor.z);
    
    // Set for floor shader
    glUseProgram(floorShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(floorShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3f(glGetUniformLocation(floorShaderProgram, "viewPos"), viewPos.x, viewPos.y, viewPos.z);
    glUniform3f(glGetUniformLocation(floorShaderProgram, "lightPos"), currentLightPos.x, currentLightPos.y, currentLightPos.z);
    glUniform3f(glGetUniformLocation(floorShaderProgram, "lightColor"), currentLightColor.x, currentLightColor.y, currentLightColor.z);
}