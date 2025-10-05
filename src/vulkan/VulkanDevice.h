#pragma once

#include "VulkanTypes.h"
#include <vector>
#include <string>
#include <string_view>
#include <optional>

struct GLFWwindow;

namespace vibe::vk {

class VulkanDevice {
public:
    VulkanDevice() = default;
    ~VulkanDevice();

    // Initialize Vulkan device
    bool initialize(GLFWwindow* window, bool enableValidation = true);
    void cleanup();

    // Getters
    [[nodiscard]] VkInstance getInstance() const noexcept { return instance_; }
    [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const noexcept { return physicalDevice_; }
    [[nodiscard]] VkDevice getDevice() const noexcept { return device_; }
    [[nodiscard]] VkSurfaceKHR getSurface() const noexcept { return surface_; }
    [[nodiscard]] VkQueue getGraphicsQueue() const noexcept { return graphicsQueue_; }
    [[nodiscard]] VkQueue getPresentQueue() const noexcept { return presentQueue_; }
    [[nodiscard]] VkQueue getComputeQueue() const noexcept { return computeQueue_; }
    [[nodiscard]] const QueueFamilyIndices& getQueueFamilies() const noexcept { return queueFamilies_; }
    [[nodiscard]] VkCommandPool getCommandPool() const noexcept { return commandPool_; }
    [[nodiscard]] VkPhysicalDeviceProperties getDeviceProperties() const noexcept { return deviceProperties_; }

    // Query support
    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport() const;
    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    // Command buffer utilities
    [[nodiscard]] VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    // Wait for device idle
    void waitIdle() const;

private:
    bool createInstance(bool enableValidation);
    bool setupDebugMessenger();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createCommandPool();

    [[nodiscard]] bool isDeviceSuitable(VkPhysicalDevice device) const;
    [[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    [[nodiscard]] bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    // Validation layers
    static constexpr std::array validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    // Device extensions
    static constexpr std::array deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME
    };

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    VkQueue computeQueue_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    
    QueueFamilyIndices queueFamilies_;
    VkPhysicalDeviceProperties deviceProperties_{};
    bool validationEnabled_ = false;
};

} // namespace vibe::vk
