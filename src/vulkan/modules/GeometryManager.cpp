#include "GeometryManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <algorithm>
#include <unordered_map>

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
        
        // Floor plane (medium size, below the cube) - Gray
        {{-25.0f, -2.0f,  25.0f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {0.3f, 0.3f, 0.35f}},
        {{ 25.0f, -2.0f,  25.0f}, { 0.0f,  1.0f,  0.0f}, {25.0f, 0.0f}, {0.3f, 0.3f, 0.35f}},
        {{ 25.0f, -2.0f, -25.0f}, { 0.0f,  1.0f,  0.0f}, {25.0f, 25.0f}, {0.3f, 0.3f, 0.35f}},
        {{-25.0f, -2.0f, -25.0f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 25.0f}, {0.3f, 0.3f, 0.35f}}
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

bool GeometryManager::loadModelGeometry(const std::string& filepath, 
                                       VulkanBuffer& vertexBuffer, 
                                       VulkanBuffer& indexBuffer, 
                                       uint32_t& indexCount) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Determine file type by extension
    auto hasExtension = [](const std::string& str, const std::string& ext) {
        if (str.length() < ext.length()) return false;
        return str.compare(str.length() - ext.length(), ext.length(), ext) == 0;
    };
    
    bool loadSuccess = false;
    if (hasExtension(filepath, ".obj") || hasExtension(filepath, ".OBJ")) {
        loadSuccess = loadOBJModel(filepath, vertices, indices);
    } else if (hasExtension(filepath, ".glb") || hasExtension(filepath, ".GLB") || 
               hasExtension(filepath, ".gltf") || hasExtension(filepath, ".GLTF")) {
        loadSuccess = loadGLBModel(filepath, vertices, indices);
    } else {
        std::cerr << "Unsupported file format: " << filepath << std::endl;
        return false;
    }
    
    if (!loadSuccess) {
        std::cerr << "Failed to load model: " << filepath << std::endl;
        return false;
    }
    
    // Add floor plane to the model
    size_t modelVertexCount = vertices.size();
    size_t modelIndexCount = indices.size();
    
    // Floor vertices
    vertices.push_back({{-25.0f, -2.0f,  25.0f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {0.3f, 0.3f, 0.35f}});
    vertices.push_back({{ 25.0f, -2.0f,  25.0f}, { 0.0f,  1.0f,  0.0f}, {25.0f, 0.0f}, {0.3f, 0.3f, 0.35f}});
    vertices.push_back({{ 25.0f, -2.0f, -25.0f}, { 0.0f,  1.0f,  0.0f}, {25.0f, 25.0f}, {0.3f, 0.3f, 0.35f}});
    vertices.push_back({{-25.0f, -2.0f, -25.0f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 25.0f}, {0.3f, 0.3f, 0.35f}});
    
    // Floor indices (offset by model vertex count)
    uint32_t floorBase = static_cast<uint32_t>(modelVertexCount);
    indices.push_back(floorBase + 0);
    indices.push_back(floorBase + 1);
    indices.push_back(floorBase + 2);
    indices.push_back(floorBase + 2);
    indices.push_back(floorBase + 3);
    indices.push_back(floorBase + 0);
    
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
    
    std::cout << "Loaded model: " << filepath << std::endl;
    std::cout << "  Vertices: " << modelVertexCount << " (+" << vertices.size() - modelVertexCount << " floor)" << std::endl;
    std::cout << "  Indices: " << modelIndexCount << " (+" << indices.size() - modelIndexCount << " floor)" << std::endl;
    
    return true;
}

bool GeometryManager::loadGLBModel(const std::string& filepath, 
                                  std::vector<Vertex>& vertices, 
                                  std::vector<uint32_t>& indices) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath);
    
    if (!warn.empty()) {
        std::cout << "Warning: " << warn << std::endl;
    }
    
    if (!err.empty()) {
        std::cerr << "Error: " << err << std::endl;
    }
    
    if (!ret) {
        return false;
    }
    
    // Process all meshes in the model
    for (const auto& mesh : model.meshes) {
        for (const auto& primitive : mesh.primitives) {
            // Get positions
            const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
            const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
            const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
            const float* positions = reinterpret_cast<const float*>(
                &posBuffer.data[posView.byteOffset + posAccessor.byteOffset]
            );
            
            // Get normals (optional)
            const float* normals = nullptr;
            if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
                const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
                const tinygltf::Buffer& normBuffer = model.buffers[normView.buffer];
                normals = reinterpret_cast<const float*>(
                    &normBuffer.data[normView.byteOffset + normAccessor.byteOffset]
                );
            }
            
            // Get UVs (optional)
            const float* texCoords = nullptr;
            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
                const tinygltf::Buffer& uvBuffer = model.buffers[uvView.buffer];
                texCoords = reinterpret_cast<const float*>(
                    &uvBuffer.data[uvView.byteOffset + uvAccessor.byteOffset]
                );
            }
            
            // Get vertex colors (optional)
            const float* colors = nullptr;
            if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
                const tinygltf::Accessor& colorAccessor = model.accessors[primitive.attributes.at("COLOR_0")];
                const tinygltf::BufferView& colorView = model.bufferViews[colorAccessor.bufferView];
                const tinygltf::Buffer& colorBuffer = model.buffers[colorView.buffer];
                colors = reinterpret_cast<const float*>(
                    &colorBuffer.data[colorView.byteOffset + colorAccessor.byteOffset]
                );
            }
            
            size_t vertexCount = posAccessor.count;
            size_t baseVertex = vertices.size();
            
            // Build vertices
            for (size_t i = 0; i < vertexCount; ++i) {
                Vertex vertex{};
                
                // Position
                vertex.position = glm::vec3(
                    positions[i * 3 + 0],
                    positions[i * 3 + 1],
                    positions[i * 3 + 2]
                );
                
                // Normal
                if (normals) {
                    vertex.normal = glm::vec3(
                        normals[i * 3 + 0],
                        normals[i * 3 + 1],
                        normals[i * 3 + 2]
                    );
                } else {
                    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
                
                // Texture coordinates
                if (texCoords) {
                    vertex.texCoord = glm::vec2(
                        texCoords[i * 2 + 0],
                        texCoords[i * 2 + 1]
                    );
                } else {
                    vertex.texCoord = glm::vec2(0.0f);
                }
                
                // Color
                if (colors) {
                    vertex.color = glm::vec3(
                        colors[i * 3 + 0],
                        colors[i * 3 + 1],
                        colors[i * 3 + 2]
                    );
                } else {
                    vertex.color = glm::vec3(0.8f, 0.8f, 0.8f); // Default gray
                }
                
                vertices.push_back(vertex);
            }
            
            // Get indices
            if (primitive.indices >= 0) {
                const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer& indexBuffer = model.buffers[indexView.buffer];
                const void* indexData = &indexBuffer.data[indexView.byteOffset + indexAccessor.byteOffset];
                
                for (size_t i = 0; i < indexAccessor.count; ++i) {
                    uint32_t index = 0;
                    
                    switch (indexAccessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                            const uint16_t* buf = static_cast<const uint16_t*>(indexData);
                            index = buf[i];
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                            const uint32_t* buf = static_cast<const uint32_t*>(indexData);
                            index = buf[i];
                            break;
                        }
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                            const uint8_t* buf = static_cast<const uint8_t*>(indexData);
                            index = buf[i];
                            break;
                        }
                    }
                    
                    indices.push_back(static_cast<uint32_t>(baseVertex + index));
                }
            }
        }
    }
    
    return !vertices.empty();
}

bool GeometryManager::loadOBJModel(const std::string& filepath,
                                  std::vector<Vertex>& vertices,
                                  std::vector<uint32_t>& indices) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    
    // Get directory from filepath for material loading
    std::string directory;
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        directory = filepath.substr(0, lastSlash + 1);
    }
    
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), directory.c_str())) {
        if (!warn.empty()) {
            std::cout << "OBJ Warning: " << warn << std::endl;
        }
        if (!err.empty()) {
            std::cerr << "OBJ Error: " << err << std::endl;
        }
        return false;
    }
    
    if (!warn.empty()) {
        std::cout << "OBJ Warning: " << warn << std::endl;
    }
    
    // Use a hash map to deduplicate vertices
    std::unordered_map<std::string, uint32_t> uniqueVertices;
    
    // Process all shapes in the OBJ file
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};
            
            // Position (required)
            if (index.vertex_index >= 0) {
                // Load position directly without coordinate conversion
                // Assume OBJ is already in correct coordinate system or let user export correctly
                vertex.position = glm::vec3(
                    attrib.vertices[3 * size_t(index.vertex_index) + 0],
                    attrib.vertices[3 * size_t(index.vertex_index) + 1],
                    attrib.vertices[3 * size_t(index.vertex_index) + 2]
                );
            }
            
            // Normal (optional)
            if (index.normal_index >= 0 && !attrib.normals.empty()) {
                // Load normals directly
                vertex.normal = glm::vec3(
                    attrib.normals[3 * size_t(index.normal_index) + 0],
                    attrib.normals[3 * size_t(index.normal_index) + 1],
                    attrib.normals[3 * size_t(index.normal_index) + 2]
                );
            } else {
                // Default normal pointing up
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            
            // Texture coordinates (optional)
            if (index.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                vertex.texCoord = glm::vec2(
                    attrib.texcoords[2 * size_t(index.texcoord_index) + 0],
                    1.0f - attrib.texcoords[2 * size_t(index.texcoord_index) + 1]  // Flip V coordinate
                );
            } else {
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
            }
            
            // Default vertex color (OBJ typically doesn't have per-vertex colors)
            vertex.color = glm::vec3(0.8f, 0.8f, 0.8f);
            
            // Create a unique key for this vertex
            std::string key = std::to_string(vertex.position.x) + "_" +
                            std::to_string(vertex.position.y) + "_" +
                            std::to_string(vertex.position.z) + "_" +
                            std::to_string(vertex.normal.x) + "_" +
                            std::to_string(vertex.normal.y) + "_" +
                            std::to_string(vertex.normal.z) + "_" +
                            std::to_string(vertex.texCoord.x) + "_" +
                            std::to_string(vertex.texCoord.y);
            
            // Check if we've seen this vertex before
            auto it = uniqueVertices.find(key);
            if (it != uniqueVertices.end()) {
                // Reuse existing vertex
                indices.push_back(it->second);
            } else {
                // Add new vertex
                uint32_t newIndex = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
                uniqueVertices[key] = newIndex;
                indices.push_back(newIndex);
            }
        }
    }
    
    std::cout << "OBJ Loaded: " << shapes.size() << " shapes, " 
              << vertices.size() << " unique vertices, " 
              << indices.size() << " indices" << std::endl;
    
    return !vertices.empty();
}

} // namespace vibe::vk
