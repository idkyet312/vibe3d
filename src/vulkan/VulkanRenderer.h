#pragma once

#include "VulkanTypes.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include <memory>

struct GLFWwindow;

namespace vibe::vk {

class VulkanRenderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer();

    bool initialize(GLFWwindow* window, uint32_t width, uint32_t height);
    void cleanup();
    
    bool beginFrame();
    void endFrame();
    
    [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }
    
private:
    bool initialized_ = false;
    std::unique_ptr<VulkanDevice> device_;
    std::unique_ptr<VulkanSwapChain> swapChain_;
};

} // namespace vibe::vk
