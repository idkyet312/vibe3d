#include "VulkanPipeline.h"
#include <fstream>
#include <stdexcept>

namespace vibe::vk {

std::vector<char> VulkanPipeline::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    size_t fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

VkShaderModule VulkanPipeline::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
    
    return shaderModule;
}

bool VulkanPipeline::createGraphics(VulkanDevice& device, const std::string& vertPath,
                                   const std::string& fragPath, VkRenderPass renderPass) {
    device_ = &device;
    // Simplified - just create basic layout
    VkPipelineLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };
    
    return vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &layout_) == VK_SUCCESS;
}

bool VulkanPipeline::createCompute(VulkanDevice& device, const std::string& compPath) {
    device_ = &device;
    VkPipelineLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
    };
    
    return vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &layout_) == VK_SUCCESS;
}

void VulkanPipeline::cleanup() {
    if (device_) {
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_->getDevice(), pipeline_, nullptr);
        }
        if (layout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_->getDevice(), layout_, nullptr);
        }
    }
}

} // namespace vibe::vk
