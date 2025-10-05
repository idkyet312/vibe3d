#include "VulkanDescriptor.h"

namespace vibe::vk {

bool VulkanDescriptorPool::create(VulkanDevice& device, uint32_t maxSets) {
    device_ = &device;
    
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxSets * 4},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxSets * 4},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxSets * 4}
    };
    
    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    
    return vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &pool_) == VK_SUCCESS;
}

void VulkanDescriptorPool::cleanup() {
    if (device_ && pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_->getDevice(), pool_, nullptr);
    }
}

} // namespace vibe::vk
