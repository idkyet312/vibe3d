#pragma once
#include "VulkanTypes.h"
#include "VulkanDevice.h"

namespace vibe::vk {

class VulkanImage {
public:
    VulkanImage() = default;
    ~VulkanImage() { cleanup(); }
    
    bool create(VulkanDevice& device, uint32_t width, uint32_t height, VkFormat format, 
                VkImageUsageFlags usage);
    void cleanup();
    
    [[nodiscard]] VkImage getImage() const noexcept { return image_; }
    
private:
    VulkanDevice* device_ = nullptr;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
};

} // namespace vibe::vk
