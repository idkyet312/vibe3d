#pragma once

#include "VulkanTypes.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include <memory>
#include <vector>
#include <array>

namespace vibe::vk {

/**
 * @brief Post-processing manager for bloom and other effects
 * Handles offscreen rendering, bloom passes, and final composite
 */
class PostProcessManager {
public:
    explicit PostProcessManager(VulkanDevice& device);
    ~PostProcessManager();
    
    struct Config {
        uint32_t width = 1280;
        uint32_t height = 720;
        bool enableBloom = true;
        float bloomThreshold = 1.0f;
        float bloomIntensity = 0.3f;
        float bloomRadius = 4.0f;
    };
    
    bool initialize(const Config& config);
    void cleanup();
    
    // Get render target for main scene rendering (HDR)
    VkImage getSceneImage() const { return sceneColorImage_->getImage(); }
    VkImageView getSceneImageView() const { return sceneColorImageView_; }
    VkFramebuffer getSceneFramebuffer(size_t index) const { return sceneFramebuffers_[index]; }
    VkRenderPass getSceneRenderPass() const { return sceneRenderPass_; }
    
    // Apply post-processing effects
    void applyPostProcessing(VkCommandBuffer cmd, VkImageView depthView, uint32_t swapchainImageIndex);
    
    // Getters for swapchain presentation
    VkRenderPass getFinalRenderPass() const { return finalRenderPass_; }
    VkFramebuffer getFinalFramebuffer(size_t index) const { return finalFramebuffers_[index]; }
    
    // Update settings
    void setBloomEnabled(bool enabled) { config_.enableBloom = enabled; }
    void setBloomThreshold(float threshold) { config_.bloomThreshold = threshold; }
    void setBloomIntensity(float intensity) { config_.bloomIntensity = intensity; }
    void setBloomRadius(float radius) { config_.bloomRadius = radius; }
    
    bool isInitialized() const { return initialized_; }
    
    void onResize(uint32_t width, uint32_t height);
    
    // Setup framebuffers (called after swapchain creation/recreation)
    bool setupFramebuffers(VkImageView depthView, const std::vector<VkImageView>& swapchainViews);

private:
    std::vector<char> readShaderFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    
    bool createSceneRenderTarget();
    bool createSceneRenderPass();
    bool createSceneFramebuffers(VkImageView depthView);
    
    bool createBloomResources();
    bool createBloomPipeline();
    
    bool createFinalRenderPass();
    bool createFinalFramebuffers(const std::vector<VkImageView>& swapchainViews);
    bool createFinalPipeline();
    
    bool createDescriptorSets();
    
    VulkanDevice& device_;
    Config config_;
    bool initialized_ = false;
    
    // Scene render target (HDR)
    std::unique_ptr<VulkanImage> sceneColorImage_;
    VkImageView sceneColorImageView_ = VK_NULL_HANDLE;
    VkRenderPass sceneRenderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> sceneFramebuffers_;
    
    // Bloom resources
    std::unique_ptr<VulkanImage> bloomImage_;
    VkImageView bloomImageView_ = VK_NULL_HANDLE;
    VkSampler bloomSampler_ = VK_NULL_HANDLE;
    VkPipeline bloomPipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout bloomPipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout bloomDescriptorLayout_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> bloomDescriptorSets_;
    
    // Final composite to swapchain
    VkRenderPass finalRenderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> finalFramebuffers_;
    VkPipeline finalPipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout finalPipelineLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout finalDescriptorLayout_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> finalDescriptorSets_;
    
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
};

} // namespace vibe::vk
