#pragma once

#include "VulkanTypes.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanDescriptor.h"
#include <memory>
#include <vector>
#include <array>

struct GLFWwindow;

namespace vibe::vk {

/**
 * @brief Modern Forward+ (Tiled Forward) Renderer using Vulkan
 * 
 * This renderer implements a tiled forward+ rendering pipeline:
 * 1. Depth prepass
 * 2. Light culling compute pass (generates per-tile light lists)
 * 3. Forward shading pass (uses per-tile light lists)
 * 
 * Features C++20 concepts, ranges, and modern practices
 */
class ForwardPlusRenderer {
public:
    struct Config {
        uint32_t width = 1280;
        uint32_t height = 720;
        uint32_t maxLights = 1024;
        uint32_t tileSize = 16;
        bool enableMSAA = true;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_4_BIT;
    };

    explicit ForwardPlusRenderer(const Config& config);
    ~ForwardPlusRenderer();

    // Initialization
    bool initialize(GLFWwindow* window);
    void cleanup();

    // Frame rendering
    void beginFrame();
    void endFrame();
    
    // Drawing
    void renderScene(const CameraUBO& camera, std::span<const PointLight> lights);
    
    // Resource management
    void updateLights(std::span<const PointLight> lights);
    void uploadMesh(std::span<const Vertex> vertices, std::span<const uint32_t> indices);
    
    // Getters
    [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }
    [[nodiscard]] const Config& getConfig() const noexcept { return config_; }
    [[nodiscard]] uint32_t getCurrentFrame() const noexcept { return currentFrame_; }

    // Resize handling
    void onWindowResize(uint32_t width, uint32_t height);

private:
    // Initialization helpers
    bool createRenderPass();
    bool createDepthResources();
    bool createFramebuffers();
    bool createDescriptorSetLayouts();
    bool createPipeline();
    bool createSyncObjects();
    bool createBuffers();
    bool createCommandBuffers();
    bool createCubeGeometry();
    
    // Shadow mapping helpers
    bool createShadowResources();
    bool createShadowRenderPass();
    bool createShadowFramebuffers();
    bool createShadowPipeline();
    bool createShadowSampler();
    void calculateCascadeSplits();
    void updateShadowUBO();
    void renderShadowCascades(VkCommandBuffer cmd);
    glm::mat4 calculateLightSpaceMatrix(float nearPlane, float farPlane);

    // Shader helpers
    std::vector<char> readShaderFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    // Pipeline creation
    bool createDepthPrepassPipeline();
    bool createLightCullingPipeline();
    bool createForwardPipeline();

    // Rendering passes
    void recordDepthPrepass(VkCommandBuffer cmd);
    void recordLightCulling(VkCommandBuffer cmd, uint32_t numLights);
    void recordForwardPass(VkCommandBuffer cmd);

    // Descriptor management
    void updateDescriptorSets();

    // Helper functions
    [[nodiscard]] VkFormat findDepthFormat() const;
    [[nodiscard]] uint32_t calculateNumTiles(uint32_t dimension) const noexcept;

    Config config_;
    bool initialized_ = false;
    uint32_t currentFrame_ = 0;
    uint32_t imageIndex_ = 0;
    
    // Core Vulkan objects
    std::unique_ptr<VulkanDevice> device_;
    std::unique_ptr<VulkanSwapChain> swapChain_;
    
    // Render pass and framebuffers
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkRenderPass depthPrepass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;
    std::vector<VkFramebuffer> depthFramebuffers_;
    
    // Depth resources
    std::unique_ptr<VulkanImage> depthImage_;
    VkImageView depthImageView_ = VK_NULL_HANDLE;
    
    // Shadow mapping resources
    static constexpr size_t NUM_CASCADES = 4;
    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;
    std::array<std::unique_ptr<VulkanImage>, NUM_CASCADES> shadowImages_;
    std::array<VkImageView, NUM_CASCADES> shadowImageViews_{};
    VkSampler shadowSampler_ = VK_NULL_HANDLE;
    VkRenderPass shadowRenderPass_ = VK_NULL_HANDLE;
    std::array<std::vector<VkFramebuffer>, NUM_CASCADES> shadowFramebuffers_;
    VkPipelineLayout shadowPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline shadowPipeline_ = VK_NULL_HANDLE;
    
    // Cascade split distances
    std::array<float, NUM_CASCADES> cascadeSplits_{};
    glm::vec3 lightDirection_ = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    
    // MSAA resources
    std::unique_ptr<VulkanImage> colorImage_;
    VkImageView colorImageView_ = VK_NULL_HANDLE;
    
    // Pipelines
    VkPipelineLayout depthPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline depthPipeline_ = VK_NULL_HANDLE;
    
    VkPipelineLayout lightCullingPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline lightCullingPipeline_ = VK_NULL_HANDLE;
    
    VkPipelineLayout forwardPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline forwardPipeline_ = VK_NULL_HANDLE;
    
    // Descriptor sets
    std::unique_ptr<VulkanDescriptorPool> descriptorPool_;
    VkDescriptorSetLayout globalDescriptorLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout lightCullingDescriptorLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout materialDescriptorLayout_ = VK_NULL_HANDLE;
    
    std::vector<VkDescriptorSet> globalDescriptorSets_;
    std::vector<VkDescriptorSet> lightCullingDescriptorSets_;
    
    // Buffers
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    
    std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> cameraBuffers_;
    std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> shadowBuffers_;
    std::unique_ptr<VulkanBuffer> lightBuffer_;
    std::unique_ptr<VulkanBuffer> lightGridBuffer_;
    std::unique_ptr<VulkanBuffer> visibleLightIndicesBuffer_;
    std::unique_ptr<VulkanBuffer> vertexBuffer_;
    std::unique_ptr<VulkanBuffer> indexBuffer_;
    
    // Synchronization
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores_{};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores_{};
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences_{};
    
    // Command buffers
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers_{};
    
    // Light culling data
    LightGrid lightGrid_;
    uint32_t numTilesX_ = 0;
    uint32_t numTilesY_ = 0;
    
    // Mesh data
    uint32_t indexCount_ = 0;
};

} // namespace vibe::vk
