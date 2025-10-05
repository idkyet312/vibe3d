#pragma once

#include "VulkanTypes.h"
#include "VulkanDevice.h"
#include <memory>

namespace vibe::vk {

class VulkanBuffer {
public:
    VulkanBuffer() = default;
    ~VulkanBuffer();

    bool create(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage,
                VkMemoryPropertyFlags properties);
    void cleanup();

    void copyFrom(const void* data, VkDeviceSize size);
    void* map();
    void unmap();

    [[nodiscard]] VkBuffer getBuffer() const noexcept { return buffer_; }
    [[nodiscard]] VkDeviceSize getSize() const noexcept { return size_; }

private:
    VulkanDevice* device_ = nullptr;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkDeviceSize size_ = 0;
    void* mapped_ = nullptr;
};

} // namespace vibe::vk
