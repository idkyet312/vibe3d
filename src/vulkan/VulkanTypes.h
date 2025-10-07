#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <optional>
#include <memory>
#include <string_view>
#include <span>
#include <concepts>

namespace vibe::vk {

// Queue family indices
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;

    [[nodiscard]] constexpr bool isComplete() const noexcept {
        return graphicsFamily.has_value() && 
               presentFamily.has_value() && 
               computeFamily.has_value();
    }
};

// Swap chain support details
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Vertex data structure with C++20 features
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 color;

    static constexpr VkVertexInputBindingDescription getBindingDescription() noexcept {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    static constexpr std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() noexcept {
        return {{
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, position)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, normal)
            },
            {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texCoord)
            },
            {
                .location = 3,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, color)
            }
        }};
    }
};

// Uniform buffer objects
struct CameraUBO {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec4 position; // w unused
};

struct ShadowUBO {
    alignas(16) glm::mat4 lightSpaceMatrix; // Single light space matrix
    alignas(16) glm::vec4 cascadeSplits; // Kept for compatibility (unused)
    alignas(16) glm::vec3 lightDirection;
    alignas(4) float depthBiasConstant; // Simple depth bias
    alignas(16) glm::vec4 cascadeBiasValues; // Kept for compatibility (unused)
};

// BACKUP: Cascaded shadow UBO (disabled)
// struct ShadowUBO {
//     alignas(16) glm::mat4 lightSpaceMatrices[4]; // 4 cascades
//     alignas(16) glm::vec4 cascadeSplits;
//     alignas(16) glm::vec3 lightDirection;
//     alignas(4) float receiverBiasMultiplier;
//     alignas(16) glm::vec4 cascadeBiasValues;
// };

struct ModelUBO {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 normalMatrix;
};

struct MaterialUBO {
    alignas(16) glm::vec3 albedo; // RGB from controller
    alignas(4) float roughness;
    alignas(16) glm::vec3 emissive; // RGB emissive color
    alignas(4) float metallic;
    alignas(4) float ambientStrength;
    alignas(4) float lightIntensity;
    alignas(4) float emissiveStrength;
    alignas(4) float padding;
};

// Forward+ specific structures
struct PointLight {
    alignas(16) glm::vec3 position;
    alignas(4) float radius;
    alignas(16) glm::vec3 color;
    alignas(4) float intensity;
};

struct LightGrid {
    static constexpr uint32_t TILE_SIZE = 16;
    uint32_t numTilesX;
    uint32_t numTilesY;
    uint32_t maxLightsPerTile;
};

// Material data
struct MaterialData {
    alignas(16) glm::vec3 albedo;
    alignas(4) float metallic;
    alignas(16) glm::vec3 emission;
    alignas(4) float roughness;
    alignas(4) float ao;
    alignas(4) uint32_t textureFlags; // Bitfield for texture availability
    alignas(8) glm::vec2 padding;
};

// Push constants
struct PushConstants {
    alignas(16) glm::mat4 model;
    alignas(4) int debugMode;  // 0 = normal, 1 = show shadows, 2 = show cascades
    alignas(4) int objectID;   // 0 = cube, 1 = floor
    alignas(4) float padding[2];  // 2 floats for padding
};

// Forward+ push constants for compute
struct LightCullingPushConstants {
    alignas(4) uint32_t numLights;
    alignas(4) uint32_t numTilesX;
    alignas(4) uint32_t numTilesY;
    alignas(4) uint32_t screenWidth;
    alignas(4) uint32_t screenHeight;
};

// Concepts for type safety
template<typename T>
concept VulkanHandle = std::is_pointer_v<T> || std::is_integral_v<T>;

template<typename T>
concept BufferDataType = std::is_trivially_copyable_v<T>;

} // namespace vibe::vk
