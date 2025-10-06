#include "ForwardPlusRenderer.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>

namespace vibe::vk {

ForwardPlusRenderer::ForwardPlusRenderer(const Config& config)
    : config_(config) {
}

ForwardPlusRenderer::~ForwardPlusRenderer() {
    cleanup();
}

bool ForwardPlusRenderer::initialize(GLFWwindow* window) {
    std::cout << "Initializing Forward+ renderer..." << std::endl;
    
    device_ = std::make_unique<VulkanDevice>();
    if (!device_->initialize(window, false)) { // Disable validation for better performance
        std::cerr << "Failed to initialize Vulkan device for Forward+" << std::endl;
        return false;
    }
    
    swapChain_ = std::make_unique<VulkanSwapChain>();
    if (!swapChain_->create(*device_, config_.width, config_.height)) {
        std::cerr << "Failed to create swap chain for Forward+" << std::endl;
        return false;
    }
    
    numTilesX_ = calculateNumTiles(config_.width);
    numTilesY_ = calculateNumTiles(config_.height);
    
    lightGrid_.numTilesX = numTilesX_;
    lightGrid_.numTilesY = numTilesY_;
    lightGrid_.maxLightsPerTile = config_.maxLights;
    
    // Calculate cascade splits for shadow mapping
    calculateCascadeSplits();
    
    if (!createDepthResources()) {
        std::cerr << "Failed to create depth resources" << std::endl;
        return false;
    }
    
    if (!createShadowResources()) {
        std::cerr << "Failed to create shadow resources" << std::endl;
        return false;
    }
    
    if (!createShadowRenderPass()) {
        std::cerr << "Failed to create shadow render pass" << std::endl;
        return false;
    }
    
    if (!createShadowFramebuffers()) {
        std::cerr << "Failed to create shadow framebuffers" << std::endl;
        return false;
    }
    
    if (!createShadowSampler()) {
        std::cerr << "Failed to create shadow sampler" << std::endl;
        return false;
    }
    
    if (!createShadowPipeline()) {
        std::cerr << "Failed to create shadow pipeline" << std::endl;
        return false;
    }
    
    if (!createRenderPass()) {
        std::cerr << "Failed to create render pass" << std::endl;
        return false;
    }
    
    if (!createFramebuffers()) {
        std::cerr << "Failed to create framebuffers" << std::endl;
        return false;
    }
    
    if (!createDescriptorSetLayouts()) {
        std::cerr << "Failed to create descriptor layouts" << std::endl;
        return false;
    }
    
    if (!createBuffers()) {
        std::cerr << "Failed to create buffers" << std::endl;
        return false;
    }
    
    if (!createPipeline()) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return false;
    }
    
    if (!createCubeGeometry()) {
        std::cerr << "Failed to create cube geometry" << std::endl;
        return false;
    }
    
    if (!createSyncObjects()) {
        std::cerr << "Failed to create sync objects" << std::endl;
        return false;
    }
    
    if (!createCommandBuffers()) {
        std::cerr << "Failed to create command buffers" << std::endl;
        return false;
    }
    
    std::cout << "Forward+ renderer initialized: " << numTilesX_ << "x" << numTilesY_ << " tiles" << std::endl;
    
    initialized_ = true;
    return true;
}

void ForwardPlusRenderer::cleanup() {
    if (device_) {
        device_->waitIdle();
        
        // Clean up shadow resources
        if (shadowSampler_ != VK_NULL_HANDLE) {
            vkDestroySampler(device_->getDevice(), shadowSampler_, nullptr);
        }
        
        for (size_t i = 0; i < NUM_CASCADES; ++i) {
            if (shadowImageViews_[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(device_->getDevice(), shadowImageViews_[i], nullptr);
            }
            shadowImages_[i].reset();
            
            for (auto fb : shadowFramebuffers_[i]) {
                if (fb != VK_NULL_HANDLE) {
                    vkDestroyFramebuffer(device_->getDevice(), fb, nullptr);
                }
            }
        }
        
        if (shadowRenderPass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_->getDevice(), shadowRenderPass_, nullptr);
        }
        
        if (shadowPipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_->getDevice(), shadowPipeline_, nullptr);
        }
        
        if (shadowPipelineLayout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_->getDevice(), shadowPipelineLayout_, nullptr);
        }
        
        // Clean up depth resources
        if (depthImageView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_->getDevice(), depthImageView_, nullptr);
        }
        depthImage_.reset();
        
        // Clean up buffers
        vertexBuffer_.reset();
        indexBuffer_.reset();
        for (auto& buffer : cameraBuffers_) {
            buffer.reset();
        }
        for (auto& buffer : shadowBuffers_) {
            buffer.reset();
        }
        
        // Clean up descriptor pool
        descriptorPool_.reset();
        
        // Clean up descriptor layouts
        if (globalDescriptorLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_->getDevice(), globalDescriptorLayout_, nullptr);
        }
        
        // Clean up pipelines
        if (forwardPipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_->getDevice(), forwardPipeline_, nullptr);
        }
        if (forwardPipelineLayout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_->getDevice(), forwardPipelineLayout_, nullptr);
        }
        
        // Clean up sync objects
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (renderFinishedSemaphores_[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device_->getDevice(), renderFinishedSemaphores_[i], nullptr);
            }
            if (imageAvailableSemaphores_[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device_->getDevice(), imageAvailableSemaphores_[i], nullptr);
            }
            if (inFlightFences_[i] != VK_NULL_HANDLE) {
                vkDestroyFence(device_->getDevice(), inFlightFences_[i], nullptr);
            }
        }
        
        // Clean up command pool
        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_->getDevice(), commandPool_, nullptr);
        }
        
        // Clean up framebuffers
        for (auto framebuffer : framebuffers_) {
            vkDestroyFramebuffer(device_->getDevice(), framebuffer, nullptr);
        }
        
        // Clean up render pass
        if (renderPass_ != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device_->getDevice(), renderPass_, nullptr);
        }
    }
    
    swapChain_.reset();
    device_.reset();
    
    initialized_ = false;
}

bool ForwardPlusRenderer::createRenderPass() {
    // Color attachment
    VkAttachmentDescription colorAttachment{
        .format = swapChain_->getImageFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    // Depth attachment
    VkAttachmentDescription depthAttachment{
        .format = findDepthFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
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

    return vkCreateRenderPass(device_->getDevice(), &renderPassInfo, nullptr, &renderPass_) == VK_SUCCESS;
}

bool ForwardPlusRenderer::createFramebuffers() {
    const auto& imageViews = swapChain_->getImageViews();
    framebuffers_.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            imageViews[i],
            depthImageView_
        };

        VkFramebufferCreateInfo framebufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass_,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swapChain_->getExtent().width,
            .height = swapChain_->getExtent().height,
            .layers = 1
        };

        if (vkCreateFramebuffer(device_->getDevice(), &framebufferInfo, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool ForwardPlusRenderer::createDescriptorSetLayouts() {
    // Camera UBO descriptor layout binding 0
    VkDescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };
    
    // Shadow UBO descriptor layout binding 1
    VkDescriptorSetLayoutBinding shadowUboLayoutBinding{
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };
    
    // Shadow map sampler binding 2
    VkDescriptorSetLayoutBinding shadowMapBinding{
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(NUM_CASCADES),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr
    };

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
        uboLayoutBinding, 
        shadowUboLayoutBinding, 
        shadowMapBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    return vkCreateDescriptorSetLayout(device_->getDevice(), &layoutInfo, nullptr, 
                                      &globalDescriptorLayout_) == VK_SUCCESS;
}

bool ForwardPlusRenderer::createBuffers() {
    // Create uniform buffers for camera
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        cameraBuffers_[i] = std::make_unique<VulkanBuffer>();
        if (!cameraBuffers_[i]->create(*device_, sizeof(CameraUBO),
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            return false;
        }
        
        shadowBuffers_[i] = std::make_unique<VulkanBuffer>();
        if (!shadowBuffers_[i]->create(*device_, sizeof(ShadowUBO),
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            return false;
        }
    }
    
    // Create descriptor pool
    descriptorPool_ = std::make_unique<VulkanDescriptorPool>();
    if (!descriptorPool_->create(*device_, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 10)) {
        return false;
    }
    
    // Allocate descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, globalDescriptorLayout_);
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool_->getPool(),
        .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        .pSetLayouts = layouts.data()
    };
    
    globalDescriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device_->getDevice(), &allocInfo, 
                                 globalDescriptorSets_.data()) != VK_SUCCESS) {
        return false;
    }
    
    // Update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{
            .buffer = cameraBuffers_[i]->getBuffer(),
            .offset = 0,
            .range = sizeof(CameraUBO)
        };
        
        VkDescriptorBufferInfo shadowBufferInfo{
            .buffer = shadowBuffers_[i]->getBuffer(),
            .offset = 0,
            .range = sizeof(ShadowUBO)
        };
        
        // Shadow map image infos
        std::array<VkDescriptorImageInfo, NUM_CASCADES> shadowImageInfos;
        for (size_t j = 0; j < NUM_CASCADES; ++j) {
            shadowImageInfos[j] = {
                .sampler = shadowSampler_,
                .imageView = shadowImageViews_[j],
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            };
        }
        
        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        
        descriptorWrites[0] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = globalDescriptorSets_[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo
        };
        
        descriptorWrites[1] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = globalDescriptorSets_[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &shadowBufferInfo
        };
        
        descriptorWrites[2] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = globalDescriptorSets_[i],
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = static_cast<uint32_t>(NUM_CASCADES),
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = shadowImageInfos.data()
        };
        
        vkUpdateDescriptorSets(device_->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), 
                              descriptorWrites.data(), 0, nullptr);
    }
    
    return true;
}

bool ForwardPlusRenderer::createPipeline() {
    // Load shaders
    auto vertShaderCode = readShaderFile("shaders/cube.vert.spv");
    auto fragShaderCode = readShaderFile("shaders/cube.frag.spv");
    
    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        std::cerr << "Failed to load cube shaders" << std::endl;
        return false;
    }
    
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // Vertex input
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    
    VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swapChain_->getExtent().width),
        .height = static_cast<float>(swapChain_->getExtent().height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    
    VkRect2D scissor{
        .offset = {0, 0},
        .extent = swapChain_->getExtent()
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
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };
    
    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };
    
    // Enable depth testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
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
    
    // Push constants for model matrix
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(glm::mat4)
    };
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &globalDescriptorLayout_,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    
    if (vkCreatePipelineLayout(device_->getDevice(), &pipelineLayoutInfo, nullptr, 
                              &forwardPipelineLayout_) != VK_SUCCESS) {
        return false;
    }
    
    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .layout = forwardPipelineLayout_,
        .renderPass = renderPass_,
        .subpass = 0
    };
    
    if (vkCreateGraphicsPipelines(device_->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, 
                                 nullptr, &forwardPipeline_) != VK_SUCCESS) {
        return false;
    }
    
    vkDestroyShaderModule(device_->getDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(device_->getDevice(), vertShaderModule, nullptr);
    
    return true;
}

bool ForwardPlusRenderer::createShadowPipeline() {
    // Load shadow shaders
    auto vertShaderCode = readShaderFile("shaders/shadow_cascade.vert.spv");
    auto fragShaderCode = readShaderFile("shaders/shadow_cascade.frag.spv");
    
    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        std::cerr << "Failed to load shadow shaders" << std::endl;
        return false;
    }
    
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main"
    };
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // Vertex input - same as main pipeline
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    
    VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(SHADOW_MAP_SIZE),
        .height = static_cast<float>(SHADOW_MAP_SIZE),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    
    VkRect2D scissor{
        .offset = {0, 0},
        .extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}
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
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_TRUE,  // Enable depth bias for shadow maps
        .depthBiasConstantFactor = 1.25f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 1.75f,
        .lineWidth = 1.0f
    };
    
    VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f
    };
    
    // No color attachments for shadow pass
    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 0,
        .pAttachments = nullptr
    };
    
    // Push constants for model and light space matrices
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(glm::mat4) * 2  // model + lightSpaceMatrix
    };
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    
    if (vkCreatePipelineLayout(device_->getDevice(), &pipelineLayoutInfo, nullptr, 
                              &shadowPipelineLayout_) != VK_SUCCESS) {
        vkDestroyShaderModule(device_->getDevice(), fragShaderModule, nullptr);
        vkDestroyShaderModule(device_->getDevice(), vertShaderModule, nullptr);
        return false;
    }
    
    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .layout = shadowPipelineLayout_,
        .renderPass = shadowRenderPass_,
        .subpass = 0
    };
    
    bool success = vkCreateGraphicsPipelines(device_->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, 
                                            nullptr, &shadowPipeline_) == VK_SUCCESS;
    
    vkDestroyShaderModule(device_->getDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(device_->getDevice(), vertShaderModule, nullptr);
    
    return success;
}

std::vector<char> ForwardPlusRenderer::readShaderFile(const std::string& filename) {
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

VkShaderModule ForwardPlusRenderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    
    return shaderModule;
}

bool ForwardPlusRenderer::createCubeGeometry() {
    // Cube vertices with proper normals for each face
    std::vector<Vertex> vertices = {
        // Front face (Z+) - Red
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {1.0f, 0.2f, 0.2f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {1.0f, 0.2f, 0.2f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {1.0f, 0.2f, 0.2f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {1.0f, 0.2f, 0.2f}},
        
        // Back face (Z-) - Green
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {0.2f, 1.0f, 0.2f}},
        {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {0.2f, 1.0f, 0.2f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {0.2f, 1.0f, 0.2f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {0.2f, 1.0f, 0.2f}},
        
        // Top face (Y+) - Blue
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {0.2f, 0.2f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {0.2f, 0.2f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {0.2f, 0.2f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {0.2f, 0.2f, 1.0f}},
        
        // Bottom face (Y-) - Yellow
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 0.2f}},
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 0.2f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 0.2f}},
        {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 0.2f}},
        
        // Right face (X+) - Magenta
        {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {1.0f, 0.2f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {1.0f, 0.2f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {1.0f, 0.2f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {1.0f, 0.2f, 1.0f}},
        
        // Left face (X-) - Cyan
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.2f, 1.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.2f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.2f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.2f, 1.0f, 1.0f}},
        
        // Floor plane (large, below the cube) - Gray with grid pattern
        {{-5.0f, -2.0f,  5.0f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {0.3f, 0.3f, 0.35f}},
        {{ 5.0f, -2.0f,  5.0f}, { 0.0f,  1.0f,  0.0f}, {5.0f, 0.0f}, {0.3f, 0.3f, 0.35f}},
        {{ 5.0f, -2.0f, -5.0f}, { 0.0f,  1.0f,  0.0f}, {5.0f, 5.0f}, {0.3f, 0.3f, 0.35f}},
        {{-5.0f, -2.0f, -5.0f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 5.0f}, {0.3f, 0.3f, 0.35f}}
    };
    
    // Corrected winding order for proper culling (counter-clockwise from outside)
    std::vector<uint32_t> indices = {
        0,  1,  2,  2,  3,  0,   // Front
        4,  5,  6,  6,  7,  4,   // Back
        8,  9,  10, 10, 11, 8,   // Top
        12, 13, 14, 14, 15, 12,  // Bottom
        16, 17, 18, 18, 19, 16,  // Right
        20, 21, 22, 22, 23, 20,  // Left
        24, 25, 26, 26, 27, 24   // Floor
    };
    
    indexCount_ = static_cast<uint32_t>(indices.size());
    
    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    vertexBuffer_ = std::make_unique<VulkanBuffer>();
    if (!vertexBuffer_->create(*device_, vertexBufferSize,
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        return false;
    }
    vertexBuffer_->copyFrom(vertices.data(), vertexBufferSize);
    
    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    indexBuffer_ = std::make_unique<VulkanBuffer>();
    if (!indexBuffer_->create(*device_, indexBufferSize,
                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        return false;
    }
    indexBuffer_->copyFrom(indices.data(), indexBufferSize);
    
    return true;
}

bool ForwardPlusRenderer::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device_->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device_->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(device_->getDevice(), &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool ForwardPlusRenderer::createCommandBuffers() {
    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = device_->getQueueFamilies().graphicsFamily.value()
    };

    if (vkCreateCommandPool(device_->getDevice(), &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        return false;
    }

    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool_,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
    };

    return vkAllocateCommandBuffers(device_->getDevice(), &allocInfo, commandBuffers_.data()) == VK_SUCCESS;
}

uint32_t ForwardPlusRenderer::calculateNumTiles(uint32_t dimension) const noexcept {
    return (dimension + config_.tileSize - 1) / config_.tileSize;
}

void ForwardPlusRenderer::beginFrame() {
    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
    
    vkWaitForFences(device_->getDevice(), 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
    vkResetFences(device_->getDevice(), 1, &inFlightFences_[currentFrame_]);
    
    imageIndex_ = swapChain_->acquireNextImage(imageAvailableSemaphores_[currentFrame_]);
}

void ForwardPlusRenderer::endFrame() {
    swapChain_->present(imageIndex_, renderFinishedSemaphores_[currentFrame_]);
}

void ForwardPlusRenderer::renderScene(const CameraUBO& camera, std::span<const PointLight> lights) {
    // Update camera UBO
    cameraBuffers_[currentFrame_]->copyFrom(&camera, sizeof(CameraUBO));
    
    // Update shadow UBO
    updateShadowUBO();
    
    VkCommandBuffer cmd = commandBuffers_[currentFrame_];
    
    vkResetCommandBuffer(cmd, 0);
    
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        return;
    }
    
    // Render shadow cascades first
    renderShadowCascades(cmd);
    
    // Clear values for color and depth
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.02f, 0.02f, 0.04f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    
    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass_,
        .framebuffer = framebuffers_[imageIndex_],
        .renderArea = {
            .offset = {0, 0},
            .extent = swapChain_->getExtent()
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data()
    };
    
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, forwardPipeline_);
    
    // Bind vertex buffer
    VkBuffer vertexBuffers[] = {vertexBuffer_->getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    
    // Bind index buffer
    vkCmdBindIndexBuffer(cmd, indexBuffer_->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    // Bind descriptor set
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, forwardPipelineLayout_,
                           0, 1, &globalDescriptorSets_[currentFrame_], 0, nullptr);
    
    float time = static_cast<float>(glfwGetTime());
    
    // Draw rotating cube hovering above the floor
    glm::mat4 cubeModel = glm::mat4(1.0f);
    cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 0.5f, 0.0f)); // Hover above floor
    cubeModel = glm::rotate(cubeModel, time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    cubeModel = glm::rotate(cubeModel, time * glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Push model matrix for cube
    vkCmdPushConstants(cmd, forwardPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 
                      0, sizeof(glm::mat4), &cubeModel);
    
    // Draw cube (36 indices for 6 faces)
    vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);
    
    // Draw stationary floor
    glm::mat4 floorModel = glm::mat4(1.0f); // Identity - no transformation
    
    // Push model matrix for floor
    vkCmdPushConstants(cmd, forwardPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 
                      0, sizeof(glm::mat4), &floorModel);
    
    // Draw floor (6 indices starting at index 36)
    vkCmdDrawIndexed(cmd, 6, 1, 36, 0, 0);
    
    vkCmdEndRenderPass(cmd);
    
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        return;
    }
    
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO
    };
    
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkQueueSubmit(device_->getGraphicsQueue(), 1, &submitInfo, inFlightFences_[currentFrame_]);
}

void ForwardPlusRenderer::updateLights(std::span<const PointLight> lights) {
    // Update light buffer
}

void ForwardPlusRenderer::uploadMesh(std::span<const Vertex> vertices, std::span<const uint32_t> indices) {
    indexCount_ = static_cast<uint32_t>(indices.size());
}

void ForwardPlusRenderer::onWindowResize(uint32_t width, uint32_t height) {
    config_.width = width;
    config_.height = height;
    
    if (device_) {
        device_->waitIdle();
    }
    
    numTilesX_ = calculateNumTiles(width);
    numTilesY_ = calculateNumTiles(height);
}

VkFormat ForwardPlusRenderer::findDepthFormat() const {
    std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };
    
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device_->getPhysicalDevice(), format, &props);
        
        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }
    
    return VK_FORMAT_D32_SFLOAT; // Fallback
}

bool ForwardPlusRenderer::createShadowResources() {
    VkFormat depthFormat = findDepthFormat();
    
    // Create shadow map images for each cascade
    for (size_t i = 0; i < NUM_CASCADES; ++i) {
        shadowImages_[i] = std::make_unique<VulkanImage>();
        if (!shadowImages_[i]->create(*device_, 
                                      SHADOW_MAP_SIZE,
                                      SHADOW_MAP_SIZE,
                                      depthFormat,
                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)) {
            return false;
        }
        
        // Create image view for this cascade
        VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = shadowImages_[i]->getImage(),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = depthFormat,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        
        if (vkCreateImageView(device_->getDevice(), &viewInfo, nullptr, &shadowImageViews_[i]) != VK_SUCCESS) {
            return false;
        }
    }
    
    return true;
}

bool ForwardPlusRenderer::createShadowRenderPass() {
    VkAttachmentDescription depthAttachment{
        .format = findDepthFormat(),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };
    
    VkAttachmentReference depthRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    
    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 0,
        .pDepthStencilAttachment = &depthRef
    };
    
    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };
    
    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &depthAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    
    return vkCreateRenderPass(device_->getDevice(), &renderPassInfo, nullptr, &shadowRenderPass_) == VK_SUCCESS;
}

bool ForwardPlusRenderer::createShadowFramebuffers() {
    for (size_t i = 0; i < NUM_CASCADES; ++i) {
        VkFramebufferCreateInfo framebufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = shadowRenderPass_,
            .attachmentCount = 1,
            .pAttachments = &shadowImageViews_[i],
            .width = SHADOW_MAP_SIZE,
            .height = SHADOW_MAP_SIZE,
            .layers = 1
        };
        
        VkFramebuffer framebuffer;
        if (vkCreateFramebuffer(device_->getDevice(), &framebufferInfo, nullptr, 
                               &framebuffer) != VK_SUCCESS) {
            return false;
        }
        shadowFramebuffers_[i].push_back(framebuffer);
    }
    
    return true;
}

bool ForwardPlusRenderer::createShadowSampler() {
    VkSamplerCreateInfo samplerInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .compareEnable = VK_TRUE,
        .compareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .minLod = 0.0f,
        .maxLod = 1.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE
    };
    
    return vkCreateSampler(device_->getDevice(), &samplerInfo, nullptr, &shadowSampler_) == VK_SUCCESS;
}

void ForwardPlusRenderer::calculateCascadeSplits() {
    float nearPlane = 0.1f;
    float farPlane = 200.0f;  // Extended far plane for larger coverage
    float range = farPlane - nearPlane;
    float ratio = farPlane / nearPlane;
    
    // Calculate split depths based on practical split scheme
    for (uint32_t i = 0; i < NUM_CASCADES; ++i) {
        float p = (i + 1) / static_cast<float>(NUM_CASCADES);
        float log = nearPlane * std::pow(ratio, p);
        float uniform = nearPlane + range * p;
        float d = 0.90f * log + 0.10f * uniform; // More logarithmic distribution
        cascadeSplits_[i] = (d - nearPlane) / range;
    }
    
    // Convert to actual distances
    cascadeSplits_[0] *= farPlane; // First split
    cascadeSplits_[1] *= farPlane; // Second split
    cascadeSplits_[2] *= farPlane; // Third split
    cascadeSplits_[3] = farPlane;  // Far plane
}

glm::mat4 ForwardPlusRenderer::calculateLightSpaceMatrix(float nearPlane, float farPlane) {
    // Get camera frustum corners in world space
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 
                                      static_cast<float>(config_.width) / config_.height,
                                      nearPlane, farPlane);
    glm::mat4 invCam = glm::inverse(proj);
    
    // Create light view matrix
    glm::vec3 lightPos = -lightDirection_ * 50.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Calculate frustum corners
    std::vector<glm::vec4> frustumCorners;
    for (uint32_t x = 0; x < 2; ++x) {
        for (uint32_t y = 0; y < 2; ++y) {
            for (uint32_t z = 0; z < 2; ++z) {
                glm::vec4 pt = invCam * glm::vec4(
                    2.0f * x - 1.0f,
                    2.0f * y - 1.0f,
                    2.0f * z - 1.0f,
                    1.0f
                );
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    
    // Transform to light space and find bounds
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
    
    for (const auto& corner : frustumCorners) {
        glm::vec4 lightSpacePos = lightView * corner;
        minBounds = glm::min(minBounds, glm::vec3(lightSpacePos));
        maxBounds = glm::max(maxBounds, glm::vec3(lightSpacePos));
    }
    
    // Create orthographic projection for shadow map
    glm::mat4 lightProjection = glm::ortho(
        minBounds.x, maxBounds.x,
        minBounds.y, maxBounds.y,
        minBounds.z, maxBounds.z
    );
    
    return lightProjection * lightView;
}

void ForwardPlusRenderer::updateShadowUBO() {
    ShadowUBO shadowUBO{};
    
    // Calculate light space matrices for each cascade
    float lastSplit = 0.1f;
    for (size_t i = 0; i < NUM_CASCADES; ++i) {
        shadowUBO.lightSpaceMatrices[i] = calculateLightSpaceMatrix(lastSplit, cascadeSplits_[i]);
        lastSplit = cascadeSplits_[i];
    }
    
    shadowUBO.cascadeSplits = glm::vec4(cascadeSplits_[0], cascadeSplits_[1], cascadeSplits_[2], NUM_CASCADES);
    shadowUBO.lightDirection = lightDirection_;
    shadowUBO.padding = 0.0f;
    
    shadowBuffers_[currentFrame_]->copyFrom(&shadowUBO, sizeof(ShadowUBO));
}

void ForwardPlusRenderer::renderShadowCascades(VkCommandBuffer cmd) {
    // Render scene geometry from light's perspective for each cascade
    for (size_t i = 0; i < NUM_CASCADES; ++i) {
        VkClearValue clearValue{};
        clearValue.depthStencil = {1.0f, 0};
        
        VkRenderPassBeginInfo renderPassInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = shadowRenderPass_,
            .framebuffer = shadowFramebuffers_[i][0],
            .renderArea = {
                .offset = {0, 0},
                .extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}
            },
            .clearValueCount = 1,
            .pClearValues = &clearValue
        };
        
        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        // Bind shadow pipeline
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline_);
        
        // Bind vertex and index buffers
        VkBuffer vertexBuffers[] = {vertexBuffer_->getBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, indexBuffer_->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        
        // Get light space matrix for this cascade
        float lastSplit = (i == 0) ? 0.1f : cascadeSplits_[i - 1];
        glm::mat4 lightSpaceMatrix = calculateLightSpaceMatrix(lastSplit, cascadeSplits_[i]);
        
        float time = static_cast<float>(glfwGetTime());
        
        // Render rotating cube
        glm::mat4 cubeModel = glm::mat4(1.0f);
        cubeModel = glm::translate(cubeModel, glm::vec3(0.0f, 0.5f, 0.0f));
        cubeModel = glm::rotate(cubeModel, time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        cubeModel = glm::rotate(cubeModel, time * glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Push model and light space matrices
        struct {
            glm::mat4 model;
            glm::mat4 lightSpace;
        } pushConstants;
        
        pushConstants.model = cubeModel;
        pushConstants.lightSpace = lightSpaceMatrix;
        
        vkCmdPushConstants(cmd, shadowPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 
                          0, sizeof(pushConstants), &pushConstants);
        
        // Draw cube (36 indices for 6 faces)
        vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);
        
        // Render floor
        glm::mat4 floorModel = glm::mat4(1.0f);
        pushConstants.model = floorModel;
        
        vkCmdPushConstants(cmd, shadowPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 
                          0, sizeof(pushConstants), &pushConstants);
        
        // Draw floor (6 indices starting at index 36)
        vkCmdDrawIndexed(cmd, 6, 1, 36, 0, 0);
        
        vkCmdEndRenderPass(cmd);
    }
}

bool ForwardPlusRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    
    depthImage_ = std::make_unique<VulkanImage>();
    if (!depthImage_->create(*device_, 
                             swapChain_->getExtent().width,
                             swapChain_->getExtent().height,
                             depthFormat,
                             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
        return false;
    }
    
    // Create image view
    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depthImage_->getImage(),
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depthFormat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    
    if (vkCreateImageView(device_->getDevice(), &viewInfo, nullptr, &depthImageView_) != VK_SUCCESS) {
        return false;
    }
    
    return true;
}

} // namespace vibe::vk
