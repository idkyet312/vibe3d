#pragma once

#include <vulkan/vulkan.h>
#include <string>

struct GLFWwindow;

namespace vibe::vk {

/**
 * @brief Manages ImGui integration for Vulkan renderer
 * Provides material control UI embedded in the main window
 */
class ImGuiManager {
public:
    ImGuiManager() = default;
    ~ImGuiManager();
    
    // Initialization
    bool initialize(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physicalDevice,
                   VkDevice device, uint32_t graphicsQueueFamily, VkQueue graphicsQueue,
                   VkRenderPass renderPass);
    void cleanup();
    
    // Frame rendering
    void beginFrame();
    void endFrame(VkCommandBuffer commandBuffer);
    
    // Material control panel
    struct MaterialControls {
        float albedoR = 0.8f;
        float albedoG = 0.3f;
        float albedoB = 0.2f;
        float roughness = 0.5f;
        float metallic = 0.0f;
        float ambientStrength = 0.001f;
        float lightIntensity = 0.1f;  // Updated to current level (was 0.4)
        float lightYaw = 225.0f;
        float lightPitch = 45.0f;
        float emissiveR = 0.549f;  // Red emissive color
        float emissiveG = 0.0f;
        float emissiveB = 0.0f;
        float emissiveStrength = 0.1f;  // Default emissive strength
        int currentPreset = 0;  // 0 = Custom
        bool valuesChanged = false;
    };
    
    // Render the material control panel and return current values
    MaterialControls& getMaterialControls() { return controls_; }
    void renderMaterialPanel();
    void renderFPSCounter();
    
    // Bloom controls
    struct BloomControls {
        bool enabled = true;
        float strength = 0.5f;    // Realistic bloom intensity
        float threshold = 0.15f;  // Lower threshold to catch more emissive areas
        bool valuesChanged = false;
    };
    
    BloomControls& getBloomControls() { return bloomControls_; }
    void renderBloomPanel();
    
    // Config save/load (persists to disk)
    void saveConfig(int slot); // slot 0-2
    void loadConfig(int slot);
    void saveConfigsToDisk();  // Save all configs to file
    void loadConfigsFromDisk(); // Load all configs from file
    
    // Handle window resize
    void onWindowResize();

private:
    VkDescriptorPool imguiPool_ = VK_NULL_HANDLE;
    bool initialized_ = false;
    MaterialControls controls_;
    BloomControls bloomControls_;
    
    // Saved config slots
    struct SavedConfig {
        MaterialControls material;
        BloomControls bloom;
        bool hasData = false;
    };
    SavedConfig savedConfigs_[3];  // 3 save slots
    
    // Preset definitions
    struct Preset {
        const char* name;
        float r, g, b;
        float roughness;
        float metallic;
    };
    
    static constexpr Preset PRESETS[] = {
        {"Custom", 0.8f, 0.3f, 0.2f, 0.5f, 0.0f},
        {"Rough Plastic", 0.8f, 0.3f, 0.2f, 0.9f, 0.0f},
        {"Smooth Plastic", 0.2f, 0.6f, 0.9f, 0.2f, 0.0f},
        {"Rough Metal", 0.7f, 0.7f, 0.7f, 0.8f, 1.0f},
        {"Polished Metal", 0.8f, 0.8f, 0.8f, 0.2f, 1.0f},
        {"Gold", 1.0f, 0.766f, 0.336f, 0.3f, 1.0f},
        {"Copper", 0.955f, 0.637f, 0.538f, 0.4f, 1.0f},
        {"Chrome", 0.9f, 0.9f, 0.9f, 0.1f, 1.0f},
        {"Aluminum", 0.913f, 0.921f, 0.925f, 0.5f, 1.0f},
        {"Rubber", 0.1f, 0.1f, 0.1f, 0.95f, 0.0f},
        {"Wood", 0.545f, 0.353f, 0.169f, 0.8f, 0.0f}
    };
    
    void applyPreset(int presetIndex);
};

} // namespace vibe::vk
