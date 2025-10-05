#include "ForwardPlusRenderer.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <array>
#include <cmath>

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
    
    if (!createRenderPass()) {
        std::cerr << "Failed to create render pass" << std::endl;
        return false;
    }
    
    if (!createFramebuffers()) {
        std::cerr << "Failed to create framebuffers" << std::endl;
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

    VkAttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
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

    return vkCreateRenderPass(device_->getDevice(), &renderPassInfo, nullptr, &renderPass_) == VK_SUCCESS;
}

bool ForwardPlusRenderer::createFramebuffers() {
    const auto& imageViews = swapChain_->getImageViews();
    framebuffers_.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        VkImageView attachments[] = { imageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass_,
            .attachmentCount = 1,
            .pAttachments = attachments,
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
    VkCommandBuffer cmd = commandBuffers_[currentFrame_];
    
    vkResetCommandBuffer(cmd, 0);
    
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        return;
    }
    
    // Animated clear color based on time
    float time = static_cast<float>(glfwGetTime());
    float r = (std::sin(time * 0.5f) + 1.0f) * 0.5f;
    float g = (std::sin(time * 0.7f + 2.0f) + 1.0f) * 0.5f;
    float b = (std::sin(time * 0.3f + 4.0f) + 1.0f) * 0.5f;
    
    VkClearValue clearColor = {{{r * 0.3f, g * 0.4f, b * 0.6f, 1.0f}}};
    
    VkRenderPassBeginInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass_,
        .framebuffer = framebuffers_[imageIndex_],
        .renderArea = {
            .offset = {0, 0},
            .extent = swapChain_->getExtent()
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };
    
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // TODO: Draw geometry here (triangles, meshes, etc.)
    // For now we have a beautiful animated gradient background
    
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

uint32_t ForwardPlusRenderer::calculateNumTiles(uint32_t dimension) const noexcept {
    return (dimension + config_.tileSize - 1) / config_.tileSize;
}

} // namespace vibe::vk
