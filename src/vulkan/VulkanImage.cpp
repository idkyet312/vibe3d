#include "VulkanImage.h"

namespace vibe::vk {

bool VulkanImage::create(VulkanDevice& device, uint32_t width, uint32_t height, 
                        VkFormat format, VkImageUsageFlags usage) {
    device_ = &device;
    
    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    
    if (vkCreateImage(device.getDevice(), &imageInfo, nullptr, &image_) != VK_SUCCESS) {
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.getDevice(), image_, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, 
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };
    
    if (vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
        return false;
    }
    
    vkBindImageMemory(device.getDevice(), image_, memory_, 0);
    return true;
}

void VulkanImage::cleanup() {
    if (device_) {
        if (image_ != VK_NULL_HANDLE) {
            vkDestroyImage(device_->getDevice(), image_, nullptr);
        }
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_->getDevice(), memory_, nullptr);
        }
    }
}

} // namespace vibe::vk
