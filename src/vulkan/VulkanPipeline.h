#pragma once
#include "VulkanTypes.h"
#include "VulkanDevice.h"
#include <vector>
#include <string>

namespace vibe::vk {

class VulkanPipeline {
public:
    VulkanPipeline() = default;
    ~VulkanPipeline() { cleanup(); }
    
    bool createGraphics(VulkanDevice& device, const std::string& vertPath, 
                       const std::string& fragPath, VkRenderPass renderPass);
    bool createCompute(VulkanDevice& device, const std::string& compPath);
    void cleanup();
    
    [[nodiscard]] VkPipeline getPipeline() const noexcept { return pipeline_; }
    [[nodiscard]] VkPipelineLayout getLayout() const noexcept { return layout_; }
    
private:
    std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    
    VulkanDevice* device_ = nullptr;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
};

} // namespace vibe::vk
