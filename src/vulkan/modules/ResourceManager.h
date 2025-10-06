#pragma once

#include "../VulkanTypes.h"
#include "../VulkanDevice.h"
#include "../VulkanBuffer.h"
#include "../VulkanDescriptor.h"
#include <memory>
#include <array>
#include <vector>

namespace vibe::vk {

/**
 * @brief Manages all rendering resources (buffers, descriptors, images)
 * 
 * Handles:
 * - Uniform buffer creation and management
 * - Descriptor pool and set allocation
 * - Vertex and index buffer management
 * - Descriptor set updates
 */
class ResourceManager {
public:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    explicit ResourceManager(VulkanDevice& device);
    ~ResourceManager();

    // Initialization
    bool initialize(VkDescriptorSetLayout globalLayout, VkSampler shadowSampler,
                   const std::array<VkImageView, 4>& shadowImageViews);
    void cleanup();

    // Buffer management
    bool createUniformBuffers();
    bool createGeometryBuffers(std::span<const Vertex> vertices, std::span<const uint32_t> indices);
    void updateCameraBuffer(const CameraUBO& camera, uint32_t currentFrame);
    void updateShadowBuffer(const ShadowUBO& shadowData, uint32_t currentFrame);

    // Descriptor management
    bool createDescriptorPool();
    bool allocateDescriptorSets(VkDescriptorSetLayout layout);
    void updateDescriptorSets(VkSampler shadowSampler, 
                             const std::array<VkImageView, 4>& shadowImageViews);

    // Getters
    [[nodiscard]] VkBuffer getVertexBuffer() const noexcept { return vertexBuffer_->getBuffer(); }
    [[nodiscard]] VkBuffer getIndexBuffer() const noexcept { return indexBuffer_->getBuffer(); }
    [[nodiscard]] const std::vector<VkDescriptorSet>& getGlobalDescriptorSets() const noexcept {
        return globalDescriptorSets_;
    }
    [[nodiscard]] uint32_t getIndexCount() const noexcept { return indexCount_; }

private:
    VulkanDevice& device_;
    uint32_t indexCount_ = 0;

    // Buffers
    std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> cameraBuffers_;
    std::array<std::unique_ptr<VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> shadowBuffers_;
    std::unique_ptr<VulkanBuffer> vertexBuffer_;
    std::unique_ptr<VulkanBuffer> indexBuffer_;

    // Descriptors
    std::unique_ptr<VulkanDescriptorPool> descriptorPool_;
    std::vector<VkDescriptorSet> globalDescriptorSets_;
};

} // namespace vibe::vk
