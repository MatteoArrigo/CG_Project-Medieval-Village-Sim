#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include "modules/Starter.hpp"
#include "modules/Scene.hpp"

// Structure to hold Bullet Physics objects
struct PhysicsObject {
    btRigidBody* body;
    btCollisionShape* shape;
    btDefaultMotionState* motionState;
    glm::vec3 initialPosition;

    PhysicsObject() : body(nullptr), shape(nullptr), motionState(nullptr) {}
    ~PhysicsObject();
};

// Physical parameters for the player
struct PlayerConfig {
    float capsuleRadius = 0.3f;
    float capsuleHeight = 1.6f;
    float mass = 80.0f;
    glm::vec3 startPosition = glm::vec3(20, 20, 20);

    float moveSpeed = 2.3f;
    float runSpeed = 3.5f;
    float jumpForce = 500.0f;
    float airControl = 0.3f; // On-air control factor
    float groundDamping = 0.5f; // Damping factor when grounded
};

// Background terrain configuration.
// NOTE: This is a large flat plane which serves as "fallback" ground in case no terrain is loaded.
struct BackgroundTerrainConfig {
    float size = 500.0f; // Side length of the terrain square
    glm::vec3 position = glm::vec3(0, 0, 0);
};

class PhysicsManager {
private:
    // Bullet Physics core objects
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfig;
    btCollisionDispatcher* dispatcher;
    btSequentialImpulseConstraintSolver* solver;
    btDiscreteDynamicsWorld* dynamicsWorld;

    // Game objects
    std::unique_ptr<PhysicsObject> player;
    std::unique_ptr<PhysicsObject> terrain;
    std::vector<std::unique_ptr<PhysicsObject>> staticObjects;

    // Configuration
    PlayerConfig playerConfig;
    BackgroundTerrainConfig terrainConfig;

    // Internal state
    bool isGrounded;
    float groundCheckDistance;

    // Helper methods
    void initializePhysicsWorld();
    void createTerrain();
    void createPlayer();
    bool checkGrounded();

public:
    PhysicsManager();
    ~PhysicsManager();

    // Initialization
    bool initialize(const PlayerConfig& playerCfg = PlayerConfig(),
                   const BackgroundTerrainConfig& terrainCfg = BackgroundTerrainConfig());
    void cleanup();

    // Update
    void update(float deltaTime);

    // Player control
    void movePlayer(const glm::vec3& moveDirection, bool isRunning = false);
    void jumpPlayer();
    glm::vec3 getPlayerPosition() const;
    glm::vec3 getPlayerVelocity() const;
    bool isPlayerGrounded() const { return isGrounded; }

    // Object management
    PhysicsObject* addStaticBox(const glm::vec3& position, const glm::vec3& size);
    PhysicsObject* addStaticSphere(const glm::vec3& position, float radius);
    void addStaticMeshes(Model **modelRefs, Instance **instanceRefs, int instanceCount);

    // Utility
    void setGravity(const glm::vec3& gravity);
    void setPlayerPosition(const glm::vec3& position);

    // Debug
    int getNumRigidBodies() const;
};