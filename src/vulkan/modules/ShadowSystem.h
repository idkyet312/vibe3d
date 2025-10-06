#pragma once

#include "../VulkanTypes.h"
#include "../VulkanDevice.h"
#include "../VulkanImage.h"
#include <memory>
#include <array>
#include <vector>

namespace vibe::vk {

/**
 * @brief Manages cascaded shadow mapping resources and rendering
 * 
 * Handles:
 * - Shadow map image creation (4 cascades)
 * - Shadow render pass and framebuffers
 * - Shadow pipeline
 * - Cascade split calculation
 * - Shadow map rendering
 */
class ShadowSystem {
public:
    static constexpr size_t NUM_CASCADES = 4;
    static constexpr uint32_t SHADOW_MAP_SIZE = 2048;

    explicit ShadowSystem(VulkanDevice& device);
    ~ShadowSystem();

    // Initialization
    bool initialize(uint32_t screenWidth, uint32_t screenHeight);
    void cleanup();

    // Rendering
    void renderShadowCascades(VkCommandBuffer cmd, VkBuffer vertexBuffer, VkBuffer indexBuffer,
                             uint32_t indexCount, const glm::mat4& cubeTransform);
    void updateShadowUBO(VulkanBuffer& shadowBuffer, uint32_t currentFrame);

    // Getters
    [[nodiscard]] VkRenderPass getShadowRenderPass() const noexcept { return shadowRenderPass_; }
    [[nodiscard]] VkPipeline getShadowPipeline() const noexcept { return shadowPipeline_; }
    [[nodiscard]] VkPipelineLayout getShadowPipelineLayout() const noexcept { return shadowPipelineLayout_; }
    [[nodiscard]] VkSampler getShadowSampler() const noexcept { return shadowSampler_; }
    [[nodiscard]] const std::array<VkImageView, NUM_CASCADES>& getShadowImageViews() const noexcept {
        return shadowImageViews_;
    }
    [[nodiscard]] const std::array<float, NUM_CASCADES>& getCascadeSplits() const noexcept {
        return cascadeSplits_;
    }
    [[nodiscard]] const glm::vec3& getLightDirection() const noexcept { return lightDirection_; }

private:
    bool createShadowResources();
    bool createShadowRenderPass();
    bool createShadowFramebuffers();
    bool createShadowPipeline();
    bool createShadowSampler();
    void calculateCascadeSplits();
    glm::mat4 calculateLightSpaceMatrix(float nearPlane, float farPlane);

    VulkanDevice& device_;
    uint32_t screenWidth_;
    uint32_t screenHeight_;

    // Shadow mapping resources
    std::array<std::unique_ptr<VulkanImage>, NUM_CASCADES> shadowImages_;
    std::array<VkImageView, NUM_CASCADES> shadowImageViews_{};
    VkSampler shadowSampler_ = VK_NULL_HANDLE;
    VkRenderPass shadowRenderPass_ = VK_NULL_HANDLE;
    std::array<std::vector<VkFramebuffer>, NUM_CASCADES> shadowFramebuffers_;
    VkPipelineLayout shadowPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline shadowPipeline_ = VK_NULL_HANDLE;

    // Cascade configuration
    std::array<float, NUM_CASCADES> cascadeSplits_{};
    glm::vec3 lightDirection_ = glm::vec3(0.0f, -1.0f, 0.0f);
};

} // namespace vibe::vk
