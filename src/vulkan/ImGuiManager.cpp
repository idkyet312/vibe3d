#include "ImGuiManager.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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
    
    // Load saved configurations from disk
    loadConfigsFromDisk();
    
    // Auto-load preset 1 on startup if it exists
    if (savedConfigs_[0].hasData) {
        loadConfig(0);  // Load slot 1 (index 0)
        std::cout << "Auto-loaded preset 1 on startup" << std::endl;
    }
    
    initialized_ = true;
    return true;
}

void ImGuiManager::cleanup() {
    if (!initialized_) return;

    // Save configurations before cleanup
    saveConfigsToDisk();

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
    changed |= ImGui::DragFloat("Red", &controls_.albedoR, 0.01f, 0.0f, 1.0f, "%.3f");
    changed |= ImGui::DragFloat("Green", &controls_.albedoG, 0.01f, 0.0f, 1.0f, "%.3f");
    changed |= ImGui::DragFloat("Blue", &controls_.albedoB, 0.01f, 0.0f, 1.0f, "%.3f");
    
    // Color preview
    ImVec4 colorPreview(controls_.albedoR, controls_.albedoG, controls_.albedoB, 1.0f);
    ImGui::ColorButton("Preview", colorPreview, 0, ImVec2(100, 30));
    
    ImGui::Separator();
    ImGui::Text("Material Properties");
    changed |= ImGui::DragFloat("Roughness", &controls_.roughness, 0.01f, 0.0f, 1.0f, "%.3f");
    changed |= ImGui::DragFloat("Metallic", &controls_.metallic, 0.01f, 0.0f, 1.0f, "%.3f");
    
    ImGui::Separator();
    ImGui::Text("Lighting");
    changed |= ImGui::DragFloat("Skylight", &controls_.ambientStrength, 0.001f, 0.0f, 1.0f, "%.4f");
    changed |= ImGui::DragFloat("Sun Intensity", &controls_.lightIntensity, 0.1f, 0.0f, 20.0f, "%.2f");
    
    ImGui::Separator();
    ImGui::Text("Emissive (Glow)");
    changed |= ImGui::ColorEdit3("Emissive Color", &controls_.emissiveR, ImGuiColorEditFlags_Float);
    changed |= ImGui::DragFloat("Emissive Strength", &controls_.emissiveStrength, 1.0f, 0.0f, 500.0f, "%.1f");
    
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
    changed |= ImGui::DragFloat("Yaw (Horizontal)", &controls_.lightYaw, 1.0f, 0.0f, 360.0f, "%.1f°");
    changed |= ImGui::DragFloat("Pitch (Vertical)", &controls_.lightPitch, 1.0f, 0.0f, 90.0f, "%.1f°");
    
    // If any value changed manually, set to Custom
    if (changed && controls_.currentPreset != 0) {
        controls_.currentPreset = 0;  // Custom
    }
    
    if (changed) {
        controls_.valuesChanged = true;
    }
    
    ImGui::Separator();
    ImGui::Text("Config Management");
    
    // Save/Load buttons in a grid
    for (int i = 0; i < 3; i++) {
        ImGui::PushID(i);
        
        // Save button
        if (ImGui::Button(("Save " + std::to_string(i + 1)).c_str(), ImVec2(60, 0))) {
            saveConfig(i);
        }
        ImGui::SameLine();
        
        // Load button (disabled if no data)
        if (!savedConfigs_[i].hasData) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button(("Load " + std::to_string(i + 1)).c_str(), ImVec2(60, 0))) {
            loadConfig(i);
        }
        if (!savedConfigs_[i].hasData) {
            ImGui::EndDisabled();
        }
        
        // Show indicator if slot has data
        ImGui::SameLine();
        ImGui::Text(savedConfigs_[i].hasData ? "[*]" : "[ ]");
        
        ImGui::PopID();
    }
    
    ImGui::Separator();
    if (ImGui::Button("Reset to Default", ImVec2(-1, 0))) {
        controls_.albedoR = 0.8f;
        controls_.albedoG = 0.3f;
        controls_.albedoB = 0.2f;
        controls_.roughness = 0.5f;
        controls_.metallic = 0.0f;
        controls_.ambientStrength = 0.001f;
        controls_.lightIntensity = 8.0f;   // Match renderer default
        controls_.lightYaw = 225.0f;
        controls_.lightPitch = 45.0f;
        controls_.emissiveR = 0.549f;      // Red emissive
        controls_.emissiveG = 0.0f;
        controls_.emissiveB = 0.0f;
        controls_.emissiveStrength = 0.1f; // Default emissive glow
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

void ImGuiManager::renderShadowPanel() {
    ImGui::SetNextWindowPos(ImVec2(370, 570), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 120), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Shadow Settings", nullptr);
    
    bool changed = false;
    
    ImGui::Text("Shadow Bias Control");
    ImGui::Spacing();
    
    // Depth bias constant slider - wide range for adjustment
    ImGui::Text("Depth Bias Constant");
    if (ImGui::DragFloat("##DepthBias", &shadowControls_.depthBiasConstant, 0.0001f, 0.0f, 0.1f, "%.4f")) {
        changed = true;
    }
    
    if (changed) {
        shadowControls_.valuesChanged = true;
    }
    
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
    
    // Bloom strength slider with direct input
    ImGui::Text("Bloom Strength");
    if (ImGui::DragFloat("##BloomStrength", &bloomControls_.strength, 0.001f, 0.0f, 5.0f, "%.3f")) {
        changed = true;
    }
    
    // Bloom threshold slider with direct input
    ImGui::Text("Threshold (Brightness)");
    if (ImGui::DragFloat("##BloomThreshold", &bloomControls_.threshold, 0.01f, 0.0f, 10.0f, "%.2f")) {
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

void ImGuiManager::saveConfig(int slot) {
    if (slot < 0 || slot >= 3) return;
    
    savedConfigs_[slot].material = controls_;
    savedConfigs_[slot].bloom = bloomControls_;
    savedConfigs_[slot].hasData = true;
    
    // Auto-save to disk after each save
    saveConfigsToDisk();
    
    std::cout << "Saved configuration to slot " << (slot + 1) << std::endl;
}

void ImGuiManager::loadConfig(int slot) {
    if (slot < 0 || slot >= 3 || !savedConfigs_[slot].hasData) return;
    
    controls_ = savedConfigs_[slot].material;
    bloomControls_ = savedConfigs_[slot].bloom;
    controls_.valuesChanged = true;
    bloomControls_.valuesChanged = true;
    
    std::cout << "Loaded configuration from slot " << (slot + 1) << std::endl;
}

void ImGuiManager::saveConfigsToDisk() {
    try {
        json j = json::object();
        j["configs"] = json::array();
        
        // Save all 3 config slots
        for (int i = 0; i < 3; i++) {
            json slot = json::object();
            slot["hasData"] = savedConfigs_[i].hasData;
            
            if (savedConfigs_[i].hasData) {
                // Material settings
                auto& mat = savedConfigs_[i].material;
                slot["material"] = {
                    {"albedoR", mat.albedoR},
                    {"albedoG", mat.albedoG},
                    {"albedoB", mat.albedoB},
                    {"roughness", mat.roughness},
                    {"metallic", mat.metallic},
                    {"ambientStrength", mat.ambientStrength},
                    {"lightIntensity", mat.lightIntensity},
                    {"lightYaw", mat.lightYaw},
                    {"lightPitch", mat.lightPitch},
                    {"emissiveR", mat.emissiveR},
                    {"emissiveG", mat.emissiveG},
                    {"emissiveB", mat.emissiveB},
                    {"emissiveStrength", mat.emissiveStrength}
                };
                
                // Bloom settings
                auto& bloom = savedConfigs_[i].bloom;
                slot["bloom"] = {
                    {"enabled", bloom.enabled},
                    {"strength", bloom.strength},
                    {"threshold", bloom.threshold}
                };
            }
            
            j["configs"].push_back(slot);
        }
        
        // Write to file
        std::ofstream file("vibe3d_presets.json");
        if (file.is_open()) {
            file << j.dump(2);  // Pretty print with 2-space indent
            file.close();
            std::cout << "Saved all presets to vibe3d_presets.json" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving configs to disk: " << e.what() << std::endl;
    }
}

void ImGuiManager::loadConfigsFromDisk() {
    try {
        std::ifstream file("vibe3d_presets.json");
        if (!file.is_open()) {
            std::cout << "No saved presets found, using defaults" << std::endl;
            return;
        }
        
        json j;
        file >> j;
        file.close();
        
        // Load all 3 config slots
        if (j.contains("configs") && j["configs"].is_array()) {
            auto& configs = j["configs"];
            for (int i = 0; i < 3 && i < configs.size(); i++) {
                auto& slot = configs[i];
                
                if (!slot.contains("hasData") || !slot["hasData"].get<bool>()) {
                    savedConfigs_[i].hasData = false;
                    continue;
                }
                
                // Load material settings
                if (slot.contains("material")) {
                    auto& mat = savedConfigs_[i].material;
                    auto& jmat = slot["material"];
                    mat.albedoR = jmat["albedoR"].get<float>();
                    mat.albedoG = jmat["albedoG"].get<float>();
                    mat.albedoB = jmat["albedoB"].get<float>();
                    mat.roughness = jmat["roughness"].get<float>();
                    mat.metallic = jmat["metallic"].get<float>();
                    mat.ambientStrength = jmat["ambientStrength"].get<float>();
                    mat.lightIntensity = jmat["lightIntensity"].get<float>();
                    mat.lightYaw = jmat["lightYaw"].get<float>();
                    mat.lightPitch = jmat["lightPitch"].get<float>();
                    mat.emissiveR = jmat["emissiveR"].get<float>();
                    mat.emissiveG = jmat["emissiveG"].get<float>();
                    mat.emissiveB = jmat["emissiveB"].get<float>();
                    mat.emissiveStrength = jmat["emissiveStrength"].get<float>();
                }
                
                // Load bloom settings
                if (slot.contains("bloom")) {
                    auto& bloom = savedConfigs_[i].bloom;
                    auto& jbloom = slot["bloom"];
                    bloom.enabled = jbloom["enabled"].get<bool>();
                    bloom.strength = jbloom["strength"].get<float>();
                    bloom.threshold = jbloom["threshold"].get<float>();
                }
                
                savedConfigs_[i].hasData = true;
            }
            
            std::cout << "Loaded presets from vibe3d_presets.json" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading configs from disk: " << e.what() << std::endl;
    }
}

void ImGuiManager::onWindowResize() {
    // ImGui handles this automatically
}

} // namespace vibe::vk
