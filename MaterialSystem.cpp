#include "MaterialSystem.h"
#include <iostream>
#include <cmath>

MaterialSystem::MaterialSystem() : currentMaterialIndex(0) {
    initializeMaterials();
}

void MaterialSystem::initializeMaterials() {
    materials = {
        { glm::vec3(0.1745f, 0.01175f, 0.01175f), glm::vec3(0.61424f, 0.04136f, 0.04136f), glm::vec3(0.727811f, 0.626959f, 0.626959f), 76.8f, "Ruby" },
        { glm::vec3(0.329412f, 0.223529f, 0.027451f), glm::vec3(0.780392f, 0.568627f, 0.113725f), glm::vec3(0.992157f, 0.941176f, 0.807843f), 27.8974f, "Gold" },
        { glm::vec3(0.2125f, 0.1275f, 0.054f), glm::vec3(0.714f, 0.4284f, 0.18144f), glm::vec3(0.393548f, 0.271906f, 0.166721f), 25.6f, "Bronze" },
        { glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.774597f, 0.774597f, 0.774597f), 76.8f, "Silver" },
        { glm::vec3(0.19125f, 0.0735f, 0.0225f), glm::vec3(0.7038f, 0.27048f, 0.0828f), glm::vec3(0.256777f, 0.137622f, 0.086014f), 12.8f, "Copper" },
        { glm::vec3(0.0f, 0.05f, 0.0f), glm::vec3(0.4f, 0.5f, 0.4f), glm::vec3(0.04f, 0.7f, 0.04f), 10.0f, "Emerald" },
        { glm::vec3(0.02f, 0.02f, 0.02f), glm::vec3(0.01f, 0.01f, 0.01f), glm::vec3(0.4f, 0.4f, 0.4f), 10.0f, "Black Plastic" },
        { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.7f, 0.6f, 0.6f), 32.0f, "Red Plastic" },
        { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.1f, 0.35f, 0.1f), glm::vec3(0.45f, 0.55f, 0.45f), 32.0f, "Green Plastic" },
        { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.6f, 0.6f, 0.7f), 32.0f, "Blue Plastic" }
    };
}

const Material& MaterialSystem::getCurrentMaterial() const {
    return materials[currentMaterialIndex];
}

void MaterialSystem::cycleMaterial() {
    currentMaterialIndex = (currentMaterialIndex + 1) % materials.size();
    std::cout << "Material changed to: " << materials[currentMaterialIndex].name << std::endl;
}

void MaterialSystem::setMaterial(int index) {
    if (index >= 0 && index < static_cast<int>(materials.size())) {
        currentMaterialIndex = index;
        std::cout << "Material set to: " << materials[currentMaterialIndex].name << std::endl;
    }
}

RTMaterial MaterialSystem::convertToRTMaterial(const Material& mat, int type) const {
    RTMaterial rtMat;
    rtMat.albedo = mat.diffuse;
    rtMat.specular = mat.specular;
    rtMat.shininess = mat.shininess;
    rtMat.metallic = (type == 1) ? 0.8f : 0.0f;
    rtMat.roughness = 1.0f / sqrt(mat.shininess);
    rtMat.ior = 1.5f;
    rtMat.type = type;
    return rtMat;
}

bool MaterialSystem::isMaterialMetallic(const Material& material) const {
    return (material.name.find("Gold") != std::string::npos ||
            material.name.find("Silver") != std::string::npos ||
            material.name.find("Copper") != std::string::npos ||
            material.name.find("Bronze") != std::string::npos);
}

bool MaterialSystem::isMaterialGlass(const Material& material) const {
    return material.name.find("Emerald") != std::string::npos;
}