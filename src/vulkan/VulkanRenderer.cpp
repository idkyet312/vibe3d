#include "VulkanRenderer.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace vibe::vk {

VulkanRenderer::~VulkanRenderer() {
    cleanup();
}

bool VulkanRenderer::initialize(GLFWwindow* window, uint32_t width, uint32_t height) {
    std::cout << "Attempting to initialize Vulkan renderer...\n";
    
    device_ = std::make_unique<VulkanDevice>();
    if (!device_->initialize(window, true)) {
        std::cerr << "Failed to initialize Vulkan device\n";
        return false;
    }
    
    swapChain_ = std::make_unique<VulkanSwapChain>();
    if (!swapChain_->create(*device_, width, height)) {
        std::cerr << "Failed to create swap chain\n";
        return false;
    }
    
    initialized_ = true;
    std::cout << "Vulkan renderer initialized successfully!\n";
    return true;
}

void VulkanRenderer::cleanup() {
    if (device_) {
        device_->waitIdle();
    }
    
    swapChain_.reset();
    device_.reset();
    initialized_ = false;
}

bool VulkanRenderer::beginFrame() {
    return initialized_;
}

void VulkanRenderer::endFrame() {
}

} // namespace vibe::vk
