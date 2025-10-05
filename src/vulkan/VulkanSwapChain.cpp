#include "VulkanSwapChain.h"
#include <algorithm>

namespace vibe::vk {

VulkanSwapChain::~VulkanSwapChain() {
    cleanup();
}

bool VulkanSwapChain::create(VulkanDevice& device, uint32_t width, uint32_t height) {
    device_ = &device;
    
    auto swapChainSupport = device.querySwapChainSupport();
    
    VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
    for (const auto& format : swapChainSupport.formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && 
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = format;
            break;
        }
    }
    
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& mode : swapChainSupport.presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mode;
            break;
        }
    }
    
    extent_ = {width, height};
    if (swapChainSupport.capabilities.currentExtent.width != UINT32_MAX) {
        extent_ = swapChainSupport.capabilities.currentExtent;
    }
    
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, swapChainSupport.capabilities.maxImageCount);
    }
    
    VkSwapchainCreateInfoKHR createInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = device.getSurface(),
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent_,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE
    };
    
    if (vkCreateSwapchainKHR(device.getDevice(), &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
        return false;
    }
    
    imageFormat_ = surfaceFormat.format;
    
    vkGetSwapchainImagesKHR(device.getDevice(), swapChain_, &imageCount, nullptr);
    images_.resize(imageCount);
    vkGetSwapchainImagesKHR(device.getDevice(), swapChain_, &imageCount, images_.data());
    
    imageViews_.resize(images_.size());
    for (size_t i = 0; i < images_.size(); i++) {
        VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = images_[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imageFormat_,
            .components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        
        if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &imageViews_[i]) != VK_SUCCESS) {
            return false;
        }
    }
    
    return true;
}

void VulkanSwapChain::cleanup() {
    if (device_) {
        for (auto imageView : imageViews_) {
            vkDestroyImageView(device_->getDevice(), imageView, nullptr);
        }
        if (swapChain_ != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device_->getDevice(), swapChain_, nullptr);
        }
    }
}

uint32_t VulkanSwapChain::acquireNextImage(VkSemaphore semaphore) {
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device_->getDevice(), swapChain_, UINT64_MAX, 
                         semaphore, VK_NULL_HANDLE, &imageIndex);
    return imageIndex;
}

void VulkanSwapChain::present(uint32_t imageIndex, VkSemaphore waitSemaphore) {
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapChain_,
        .pImageIndices = &imageIndex
    };
    
    vkQueuePresentKHR(device_->getPresentQueue(), &presentInfo);
}

} // namespace vibe::vk
