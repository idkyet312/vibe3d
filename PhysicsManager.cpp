#include "PhysicsManager.h"
#include <iostream>
#include <algorithm>

PhysicsManager::PhysicsManager() 
    : gravity(-9.81f), jumpForce(5.0f)
    , bulletRadius(0.05f), lastShotTime(0.0f), shootCooldown(0.15f)
#if PHYSX_ENABLED
    , gFoundation(nullptr), gPhysics(nullptr)
    , gDispatcher(nullptr), gScene(nullptr)
    , gMaterial(nullptr), gPvd(nullptr)
#endif
{
}

PhysicsManager::~PhysicsManager() {
    cleanup();
}

bool PhysicsManager::initialize() {
#if PHYSX_ENABLED
    initPhysX();
    std::cout << "PhysX enabled - using hardware physics" << std::endl;
#else
    std::cout << "PhysX not available - using simple physics fallback" << std::endl;
#endif
    return true;
}

void PhysicsManager::cleanup() {
#if PHYSX_ENABLED
    cleanupPhysX();
#endif
}

void PhysicsManager::updatePhysics(float deltaTime) {
#if PHYSX_ENABLED
    // PhysX physics update would go here when PhysX is available
#else
    // Use simple physics fallback
    updateSimplePhysics(deltaTime);
#endif
}

void PhysicsManager::shootBullet(const glm::vec3& position, const glm::vec3& direction) {
    Bullet bullet;
    bullet.position = position;
    bullet.direction = glm::normalize(direction);
    bullet.active = true;
    bullet.timeAlive = 0.0f;

#if PHYSX_ENABLED
    // PhysX implementation would go here when PhysX is available
#else
    // Simple physics fallback
    bullet.simplePhysics.position = bullet.position;
    bullet.simplePhysics.velocity = bullet.direction * bullet.speed;
    bullet.simplePhysics.isActive = true;
#endif

    bullets.push_back(bullet);
}

void PhysicsManager::spawnCube(const glm::vec3& position, const glm::vec3& velocity) {
    if (cubes.size() >= MAX_CUBES) {
        return;
    }

    Cube cube;
    cube.position = position;
    cube.isActive = true;

#if PHYSX_ENABLED
    // PhysX implementation would go here when PhysX is available
#else
    // Simple physics fallback
    cube.simplePhysics.position = position;
    cube.simplePhysics.velocity = velocity;
    cube.simplePhysics.isActive = true;
#endif

    cubes.push_back(cube);
}

void PhysicsManager::updateMainObject(glm::vec3& objectPos, float deltaTime) {
    const float floorY = -0.5f;
    const float bounceRestitution = 0.3f;
    
    // Update main object (simple falling)
    static float mainObjectVelocity = 0.0f;
    mainObjectVelocity += gravity * deltaTime;
    objectPos.y += mainObjectVelocity * deltaTime;
    
    if (objectPos.y < floorY + CUBE_SIZE/2) {
        objectPos.y = floorY + CUBE_SIZE/2;
        mainObjectVelocity *= -bounceRestitution;
    }
}

void PhysicsManager::updateCameraPhysics(glm::vec3& cameraPos, float& verticalVelocity, bool& isGrounded, float deltaTime) {
    // Apply gravity
    verticalVelocity += gravity * deltaTime;
    cameraPos.y += verticalVelocity * deltaTime;

    // Ground collision
    if (cameraPos.y <= 1.0f) {
        cameraPos.y = 1.0f;
        verticalVelocity = 0.0f;
        isGrounded = true;
    }
}

bool PhysicsManager::canShoot(float currentTime) const {
    return (currentTime - lastShotTime) >= shootCooldown;
}

void PhysicsManager::updateSimplePhysics(float deltaTime) {
    const float floorY = -0.5f;
    const float bounceRestitution = 0.3f;
    
    // Update bullets
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->timeAlive += deltaTime;
        
        if (it->timeAlive >= it->lifetime) {
            it = bullets.erase(it);
            continue;
        }

        // Apply gravity
        it->simplePhysics.velocity.y += gravity * deltaTime;
        
        // Update position
        it->simplePhysics.position += it->simplePhysics.velocity * deltaTime;
        it->position = it->simplePhysics.position;
        
        // Floor collision
        if (it->position.y < floorY) {
            it->position.y = floorY;
            it->simplePhysics.position.y = floorY;
            it->simplePhysics.velocity.y *= -bounceRestitution;
        }
        
        ++it;
    }
    
    // Update cubes
    for (auto& cube : cubes) {
        if (cube.isActive) {
            // Apply gravity
            cube.simplePhysics.velocity.y += gravity * deltaTime;
            
            // Update position
            cube.simplePhysics.position += cube.simplePhysics.velocity * deltaTime;
            cube.position = cube.simplePhysics.position;
            
            // Floor collision
            if (cube.position.y < floorY + CUBE_SIZE/2) {
                cube.position.y = floorY + CUBE_SIZE/2;
                cube.simplePhysics.position.y = cube.position.y;
                cube.simplePhysics.velocity.y *= -bounceRestitution;
                
                // Add some friction
                cube.simplePhysics.velocity.x *= 0.95f;
                cube.simplePhysics.velocity.z *= 0.95f;
            }
        }
    }
}

#if PHYSX_ENABLED
void PhysicsManager::initPhysX() {
    // PhysX initialization would go here when PhysX is available
}

void PhysicsManager::cleanupPhysX() {
    // PhysX cleanup would go here when PhysX is available
}
#endif