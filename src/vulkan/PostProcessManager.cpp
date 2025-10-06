#include "PostProcessManager.h"
#include "VulkanBuffer.h"
#include <iostream>
#include <fstream>
#include <array>

namespace vibe::vk {

PostProcessManager::PostProcessManager(VulkanDevice& device)
    : device_(device) {
}

PostProcessManager::~PostProcessManager() {
    cleanup();
}

bool PostProcessManager::initialize(const Config& config) {
    config_ = config;
    
    std::cout << "Initializing post-processing pipeline..." << std::endl;
    
    if (!createSceneRenderTarget()) {
        std::cerr << "Failed to create scene render target" << std::endl;
        return false;
    }
    
    if (!createSceneRenderPass()) {
        std::cerr << "Failed to create scene render pass" << std::endl;
        return false;
    }
    
    if (!createBloomResources()) {
        std::cerr << "Failed to create bloom resources" << std::endl;
        return false;
    }
    
    if (!createBloomPipeline()) {
        std::cerr << "Failed to create bloom pipeline" << std::endl;
        return false;
    }
    
    if (!createFinalRenderPass()) {
        std::cerr << "Failed to create final render pass" << std::endl;
        return false;
    }
    
    if (!createFinalPipeline()) {
        std::cerr << "Failed to create final pipeline" << std::endl;
        return false;
    }
    
    if (!createDescriptorSets()) {
        std::cerr << "Failed to create descriptor sets" << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "Post-processing pipeline initialized successfully" << std::endl;
    return true;
}

void PostProcessManager::cleanup() {
    if (!initialized_) return;
    
    auto dev = device_.getDevice();
    device_.waitIdle();
    
    // Cleanup descriptor pool
    if (descriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(dev, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
    }
    
    // Cleanup final pass
    if (finalPipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(dev, finalPipeline_, nullptr);
        finalPipeline_ = VK_NULL_HANDLE;
    }
    if (finalPipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(dev, finalPipelineLayout_, nullptr);
        finalPipelineLayout_ = VK_NULL_HANDLE;
    }
    if (finalDescriptorLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(dev, finalDescriptorLayout_, nullptr);
        finalDescriptorLayout_ = VK_NULL_HANDLE;
    }
    for (auto fb : finalFramebuffers_) {
        vkDestroyFramebuffer(dev, fb, nullptr);
    }
    finalFramebuffers_.clear();
    if (finalRenderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(dev, finalRenderPass_, nullptr);
        finalRenderPass_ = VK_NULL_HANDLE;
    }
    
    // Cleanup bloom
    if (bloomPipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(dev, bloomPipeline_, nullptr);
        bloomPipeline_ = VK_NULL_HANDLE;
    }
    if (bloomPipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(dev, bloomPipelineLayout_, nullptr);
        bloomPipelineLayout_ = VK_NULL_HANDLE;
    }
    if (bloomDescriptorLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(dev, bloomDescriptorLayout_, nullptr);
        bloomDescriptorLayout_ = VK_NULL_HANDLE;
    }
    if (bloomSampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(dev, bloomSampler_, nullptr);
        bloomSampler_ = VK_NULL_HANDLE;
    }
    if (bloomImageView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(dev, bloomImageView_, nullptr);
        bloomImageView_ = VK_NULL_HANDLE;
    }
    bloomImage_.reset();
    
    // Cleanup scene
    for (auto fb : sceneFramebuffers_) {
        vkDestroyFramebuffer(dev, fb, nullptr);
    }
    sceneFramebuffers_.clear();
    if (sceneRenderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(dev, sceneRenderPass_, nullptr);
        sceneRenderPass_ = VK_NULL_HANDLE;
    }
    if (sceneColorImageView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(dev, sceneColorImageView_, nullptr);
        sceneColorImageView_ = VK_NULL_HANDLE;
    }
    sceneColorImage_.reset();
    
    initialized_ = false;
}

bool PostProcessManager::createSceneRenderTarget() {
    // Create HDR render target for scene
    sceneColorImage_ = std::make_unique<VulkanImage>();
    if (!sceneColorImage_->create(device_, config_.width, config_.height,
                                   VK_FORMAT_R16G16B16A16_SFLOAT,
                                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)) {
        return false;
    }
    
    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = sceneColorImage_->getImage(),
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    
    return vkCreateImageView(device_.getDevice(), &viewInfo, nullptr, &sceneColorImageView_) == VK_SUCCESS;
}

bool PostProcessManager::createSceneRenderPass() {
    // HDR color attachment
    VkAttachmentDescription colorAttachment{
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    
    // Depth attachment
    VkAttachmentDescription depthAttachment{
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    
    VkAttachmentReference colorRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    
    VkAttachmentReference depthRef{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    
    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorRef,
        .pDepthStencilAttachment = &depthRef
    };
    
    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };
    
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    
    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    
    return vkCreateRenderPass(device_.getDevice(), &renderPassInfo, nullptr, &sceneRenderPass_) == VK_SUCCESS;
}

bool PostProcessManager::createSceneFramebuffers(VkImageView depthView) {
    // This will be called with the depth buffer from the main renderer
    // For now, create a placeholder - will be updated when integrated
    return true;
}

bool PostProcessManager::createBloomResources() {
    // Create bloom temp image (half resolution)
    uint32_t bloomWidth = config_.width / 2;
    uint32_t bloomHeight = config_.height / 2;
    
    bloomImage_ = std::make_unique<VulkanImage>();
    if (!bloomImage_->create(device_, bloomWidth, bloomHeight,
                            VK_FORMAT_R16G16B16A16_SFLOAT,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)) {
        return false;
    }
    
    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = bloomImage_->getImage(),
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    
    if (vkCreateImageView(device_.getDevice(), &viewInfo, nullptr, &bloomImageView_) != VK_SUCCESS) {
        return false;
    }
    
    // Create sampler
    VkSamplerCreateInfo samplerInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .minLod = 0.0f,
        .maxLod = 1.0f
    };
    
    return vkCreateSampler(device_.getDevice(), &samplerInfo, nullptr, &bloomSampler_) == VK_SUCCESS;
}

bool PostProcessManager::createBloomPipeline() {
    // Create descriptor set layout for bloom shader
    VkDescriptorSetLayoutBinding samplerBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &samplerBinding
    };
    
    if (vkCreateDescriptorSetLayout(device_.getDevice(), &layoutInfo, nullptr, &bloomDescriptorLayout_) != VK_SUCCESS) {
        return false;
    }
    
    // Create pipeline layout with push constants for bloom parameters
    VkPushConstantRange pushConstant{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(float) * 3 // threshold, intensity, radius
    };
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &bloomDescriptorLayout_,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstant
    };
    
    return vkCreatePipelineLayout(device_.getDevice(), &pipelineLayoutInfo, nullptr, &bloomPipelineLayout_) == VK_SUCCESS;
}

bool PostProcessManager::createFinalRenderPass() {
    // Final pass renders to swapchain
    VkAttachmentDescription colorAttachment{
        .format = VK_FORMAT_B8G8R8A8_SRGB, // Will be updated based on swapchain format
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    
    VkAttachmentReference colorRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    
    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorRef
    };
    
    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };
    
    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    
    return vkCreateRenderPass(device_.getDevice(), &renderPassInfo, nullptr, &finalRenderPass_) == VK_SUCCESS;
}

bool PostProcessManager::createFinalFramebuffers(const std::vector<VkImageView>& swapchainViews) {
    finalFramebuffers_.resize(swapchainViews.size());
    
    for (size_t i = 0; i < swapchainViews.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = finalRenderPass_,
            .attachmentCount = 1,
            .pAttachments = &swapchainViews[i],
            .width = config_.width,
            .height = config_.height,
            .layers = 1
        };
        
        if (vkCreateFramebuffer(device_.getDevice(), &framebufferInfo, nullptr, &finalFramebuffers_[i]) != VK_SUCCESS) {
            return false;
        }
    }
    
    return true;
}

bool PostProcessManager::createFinalPipeline() {
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding samplerBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &samplerBinding
    };
    
    if (vkCreateDescriptorSetLayout(device_.getDevice(), &layoutInfo, nullptr, &finalDescriptorLayout_) != VK_SUCCESS) {
        return false;
    }
    
    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &finalDescriptorLayout_
    };
    
    return vkCreatePipelineLayout(device_.getDevice(), &pipelineLayoutInfo, nullptr, &finalPipelineLayout_) == VK_SUCCESS;
}

bool PostProcessManager::createDescriptorSets() {
    // Create descriptor pool
    std::array<VkDescriptorPoolSize, 1> poolSizes{{
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}
    }};
    
    VkDescriptorPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 10,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    
    return vkCreateDescriptorPool(device_.getDevice(), &poolInfo, nullptr, &descriptorPool_) == VK_SUCCESS;
}

void PostProcessManager::applyPostProcessing(VkCommandBuffer cmd, VkImageView depthView, uint32_t swapchainImageIndex) {
    // Apply bloom and composite to swapchain
    // This is a placeholder for the full implementation
}

void PostProcessManager::onResize(uint32_t width, uint32_t height) {
    config_.width = width;
    config_.height = height;
    // TODO: Recreate resources with new size
}

} // namespace vibe::vk
