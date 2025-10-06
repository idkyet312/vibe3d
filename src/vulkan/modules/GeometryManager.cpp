#include "GeometryManager.h"
#include <glm/glm.hpp>

namespace vibe::vk {

GeometryManager::GeometryManager(VulkanDevice& device)
    : device_(device) {
}

bool GeometryManager::createCubeGeometry(VulkanBuffer& vertexBuffer, VulkanBuffer& indexBuffer, 
                                         uint32_t& indexCount) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    createCubeMesh(vertices, indices);
    
    indexCount = static_cast<uint32_t>(indices.size());
    
    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    if (!vertexBuffer.create(device_, vertexBufferSize,
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        return false;
    }
    vertexBuffer.copyFrom(vertices.data(), vertexBufferSize);
    
    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    if (!indexBuffer.create(device_, indexBufferSize,
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        return false;
    }
    indexBuffer.copyFrom(indices.data(), indexBufferSize);
    
    return true;
}

void GeometryManager::createCubeMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    vertices = {
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
        
        // Floor plane (very wide, below the cube) - Gray
        {{-50.0f, -2.0f,  50.0f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {0.3f, 0.3f, 0.35f}},
        {{ 50.0f, -2.0f,  50.0f}, { 0.0f,  1.0f,  0.0f}, {50.0f, 0.0f}, {0.3f, 0.3f, 0.35f}},
        {{ 50.0f, -2.0f, -50.0f}, { 0.0f,  1.0f,  0.0f}, {50.0f, 50.0f}, {0.3f, 0.3f, 0.35f}},
        {{-50.0f, -2.0f, -50.0f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 50.0f}, {0.3f, 0.3f, 0.35f}}
    };
    
    // Counter-clockwise winding order for proper culling
    indices = {
        0,  1,  2,  2,  3,  0,   // Front
        4,  5,  6,  6,  7,  4,   // Back
        8,  9,  10, 10, 11, 8,   // Top
        12, 13, 14, 14, 15, 12,  // Bottom
        16, 17, 18, 18, 19, 16,  // Right
        20, 21, 22, 22, 23, 20,  // Left
        24, 25, 26, 26, 27, 24   // Floor
    };
}

} // namespace vibe::vk
