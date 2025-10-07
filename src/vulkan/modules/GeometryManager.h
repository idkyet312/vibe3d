#pragma once

#include "../VulkanTypes.h"
#include "../VulkanDevice.h"
#include "../VulkanBuffer.h"
#include <memory>
#include <vector>
#include <span>
#include <string>

namespace vibe::vk {

/**
 * @brief Manages geometry data (vertices, indices, meshes)
 * 
 * Handles:
 * - Mesh creation (cube, floor, etc.)
 * - Model loading from GLB/glTF files
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
    
    // Model loading
    bool loadModelGeometry(const std::string& filepath, 
                          VulkanBuffer& vertexBuffer, 
                          VulkanBuffer& indexBuffer, 
                          uint32_t& indexCount);

private:
    VulkanDevice& device_;
    
    // Helper functions for model loading
    bool loadGLBModel(const std::string& filepath, 
                     std::vector<Vertex>& vertices, 
                     std::vector<uint32_t>& indices);
    
    bool loadOBJModel(const std::string& filepath,
                     std::vector<Vertex>& vertices,
                     std::vector<uint32_t>& indices);
};

} // namespace vibe::vk
