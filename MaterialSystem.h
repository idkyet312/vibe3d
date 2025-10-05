#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

// Material properties structure
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    std::string name;
};

// Raytracing material structure
struct RTMaterial {
    glm::vec3 albedo;
    glm::vec3 specular;
    float shininess;
    float metallic;
    float roughness;
    float ior;
    int type; // 0 = diffuse, 1 = metal, 2 = glass
};

// Raytracing sphere structure
struct RTSphere {
    glm::vec3 center;
    float radius;
    RTMaterial material;
};

class MaterialSystem {
public:
    MaterialSystem();
    
    // Material management
    const Material& getCurrentMaterial() const;
    int getCurrentMaterialIndex() const { return currentMaterialIndex; }
    void cycleMaterial();
    void setMaterial(int index);
    size_t getMaterialCount() const { return materials.size(); }
    const std::vector<Material>& getAllMaterials() const { return materials; }
    
    // Raytracing material conversion
    RTMaterial convertToRTMaterial(const Material& mat, int type = 0) const;
    
    // Material type detection
    bool isMaterialMetallic(const Material& material) const;
    bool isMaterialGlass(const Material& material) const;
    
private:
    std::vector<Material> materials;
    int currentMaterialIndex;
    
    void initializeMaterials();
};