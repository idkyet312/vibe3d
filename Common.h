#pragma once

// Common includes used throughout the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <memory>

// OpenGL includes
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// PhysX conditional include
#if PHYSX_ENABLED
#include <PxPhysicsAPI.h>
using namespace physx;
#endif

// Common constants
constexpr float PI = 3.14159265359f;
constexpr float TWO_PI = 2.0f * PI;

// Common utility functions
namespace Utils {
    inline float radians(float degrees) {
        return degrees * PI / 180.0f;
    }
    
    inline float degrees(float radians) {
        return radians * 180.0f / PI;
    }
}