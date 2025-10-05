#pragma once
#include "VulkanTypes.h"
#include "VulkanDevice.h"
#include <vector>

namespace vibe::vk {

class VulkanDescriptorPool {
public:
    VulkanDescriptorPool() = default;
    ~VulkanDescriptorPool() { cleanup(); }
    
    bool create(VulkanDevice& device, uint32_t maxSets);
    void cleanup();
    
    [[nodiscard]] VkDescriptorPool getPool() const noexcept { return pool_; }
    
private:
    VulkanDevice* device_ = nullptr;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
};

} // namespace vibe::vk
