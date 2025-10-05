#pragma once

#include <glm/glm.hpp>
#include <vector>

#if PHYSX_ENABLED
#include <PxPhysicsAPI.h>
using namespace physx;
#endif

// Simple physics for fallback
struct SimplePhysicsObject {
    glm::vec3 position;
    glm::vec3 velocity;
    float mass = 1.0f;
    float size = 0.5f;
    bool isActive = false;
};

// Bullet system
struct Bullet {
    glm::vec3 position;
    glm::vec3 direction;
    float speed = 30.0f;
    float lifetime = 5.0f;
    float timeAlive = 0.0f;
    bool active = false;
#if PHYSX_ENABLED
    PxRigidDynamic* actor = nullptr;
#endif
    SimplePhysicsObject simplePhysics;
};

// Cube structure
struct Cube {
    glm::vec3 position;
#if PHYSX_ENABLED
    PxRigidDynamic* actor = nullptr;
#endif
    bool isActive = false;
    SimplePhysicsObject simplePhysics;
};

class PhysicsManager {
public:
    PhysicsManager();
    ~PhysicsManager();

    // Initialization
    bool initialize();
    void cleanup();

    // Physics update
    void updatePhysics(float deltaTime);
    
    // Bullet management
    void shootBullet(const glm::vec3& position, const glm::vec3& direction);
    const std::vector<Bullet>& getBullets() const { return bullets; }
    void clearBullets() { bullets.clear(); }
    
    // Cube management
    void spawnCube(const glm::vec3& position, const glm::vec3& velocity);
    const std::vector<Cube>& getCubes() const { return cubes; }
    void clearCubes() { cubes.clear(); }
    
    // Main object physics
    void updateMainObject(glm::vec3& objectPos, float deltaTime);
    
    // Camera physics
    void updateCameraPhysics(glm::vec3& cameraPos, float& verticalVelocity, bool& isGrounded, float deltaTime);
    
    // Settings
    void setGravity(float newGravity) { gravity = newGravity; }
    void setJumpForce(float newJumpForce) { jumpForce = newJumpForce; }
    
    // Shooting controls
    bool canShoot(float currentTime) const;
    void updateLastShotTime(float time) { lastShotTime = time; }

private:
    // Physics variables
    float gravity;
    float jumpForce;
    
    // Bullet system
    std::vector<Bullet> bullets;
    float bulletRadius;
    float lastShotTime;
    float shootCooldown;
    
    // Cube system
    std::vector<Cube> cubes;
    static const int MAX_CUBES = 50;
    static constexpr float CUBE_MASS = 1.0f;
    static constexpr float CUBE_SIZE = 0.5f;
    static constexpr float CUBE_INITIAL_SPEED = 5.0f;
    
#if PHYSX_ENABLED
    // PhysX global variables
    PxDefaultAllocator gAllocator;
    PxDefaultErrorCallback gErrorCallback;
    PxFoundation* gFoundation;
    PxPhysics* gPhysics;
    PxDefaultCpuDispatcher* gDispatcher;
    PxScene* gScene;
    PxMaterial* gMaterial;
    PxPvd* gPvd;
    
    void initPhysX();
    void cleanupPhysX();
#endif
    
    void updateSimplePhysics(float deltaTime);
};