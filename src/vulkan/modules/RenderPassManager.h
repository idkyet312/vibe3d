#pragma once

#include "../VulkanTypes.h"
#include "../VulkanDevice.h"
#include "../VulkanSwapChain.h"
#include "../VulkanImage.h"
#include <memory>
#include <vector>
#include <array>

namespace vibe::vk {

/**
 * @brief Manages render passes and command recording
 * 
 * Handles:
 * - Main render pass creation
 * - Framebuffer management  
 * - Command buffer recording
 * - Scene rendering orchestration
 */
class RenderPassManager {
public:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    explicit RenderPassManager(VulkanDevice& device);
    ~RenderPassManager();

    // Initialization
    bool createMainRenderPass(VulkanSwapChain& swapChain);
    bool createDepthResources(VulkanSwapChain& swapChain);
    bool createFramebuffers(VulkanSwapChain& swapChain);
    bool createSyncObjects();
    bool createCommandBuffers();
    void cleanup();

    // Frame operations
    void beginFrame(VulkanSwapChain& swapChain, uint32_t& imageIndex, uint32_t& currentFrame);
    void endFrame(VulkanSwapChain& swapChain, uint32_t imageIndex, uint32_t currentFrame);

    // Rendering
    void recordForwardPass(VkCommandBuffer cmd, uint32_t imageIndex, VulkanSwapChain& swapChain,
                          VkPipeline pipeline, VkPipelineLayout pipelineLayout,
                          VkBuffer vertexBuffer, VkBuffer indexBuffer, uint32_t indexCount,
                          const std::vector<VkDescriptorSet>& descriptorSets,
                          uint32_t currentFrame, int debugMode, const glm::mat4& cubeTransform);

    // Getters
    [[nodiscard]] VkRenderPass getMainRenderPass() const noexcept { return renderPass_; }
    [[nodiscard]] VkCommandBuffer getCommandBuffer(uint32_t frame) const noexcept {
        return commandBuffers_[frame];
    }
    [[nodiscard]] VkFence getInFlightFence(uint32_t frame) const noexcept {
        return inFlightFences_[frame];
    }

private:
    [[nodiscard]] VkFormat findDepthFormat() const;

    VulkanDevice& device_;

    // Main render pass
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;

    // Depth resources
    std::unique_ptr<VulkanImage> depthImage_;
    VkImageView depthImageView_ = VK_NULL_HANDLE;

    // Synchronization
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores_{};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores_{};
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences_{};

    // Command buffers
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers_{};
};

} // namespace vibe::vk
