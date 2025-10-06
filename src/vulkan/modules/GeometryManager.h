#pragma once

#include "../VulkanTypes.h"
#include "../VulkanDevice.h"
#include "../VulkanBuffer.h"
#include <memory>
#include <vector>
#include <span>

namespace vibe::vk {

/**
 * @brief Manages geometry data (vertices, indices, meshes)
 * 
 * Handles:
 * - Mesh creation (cube, floor, etc.)
 * - Vertex and index buffer population
 * - Geometry upload to GPU
 */
class GeometryManager {
public:
    explicit GeometryManager(VulkanDevice& device);
    ~GeometryManager() = default;

    // Mesh creation
    bool createCubeGeometry(VulkanBuffer& vertexBuffer, VulkanBuffer& indexBuffer, uint32_t& indexCount);
    void createCubeMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

private:
    VulkanDevice& device_;
};

} // namespace vibe::vk
