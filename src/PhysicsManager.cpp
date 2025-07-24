#include <PhysicsManager.hpp>
#include <iostream>

struct VertexDescriptor;
// Utility functions for GLM <-> Bullet conversion
btVector3 glmToBt(const glm::vec3& v) {
    return btVector3(v.x, v.y, v.z);
}

glm::vec3 btToGlm(const btVector3& v) {
    return glm::vec3(v.getX(), v.getY(), v.getZ());
}

// PhysicsObject destructor
PhysicsObject::~PhysicsObject() {
    if (body && body->getMotionState()) {
        delete body->getMotionState();
    }
    delete body;
    delete shape;
}

// PhysicsManager implementation
PhysicsManager::PhysicsManager()
    : broadphase(nullptr), collisionConfig(nullptr), dispatcher(nullptr),
      solver(nullptr), dynamicsWorld(nullptr), isGrounded(false),
      groundCheckDistance(0.1f) {
}

PhysicsManager::~PhysicsManager() {
    cleanup();
}

bool PhysicsManager::initialize(bool flyMode_, const PlayerConfig& playerCfg, const BackgroundTerrainConfig& terrainCfg) {
    playerConfig = playerCfg;
    terrainConfig = terrainCfg;
    flyMode = flyMode_;

    try {
        initializePhysicsWorld();
        createTerrain();

        std::cout << "PhysicsManager initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        cleanup();

        std::cerr << "PhysicsManager initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void PhysicsManager::initializePhysicsWorld() {
    // Initialize Bullet Physics
    broadphase = new btDbvtBroadphase();
    collisionConfig = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfig);
    solver = new btSequentialImpulseConstraintSolver();

    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
    if(!flyMode)
        dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
    else
        dynamicsWorld->setGravity(btVector3(0, 0, 0));
}

void PhysicsManager::createTerrain() {
    terrain = std::make_unique<PhysicsObject>();

    // Create a simple ground plane
    btTriangleMesh* terrainTriMesh = new btTriangleMesh();

    float size = terrainConfig.size;
    btVector3 A(-size, -1.8, -size);
    btVector3 B(+size, -1.8, -size);
    btVector3 C(+size, -1.8, +size);
    btVector3 D(-size, -1.8, +size);

    // Add triangles
    terrainTriMesh->addTriangle(A, B, C);
    terrainTriMesh->addTriangle(A, C, D);

    terrain->shape = new btBvhTriangleMeshShape(terrainTriMesh, true);

    btTransform terrainTransform;
    terrainTransform.setIdentity();
    terrainTransform.setOrigin(glmToBt(terrainConfig.position));

    terrain->motionState = new btDefaultMotionState(terrainTransform);

    btRigidBody::btRigidBodyConstructionInfo terrainInfo(0.0f, terrain->motionState, terrain->shape);
    terrain->body = new btRigidBody(terrainInfo);

    dynamicsWorld->addRigidBody(terrain->body);
}

/*
 * This function creates a dummy player object with a capsule shape inside the physics world.
 * The shape is not based on any character model. Will be used configuration settings from `playerConfig`.
 */
void PhysicsManager::addCapsulePlayer() {
    player = std::make_unique<PhysicsObject>();

    // Create capsule shape
    player->shape = new btCapsuleShape(playerConfig.capsuleRadius, playerConfig.capsuleHeight);

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(glmToBt(playerConfig.startPosition));

    player->motionState = new btDefaultMotionState(startTransform);
    player->initialPosition = playerConfig.startPosition;

    btVector3 inertia(0, 0, 0);
    player->shape->calculateLocalInertia(playerConfig.mass, inertia);

    btRigidBody::btRigidBodyConstructionInfo playerInfo(playerConfig.mass, player->motionState, player->shape, inertia);
    player->body = new btRigidBody(playerInfo);

    // Prevent deactivation
    player->body->setActivationState(DISABLE_DEACTIVATION);

    // Set some physics properties
    player->body->setFriction(playerConfig.friction);
    player->body->setRollingFriction(playerConfig.rollingFriction);

    dynamicsWorld->addRigidBody(player->body);
}

void PhysicsManager::addPlayerFromModel(const Model* modelRef) {
    player = std::make_unique<PhysicsObject>();

    // Create shape from model
    player->shape = getShapeFromModel(modelRef);
    if (!player->shape) {
        std::cerr << "Failed to create collision shape from model." << std::endl;
        return;
    }

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(glmToBt(playerConfig.startPosition));

    player->motionState = new btDefaultMotionState(startTransform);
    player->initialPosition = playerConfig.startPosition;

    btVector3 inertia(0, 0, 0);
    player->shape->calculateLocalInertia(playerConfig.mass, inertia);

    btRigidBody::btRigidBodyConstructionInfo playerInfo(playerConfig.mass, player->motionState, player->shape, inertia);
    player->body = new btRigidBody(playerInfo);

    // Prevent deactivation
    player->body->setActivationState(DISABLE_DEACTIVATION);

    // Set some physics properties
    player->body->setFriction(playerConfig.friction);
    player->body->setRollingFriction(playerConfig.rollingFriction);

    dynamicsWorld->addRigidBody(player->body);
}


void PhysicsManager::update(float deltaTime) {
    if (!dynamicsWorld) return;

    // Step the simulation
    dynamicsWorld->stepSimulation(deltaTime, 10);

    // Update grounded state
    isGrounded = flyMode || checkGrounded();
}

bool PhysicsManager::checkGrounded() {
    if (!player || !player->body) return false;

    btTransform playerTransform;
    player->body->getMotionState()->getWorldTransform(playerTransform);
    btVector3 playerPos = playerTransform.getOrigin();

    // Ray cast downward
    btVector3 rayStart = playerPos;
    btVector3 rayEnd = playerPos + btVector3(0, -(playerConfig.capsuleHeight * 0.5f + groundCheckDistance), 0);

    btCollisionWorld::ClosestRayResultCallback rayCallback(rayStart, rayEnd);
    dynamicsWorld->rayTest(rayStart, rayEnd, rayCallback);

    return rayCallback.hasHit();
}

void PhysicsManager::movePlayer(const glm::vec3& moveDirection, bool isRunning) {
    if (!player || !player->body) return;

    float speed = isRunning ? playerConfig.runSpeed : playerConfig.moveSpeed;
    if (flyMode)
        speed += 5;
    glm::vec3 moveDir = moveDirection * speed;

    if (flyMode) {
        // --- flyMode version ---
        if (glm::length(moveDir) > 0.001f) {
            btVector3 desiredVel(moveDir.x, moveDir.y, moveDir.z);
            player->body->setLinearVelocity(desiredVel);
            player->body->activate(true);
        } else {
            // Stop movement except for vertical velocity
            player->body->setLinearVelocity(btVector3(0, 0, 0));
        }
    } else {
        // --- ground mode version ---
        btVector3 currentVel = player->body->getLinearVelocity();
        if (glm::length(moveDir) > 0.001f) {
            btVector3 desiredVel;

            if (isGrounded) {
                // Full control on ground
                desiredVel = btVector3(moveDir.x, currentVel.getY(), moveDir.z);
            } else {
                // Limited air control
                btVector3 airMove = glmToBt(moveDir) * playerConfig.airControl;
                desiredVel = btVector3(
                        currentVel.getX() + airMove.getX(),
                        currentVel.getY(),
                        currentVel.getZ() + airMove.getZ()
                );
            }

            // Smooth interpolation
            float lerpFactor = isGrounded ? 10.0f : 2.0f;
            btVector3 newVel = currentVel.lerp(desiredVel, lerpFactor * (1.0f/60.0f)); // Assuming 60 FPS
            player->body->setLinearVelocity(newVel);

            player->body->activate(true);
        } else if (isGrounded) {
            // Apply damping when not moving
            btVector3 dampedVel(
                    currentVel.getX() * playerConfig.groundDamping,
                    currentVel.getY(),
                    currentVel.getZ() * playerConfig.groundDamping
            );
            player->body->setLinearVelocity(dampedVel);
        }
    }
}

void PhysicsManager::jumpPlayer() {
    if (!player || !player->body || !isGrounded) return;

    player->body->applyCentralImpulse(btVector3(0, playerConfig.jumpForce, 0));
    player->body->activate(true);
}

glm::vec3 PhysicsManager::getPlayerPosition() const {
    if (!player || !player->body) return glm::vec3(0);

    btTransform transform;
    player->body->getMotionState()->getWorldTransform(transform);
    return btToGlm(transform.getOrigin());
}

glm::vec3 PhysicsManager::getPlayerVelocity() const {
    if (!player || !player->body) return glm::vec3(0);

    return btToGlm(player->body->getLinearVelocity());
}

PhysicsObject* PhysicsManager::addStaticBox(const glm::vec3& position, const glm::vec3& size) {
    auto obj = std::make_unique<PhysicsObject>();

    obj->shape = new btBoxShape(glmToBt(size * 0.5f));

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(glmToBt(position));

    obj->motionState = new btDefaultMotionState(transform);
    obj->initialPosition = position;

    btRigidBody::btRigidBodyConstructionInfo info(0.0f, obj->motionState, obj->shape);
    obj->body = new btRigidBody(info);

    dynamicsWorld->addRigidBody(obj->body);

    PhysicsObject* result = obj.get();
    staticObjects.push_back(std::move(obj));
    return result;
}

PhysicsObject* PhysicsManager::addStaticSphere(const glm::vec3& position, float radius) {
    auto obj = std::make_unique<PhysicsObject>();

    obj->shape = new btSphereShape(radius);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(glmToBt(position));

    obj->motionState = new btDefaultMotionState(transform);
    obj->initialPosition = position;

    btRigidBody::btRigidBodyConstructionInfo info(0.0f, obj->motionState, obj->shape);
    obj->body = new btRigidBody(info);

    dynamicsWorld->addRigidBody(obj->body);

    PhysicsObject* result = obj.get();
    staticObjects.push_back(std::move(obj));
    return result;
}


btTransform glmMat4ToBtTransform(const glm::mat4& mat) {
    btMatrix3x3 basis(
            mat[0][0], mat[0][1], mat[0][2],
            mat[1][0], mat[1][1], mat[1][2],
            mat[2][0], mat[2][1], mat[2][2]
    );
    btVector3 origin(mat[3][0], mat[3][1], mat[3][2]);
    btTransform transform;
    transform.setBasis(basis);
    transform.setOrigin(origin);
    return transform;
}

btCollisionShape * PhysicsManager::getShapeFromModel(const Model* modelRef) {
    const Model &model = *modelRef;
    const std::vector<unsigned char> &vertices = model.vertices;
    const std::vector<uint32_t> &indices = model.indices;
    const VertexDescriptor *VD = model.VD;

    const uint32_t stride = VD->Bindings[0].stride;
    const uint32_t posOffset = VD->Position.offset;
    std::vector<float> Xs, Ys, Zs;

    for (size_t i = 0; i + 3 < indices.size(); i += 4) {
        for (int j = 0; j < 4; ++j) {
            const uint32_t idx = indices[i + j];
            const size_t base = idx * stride + posOffset;

            float x = *reinterpret_cast<const float*>(&vertices[base]);
            float y = *reinterpret_cast<const float*>(&vertices[base + 4]);
            float z = *reinterpret_cast<const float*>(&vertices[base + 8]);

            Xs.push_back(x);
            Ys.push_back(y);
            Zs.push_back(z);
        }
    }

    if (Xs.size() >= 9) {
        // At least 3 triangles (9 vertices)
        btTriangleMesh* mesh = new btTriangleMesh();

        // Each 3 vertices form a triangle
        for (size_t i = 0; i + 2 < Xs.size(); i += 3) {
            btVector3 v0(Xs[i], Ys[i], Zs[i]);
            btVector3 v1(Xs[i + 1], Ys[i + 1], Zs[i + 1]);
            btVector3 v2(Xs[i + 2], Ys[i + 2], Zs[i + 2]);
            mesh->addTriangle(v0, v1, v2);
        }

        return new btBvhTriangleMeshShape(mesh, true);
    };

    return nullptr; // No valid shape created
}

void PhysicsManager::addStaticMeshes(Model **modelRefs, Instance **instanceRefs, int instanceCount) {
    for (int instanceIdx=0; instanceIdx<instanceCount; instanceIdx++) {

        // Skip instances that are not used for physics
        if(!instanceRefs[instanceIdx]->usedForPhysics) {
            continue;
        }

        // retrieve instance and model references
        int modelIdx = instanceRefs[instanceIdx]->Mid;
        if (modelIdx < 0 || modelIdx >= instanceCount) {
            std::cerr << "Invalid model index: " << modelIdx << std::endl;
            continue;
        }
        const Instance* instanceRef = instanceRefs[instanceIdx];
        const Model* modelRef = modelRefs[modelIdx];

        auto obj = std::make_unique<PhysicsObject>();

        obj->shape = getShapeFromModel(modelRef);
        btTransform transform = glmMat4ToBtTransform(instanceRef->Wm);
        obj->motionState = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo info(0.0f, obj->motionState, obj->shape);
        obj->body = new btRigidBody(info);

        obj->body->setFriction(0.5f);
        obj->body->setRollingFriction(0.1f);

        dynamicsWorld->addRigidBody(obj->body);
        staticObjects.push_back(std::move(obj));
    }
}

void PhysicsManager::setGravity(const glm::vec3& gravity) {
    if (dynamicsWorld) {
        dynamicsWorld->setGravity(glmToBt(gravity));
    }
}

void PhysicsManager::setPlayerPosition(const glm::vec3& position) {
    if (!player || !player->body) return;

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(glmToBt(position));

    player->body->setWorldTransform(transform);
    player->body->getMotionState()->setWorldTransform(transform);
    player->body->setLinearVelocity(btVector3(0, 0, 0));
    player->body->setAngularVelocity(btVector3(0, 0, 0));
    player->body->activate(true);
}

void PhysicsManager::cleanup() {
    // Clean up physics objects
    staticObjects.clear();
    player.reset();
    terrain.reset();

    // Clean up Bullet Physics
    if (dynamicsWorld) {
        int numRigidBodies = dynamicsWorld->getCollisionObjectArray().size();
        // Remove all objects from world
        for (int i = numRigidBodies - 1; i >= 0; i--) {
            btRigidBody* body = btRigidBody::upcast(dynamicsWorld->getCollisionObjectArray()[i]);
            if (body && body->getMotionState()) {
                delete body->getMotionState();
            }
            dynamicsWorld->removeRigidBody(body);
            delete body;
        }

        delete dynamicsWorld;
        dynamicsWorld = nullptr;
    }

    delete solver;
    delete dispatcher;
    delete collisionConfig;
    delete broadphase;

    solver = nullptr;
    dispatcher = nullptr;
    collisionConfig = nullptr;
    broadphase = nullptr;
}

int PhysicsManager::getNumRigidBodies() const {
    int numRigidBodies = dynamicsWorld->getCollisionObjectArray().size();
    return dynamicsWorld ? numRigidBodies : 0;
}