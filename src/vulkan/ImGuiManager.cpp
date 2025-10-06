#include "ImGuiManager.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>

namespace vibe::vk {

ImGuiManager::~ImGuiManager() {
    cleanup();
}

bool ImGuiManager::initialize(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physicalDevice,
                               VkDevice device, uint32_t graphicsQueueFamily, VkQueue graphicsQueue,
                               VkRenderPass renderPass) {
    
    // Create descriptor pool for ImGui
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = std::size(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiPool_) != VK_SUCCESS) {
        std::cerr << "Failed to create ImGui descriptor pool" << std::endl;
        return false;
    }

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Don't capture mouse when not hovering ImGui windows
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Setup style
    ImGui::StyleColorsDark();

    // Initialize ImGui for GLFW and Vulkan
    // Pass true to install callbacks - ImGui will handle input when camera is frozen
    ImGui_ImplGlfw_InitForVulkan(window, true);
    
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = physicalDevice;
    initInfo.Device = device;
    initInfo.QueueFamily = graphicsQueueFamily;
    initInfo.Queue = graphicsQueue;
    initInfo.DescriptorPool = imguiPool_;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = 2;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.RenderPass = renderPass;

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        std::cerr << "Failed to initialize ImGui Vulkan" << std::endl;
        return false;
    }

    std::cout << "ImGui initialized successfully" << std::endl;
    initialized_ = true;
    return true;
}

void ImGuiManager::cleanup() {
    if (!initialized_) return;

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    initialized_ = false;
}

void ImGuiManager::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::endFrame(VkCommandBuffer commandBuffer) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void ImGuiManager::renderMaterialPanel() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 600), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Material Controller", nullptr, ImGuiWindowFlags_NoCollapse);
    
    // Preset selector
    const char* presetNames[] = {
        "Custom", "Rough Plastic", "Smooth Plastic", "Rough Metal",
        "Polished Metal", "Gold", "Copper", "Chrome",
        "Aluminum", "Rubber", "Wood"
    };
    
    int previousPreset = controls_.currentPreset;
    if (ImGui::Combo("Preset", &controls_.currentPreset, presetNames, IM_ARRAYSIZE(presetNames))) {
        if (controls_.currentPreset != 0) {  // Not "Custom"
            applyPreset(controls_.currentPreset);
            controls_.valuesChanged = true;
        }
    }
    
    ImGui::Separator();
    ImGui::Text("Albedo Color");
    
    bool changed = false;
    changed |= ImGui::SliderFloat("Red", &controls_.albedoR, 0.0f, 1.0f);
    changed |= ImGui::SliderFloat("Green", &controls_.albedoG, 0.0f, 1.0f);
    changed |= ImGui::SliderFloat("Blue", &controls_.albedoB, 0.0f, 1.0f);
    
    // Color preview
    ImVec4 colorPreview(controls_.albedoR, controls_.albedoG, controls_.albedoB, 1.0f);
    ImGui::ColorButton("Preview", colorPreview, 0, ImVec2(100, 30));
    
    ImGui::Separator();
    ImGui::Text("Material Properties");
    changed |= ImGui::SliderFloat("Roughness", &controls_.roughness, 0.0f, 1.0f);
    changed |= ImGui::SliderFloat("Metallic", &controls_.metallic, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Lighting");
    changed |= ImGui::SliderFloat("Skylight", &controls_.ambientStrength, 0.0f, 1.0f);
    changed |= ImGui::SliderFloat("Sun Intensity", &controls_.lightIntensity, 0.0f, 20.0f);
    
    ImGui::Separator();
    ImGui::Text("Emissive (Glow)");
    changed |= ImGui::ColorEdit3("Emissive Color", &controls_.emissiveR, ImGuiColorEditFlags_Float);
    changed |= ImGui::SliderFloat("Emissive Strength", &controls_.emissiveStrength, 0.0f, 500.0f);
    
    // Emissive color preview
    if (controls_.emissiveStrength > 0.0f) {
        ImVec4 emissivePreview(
            controls_.emissiveR * controls_.emissiveStrength,
            controls_.emissiveG * controls_.emissiveStrength,
            controls_.emissiveB * controls_.emissiveStrength,
            1.0f
        );
        ImGui::ColorButton("Emissive Preview", emissivePreview, 0, ImVec2(100, 30));
    }
    
    ImGui::Separator();
    ImGui::Text("Sun Direction");
    changed |= ImGui::SliderFloat("Yaw (Horizontal)", &controls_.lightYaw, 0.0f, 360.0f);
    changed |= ImGui::SliderFloat("Pitch (Vertical)", &controls_.lightPitch, 0.0f, 90.0f);
    
    // If any value changed manually, set to Custom
    if (changed && controls_.currentPreset != 0) {
        controls_.currentPreset = 0;  // Custom
    }
    
    if (changed) {
        controls_.valuesChanged = true;
    }
    
    ImGui::Separator();
    if (ImGui::Button("Reset to Default", ImVec2(-1, 0))) {
        controls_.albedoR = 0.8f;
        controls_.albedoG = 0.3f;
        controls_.albedoB = 0.2f;
        controls_.roughness = 0.5f;
        controls_.metallic = 0.0f;
        controls_.ambientStrength = 0.001f;
        controls_.lightIntensity = 8.0f;
        controls_.lightYaw = 225.0f;
        controls_.lightPitch = 45.0f;
        controls_.emissiveR = 0.0f;
        controls_.emissiveG = 0.0f;
        controls_.emissiveB = 0.0f;
        controls_.emissiveStrength = 0.0f;
        controls_.currentPreset = 0;
        controls_.valuesChanged = true;
    }
    
    ImGui::End();
}

void ImGuiManager::renderFPSCounter() {
    // Get the IO object for FPS calculation
    ImGuiIO& io = ImGui::GetIO();
    
    // Create a small window in the top right corner
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 120, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(110, 50), ImGuiCond_Always);
    
    // Window with no title bar, no resize, no move, semi-transparent background
    ImGui::Begin("##FPS", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | 
                 ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove | 
                 ImGuiWindowFlags_NoScrollbar | 
                 ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoFocusOnAppearing |
                 ImGuiWindowFlags_NoNav);
    
    // Display FPS with larger font
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("%.2f ms", 1000.0f / io.Framerate);
    
    ImGui::End();
}

void ImGuiManager::renderBloomPanel() {
    ImGui::SetNextWindowPos(ImVec2(10, 570), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 150), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Bloom Settings", nullptr);
    
    bool changed = false;
    
    // Bloom enable/disable
    if (ImGui::Checkbox("Enable Bloom", &bloomControls_.enabled)) {
        changed = true;
    }
    
    ImGui::Spacing();
    
    // Bloom strength slider
    ImGui::Text("Bloom Strength");
    if (ImGui::SliderFloat("##BloomStrength", &bloomControls_.strength, 0.0f, 0.2f, "%.3f")) {
        changed = true;
    }
    
    // Bloom threshold slider
    ImGui::Text("Threshold (Brightness)");
    if (ImGui::SliderFloat("##BloomThreshold", &bloomControls_.threshold, 0.0f, 3.0f, "%.2f")) {
        changed = true;
    }
    
    if (changed) {
        bloomControls_.valuesChanged = true;
    }
    
    ImGui::End();
}

void ImGuiManager::applyPreset(int presetIndex) {
    if (presetIndex < 0 || presetIndex >= std::size(PRESETS)) return;
    
    const Preset& preset = PRESETS[presetIndex];
    controls_.albedoR = preset.r;
    controls_.albedoG = preset.g;
    controls_.albedoB = preset.b;
    controls_.roughness = preset.roughness;
    controls_.metallic = preset.metallic;
}

void ImGuiManager::onWindowResize() {
    // ImGui handles this automatically
}

} // namespace vibe::vk
