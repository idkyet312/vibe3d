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

std::vector<char> PostProcessManager::readShaderFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        return {};
    }
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    return buffer;
}

VkShaderModule PostProcessManager::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    
    return shaderModule;
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
    
    if (vkCreateImageView(device_.getDevice(), &viewInfo, nullptr, &sceneColorImageView_) != VK_SUCCESS) {
        return false;
    }
    
    // Transition image to COLOR_ATTACHMENT_OPTIMAL layout
    VkCommandBuffer cmd = device_.beginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = sceneColorImage_->getImage(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    device_.endSingleTimeCommands(cmd);
    
    return true;
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
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL  // Keep in COLOR_ATTACHMENT for now
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
    // Create framebuffers for scene rendering (HDR render target + depth)
    // We need one framebuffer per swapchain image (even though we render to the same HDR target)
    // This is for consistency with the rendering loop
    
    // Get swapchain image count (we'll create one framebuffer per swapchain image)
    sceneFramebuffers_.resize(3); // Standard 3 frames in flight
    
    for (size_t i = 0; i < sceneFramebuffers_.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            sceneColorImageView_,  // HDR color attachment
            depthView               // Depth attachment (shared with main renderer)
        };
        
        VkFramebufferCreateInfo framebufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = sceneRenderPass_,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = config_.width,
            .height = config_.height,
            .layers = 1
        };
        
        if (vkCreateFramebuffer(device_.getDevice(), &framebufferInfo, nullptr, &sceneFramebuffers_[i]) != VK_SUCCESS) {
            return false;
        }
    }
    
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
        .size = sizeof(float) * 4 // threshold, intensity, radius, padding
    };
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &bloomDescriptorLayout_,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstant
    };
    
    if (vkCreatePipelineLayout(device_.getDevice(), &pipelineLayoutInfo, nullptr, &bloomPipelineLayout_) != VK_SUCCESS) {
        return false;
    }
    
    // Load shaders
    auto vertShaderCode = readShaderFile("shaders/fullscreen.vert.spv");
    auto fragShaderCode = readShaderFile("shaders/bloom.frag.spv");
    
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    
    VkPipelineShaderStageCreateInfo vertStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo fragStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};
    
    // No vertex input (fullscreen triangle generated in vertex shader)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0
    };
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    
    VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(config_.width),
        .height = static_cast<float>(config_.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    
    VkRect2D scissor{
        .offset = {0, 0},
        .extent = {config_.width, config_.height}
    };
    
    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    
    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };
    
    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    
    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };
    
    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .layout = bloomPipelineLayout_,
        .renderPass = finalRenderPass_,
        .subpass = 0
    };
    
    VkResult result = vkCreateGraphicsPipelines(device_.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &bloomPipeline_);
    
    vkDestroyShaderModule(device_.getDevice(), vertShaderModule, nullptr);
    vkDestroyShaderModule(device_.getDevice(), fragShaderModule, nullptr);
    
    return result == VK_SUCCESS;
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
    
    if (vkCreateDescriptorPool(device_.getDevice(), &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        return false;
    }
    
    // Allocate descriptor sets for bloom (one per swapchain image)
    bloomDescriptorSets_.resize(3); // 3 frames in flight
    
    std::vector<VkDescriptorSetLayout> layouts(bloomDescriptorSets_.size(), bloomDescriptorLayout_);
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool_,
        .descriptorSetCount = static_cast<uint32_t>(bloomDescriptorSets_.size()),
        .pSetLayouts = layouts.data()
    };
    
    if (vkAllocateDescriptorSets(device_.getDevice(), &allocInfo, bloomDescriptorSets_.data()) != VK_SUCCESS) {
        return false;
    }
    
    // Update descriptor sets to point to scene color image
    for (size_t i = 0; i < bloomDescriptorSets_.size(); i++) {
        VkDescriptorImageInfo imageInfo{
            .sampler = bloomSampler_,
            .imageView = sceneColorImageView_,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        
        VkWriteDescriptorSet descriptorWrite{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = bloomDescriptorSets_[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo
        };
        
        vkUpdateDescriptorSets(device_.getDevice(), 1, &descriptorWrite, 0, nullptr);
    }
    
    return true;
}

bool PostProcessManager::setupFramebuffers(VkImageView depthView, const std::vector<VkImageView>& swapchainViews) {
    if (!createSceneFramebuffers(depthView)) {
        std::cerr << "Failed to create scene framebuffers" << std::endl;
        return false;
    }
    
    if (!createFinalFramebuffers(swapchainViews)) {
        std::cerr << "Failed to create final framebuffers" << std::endl;
        return false;
    }
    
    return true;
}

void PostProcessManager::applyPostProcessing(VkCommandBuffer cmd, VkImageView depthView, uint32_t swapchainImageIndex) {
    if (!config_.enableBloom) {
        // If bloom is disabled, just copy scene to swapchain
        // For now, we'll skip this and just let the scene render directly
        return;
    }
    
    // Bloom pass: render fullscreen quad with bloom shader
    // The bloom shader samples from the scene HDR texture and applies bloom
    
    // Transition scene image for shader reading
    VkImageMemoryBarrier sceneBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = sceneColorImage_->getImage(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &sceneBarrier);
    
    // Begin final render pass (renders to swapchain)
    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = finalRenderPass_,
        .framebuffer = finalFramebuffers_[swapchainImageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = {config_.width, config_.height}
        }
    };
    
    VkClearValue clearValue{};
    clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;
    
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Bind bloom pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomPipeline_);
    
    // Bind descriptor set (scene texture)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomPipelineLayout_,
                           0, 1, &bloomDescriptorSets_[swapchainImageIndex], 0, nullptr);
    
    // Push constants for bloom parameters
    struct BloomPushConstants {
        float threshold;
        float intensity;
        float radius;
        float padding;
    } pushConstants{
        .threshold = config_.bloomThreshold,
        .intensity = config_.bloomIntensity,
        .radius = config_.bloomRadius,
        .padding = 0.0f
    };
    
    vkCmdPushConstants(cmd, bloomPipelineLayout_, VK_SHADER_STAGE_FRAGMENT_BIT,
                      0, sizeof(BloomPushConstants), &pushConstants);
    
    // Draw fullscreen triangle (3 vertices, no vertex buffer needed - generated in vertex shader)
    vkCmdDraw(cmd, 3, 1, 0, 0);
    
    // NOTE: Do NOT end the render pass here - let the caller continue rendering (e.g., ImGui)
    // vkCmdEndRenderPass(cmd);
    
    // Transition scene image back to color attachment for next frame
    // Actually, skip this since we'll do it at the beginning of next frame
    /*
    sceneBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sceneBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    sceneBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    sceneBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, 0, nullptr, 0, nullptr, 1, &sceneBarrier);
    */
}

void PostProcessManager::onResize(uint32_t width, uint32_t height) {
    config_.width = width;
    config_.height = height;
    // TODO: Recreate resources with new size
}

} // namespace vibe::vk
