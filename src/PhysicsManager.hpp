#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>

// Struttura per rappresentare un oggetto fisico
struct PhysicsObject {
    btRigidBody* body;
    btCollisionShape* shape;
    btDefaultMotionState* motionState;
    glm::vec3 initialPosition;

    PhysicsObject() : body(nullptr), shape(nullptr), motionState(nullptr) {}
    ~PhysicsObject();
};

// Parametri per la creazione del player
struct PlayerConfig {
    float capsuleRadius = 0.3f;
    float capsuleHeight = 1.6f;
    float mass = 80.0f;
    glm::vec3 startPosition = glm::vec3(0, 10, 0);

    // Parametri di movimento
    float moveSpeed = 8.0f;
    float runSpeed = 12.0f;
    float jumpForce = 500.0f;
    float airControl = 0.3f; // Controllo del movimento in aria
    float groundDamping = 0.8f; // Smorzamento quando fermo
};

// Parametri per il terreno
struct TerrainConfig {
    float size = 500.0f; // Mezzo lato del piano
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
    TerrainConfig terrainConfig;

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
                   const TerrainConfig& terrainCfg = TerrainConfig());
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
    PhysicsObject* addStaticMesh(const std::vector<glm::vec3>& vertices,
                                const std::vector<uint32_t>& indices,
                                const glm::vec3& position = glm::vec3(0));

    // Utility
    void setGravity(const glm::vec3& gravity);
    void setPlayerPosition(const glm::vec3& position);

    // Debug
    void debugDrawWorld(); // Per debug rendering (opzionale)
    int getNumRigidBodies() const;
};