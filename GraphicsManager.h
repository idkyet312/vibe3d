#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

// Forward declarations
struct Material;
struct RTMaterial;
struct RTSphere;
struct Cube;
struct Bullet;

// OpenGL function pointers for compute shaders (in case GLAD doesn't load them)
typedef void (APIENTRY *PFNGLDISPATCHCOMPUTEPROC)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void (APIENTRY *PFNGLBINDIMAGETEXTUREPROC)(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
typedef void (APIENTRY *PFNGLMEMORYBARRIERPROC)(GLbitfield barriers);

class GraphicsManager {
public:
    GraphicsManager();
    ~GraphicsManager();

    // Initialization
    bool initialize(unsigned int width, unsigned int height);
    void cleanup();

    // Shader management
    GLuint loadShaders(const char* vertex_file_path, const char* fragment_file_path);
    GLuint loadComputeShader(const char* compute_file_path);

    // Mesh creation
    void createSphereMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int segments);
    void createFloorMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices);

    // Rendering
    void beginFrame();
    void endFrame();
    void renderSphere(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, 
                      const Material& material, bool useEnhancedFeatures = true);
    void renderFloor(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);
    void renderBullet(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);
    void renderSpawned(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);
    
    // Forward rendering pipeline
    void renderForwardPass(const glm::mat4& view, const glm::mat4& projection, 
                          const std::vector<RTSphere>& spheres,
                          const std::vector<Cube>& cubes,
                          const std::vector<Bullet>& bullets,
                          const glm::vec3& mainObjectPos,
                          const Material& currentMaterial);
    void renderOpaqueObjects(const glm::mat4& view, const glm::mat4& projection,
                           const std::vector<RTSphere>& spheres,
                           const std::vector<Cube>& cubes,
                           const std::vector<Bullet>& bullets,
                           const glm::vec3& mainObjectPos,
                           const Material& currentMaterial);
    void renderTransparentObjects(const glm::mat4& view, const glm::mat4& projection,
                                const std::vector<RTSphere>& spheres,
                                const std::vector<Cube>& cubes,
                                const std::vector<Bullet>& bullets);
    void setGlobalRenderState(const glm::mat4& view, const glm::mat4& projection);
    
    // Forward+ (Tiled Forward) rendering pipeline
    void renderForwardPlusPass(const glm::mat4& view, const glm::mat4& projection,
                              const std::vector<RTSphere>& spheres,
                              const std::vector<Cube>& cubes,
                              const std::vector<Bullet>& bullets,
                              const glm::vec3& mainObjectPos,
                              const Material& currentMaterial);
    void performDepthPrepass(const glm::mat4& view, const glm::mat4& projection,
                           const std::vector<RTSphere>& spheres,
                           const std::vector<Cube>& cubes,
                           const std::vector<Bullet>& bullets,
                           const glm::vec3& mainObjectPos);
    void performLightCulling(const glm::mat4& view, const glm::mat4& projection);
    void renderTiledObjects(const glm::mat4& view, const glm::mat4& projection,
                          const std::vector<RTSphere>& spheres,
                          const std::vector<Cube>& cubes,
                          const std::vector<Bullet>& bullets,
                          const glm::vec3& mainObjectPos,
                          const Material& currentMaterial);

    // Raytracing
    bool initRaytracing();
    void createFullscreenQuad();
    void renderRaytraced(const std::vector<RTSphere>& spheres, const glm::vec3& cameraPos, 
                        const glm::vec3& cameraFront, const glm::vec3& cameraUp, const glm::vec3& cameraRight,
                        const glm::vec3& lightPos, const glm::vec3& lightColor, float time,
                        int maxBounces, int numSamples, float exposure, bool enableToneMapping);

    // Lighting
    void setLightProperties(const glm::vec3& lightPos, const glm::vec3& lightColor);

    // Getters
    bool isRaytracingSupported() const { return raytracingSupported; }
    int getSphereIndexCount() const { return sphereIndexCount; }
    
    // FPS display
    void renderFPS(float fps, float deltaTime);
    void initFPSDisplay();
    
    // Modern renderer integration
    bool useModernRenderer() const { return useVulkanRenderer; }
    void renderModern(const std::vector<RTSphere>& spheres, const glm::vec3& cameraPos, 
                     const glm::vec3& cameraFront, const glm::vec3& cameraUp, const glm::vec3& cameraRight,
                     const glm::vec3& lightPos, const glm::vec3& lightColor, float time,
                     const std::vector<Cube>& cubes, const std::vector<Bullet>& bullets,
                     const glm::vec3& mainObjectPos, const Material& currentMaterial);

private:
    // OpenGL objects
    GLuint sphereVAO, sphereVBO, sphereEBO;
    GLuint floorVAO, floorVBO, floorEBO;
    GLuint fullscreenVAO;
    
    // Shaders
    GLuint mainShaderProgram;
    GLuint floorShaderProgram;
    GLuint computeShader;
    GLuint fullscreenShader;
    
    // Raytracing
    GLuint raytracingTexture;
    bool raytracingSupported;
    
    // Forward+ (Tiled Forward) rendering resources
    GLuint depthPrepassShader;
    GLuint lightCullingComputeShader;
    GLuint tiledForwardShader;
    GLuint depthTexture;
    GLuint lightListBuffer;
    GLuint visibleLightIndicesBuffer;
    GLuint lightDataBuffer;
    bool forwardPlusSupported;
    
    // Tiling parameters
    static const int TILE_SIZE = 16;
    int numTilesX, numTilesY;
    int maxLightsPerTile;
    
    // Mesh data
    int sphereIndexCount;
    
    // Screen dimensions
    unsigned int screenWidth, screenHeight;
    
    // Lighting
    glm::vec3 currentLightPos;
    glm::vec3 currentLightColor;
    
    // OpenGL function pointers
    PFNGLDISPATCHCOMPUTEPROC pglDispatchCompute;
    PFNGLBINDIMAGETEXTUREPROC pglBindImageTexture;
    PFNGLMEMORYBARRIERPROC pglMemoryBarrier;
    
    // FPS display
    GLuint fpsShaderProgram;
    GLuint fpsVAO, fpsVBO;
    bool fpsDisplayInitialized;
    
    // Modern renderer (Vulkan-based Forward+ with Ray Tracing)
    // std::unique_ptr<class ModernRenderer> modernRenderer_;
    bool useVulkanRenderer;
    
    // Helper functions
    void setupSphereBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    void setupFloorBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    bool checkComputeShaderSupport();
    void setMaterialUniforms(GLuint program, const Material& material, bool useEnhancedFeatures);
    bool loadComputeShaderFunctions();
    
    // Forward+ helper functions
    bool initForwardPlus();
    void setupForwardPlusBuffers();
    void updateLightData(const std::vector<glm::vec3>& lightPositions, const std::vector<glm::vec3>& lightColors);
    void cleanupForwardPlus();
};