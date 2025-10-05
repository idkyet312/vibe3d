#include "VulkanBuffer.h"
#include <cstring>

namespace vibe::vk {

VulkanBuffer::~VulkanBuffer() {
    cleanup();
}

bool VulkanBuffer::create(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties) {
    device_ = &device;
    size_ = size;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(device.getDevice(), &bufferInfo, nullptr, &buffer_) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.getDevice(), buffer_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties)
    };

    if (vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(device.getDevice(), buffer_, memory_, 0);
    return true;
}

void VulkanBuffer::cleanup() {
    if (device_) {
        if (mapped_) {
            unmap();
        }
        if (buffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_->getDevice(), buffer_, nullptr);
        }
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_->getDevice(), memory_, nullptr);
        }
    }
}

void VulkanBuffer::copyFrom(const void* data, VkDeviceSize size) {
    void* mapped = map();
    std::memcpy(mapped, data, size);
    unmap();
}

void* VulkanBuffer::map() {
    if (!mapped_ && device_) {
        vkMapMemory(device_->getDevice(), memory_, 0, size_, 0, &mapped_);
    }
    return mapped_;
}

void VulkanBuffer::unmap() {
    if (mapped_ && device_) {
        vkUnmapMemory(device_->getDevice(), memory_);
        mapped_ = nullptr;
    }
}

} // namespace vibe::vk
