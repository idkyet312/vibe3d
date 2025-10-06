#pragma once

#include "../VulkanTypes.h"
#include "../VulkanDevice.h"
#include "../VulkanSwapChain.h"
#include <memory>
#include <vector>
#include <string>

namespace vibe::vk {

/**
 * @brief Manages graphics pipelines, shader loading, and descriptor layouts
 * 
 * Handles:
 * - Shader module loading from SPIR-V
 * - Descriptor set layout creation
 * - Graphics pipeline creation
 * - Pipeline layout management
 */
class PipelineManager {
public:
    explicit PipelineManager(VulkanDevice& device);
    ~PipelineManager();

    // Initialization
    bool createDescriptorSetLayouts(size_t numCascades);
    bool createForwardPipeline(VulkanSwapChain& swapChain, VkRenderPass renderPass);
    void cleanup();

    // Getters
    [[nodiscard]] VkDescriptorSetLayout getGlobalDescriptorLayout() const noexcept {
        return globalDescriptorLayout_;
    }
    [[nodiscard]] VkPipeline getForwardPipeline() const noexcept { return forwardPipeline_; }
    [[nodiscard]] VkPipelineLayout getForwardPipelineLayout() const noexcept {
        return forwardPipelineLayout_;
    }

private:
    // Shader helpers
    std::vector<char> readShaderFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    VulkanDevice& device_;

    // Descriptor layouts
    VkDescriptorSetLayout globalDescriptorLayout_ = VK_NULL_HANDLE;

    // Forward rendering pipeline
    VkPipelineLayout forwardPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline forwardPipeline_ = VK_NULL_HANDLE;
};

} // namespace vibe::vk
