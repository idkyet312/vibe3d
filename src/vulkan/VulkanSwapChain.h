#pragma once

#include "VulkanTypes.h"
#include "VulkanDevice.h"

namespace vibe::vk {

class VulkanSwapChain {
public:
    VulkanSwapChain() = default;
    ~VulkanSwapChain();

    bool create(VulkanDevice& device, uint32_t width, uint32_t height);
    void cleanup();

    [[nodiscard]] VkSwapchainKHR getSwapChain() const noexcept { return swapChain_; }
    [[nodiscard]] VkFormat getImageFormat() const noexcept { return imageFormat_; }
    [[nodiscard]] VkExtent2D getExtent() const noexcept { return extent_; }
    [[nodiscard]] const std::vector<VkImageView>& getImageViews() const noexcept { return imageViews_; }

    uint32_t acquireNextImage(VkSemaphore semaphore);
    void present(uint32_t imageIndex, VkSemaphore waitSemaphore);

private:
    VulkanDevice* device_ = nullptr;
    VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    VkFormat imageFormat_;
    VkExtent2D extent_;
};

} // namespace vibe::vk
