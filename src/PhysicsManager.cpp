#include <PhysicsManager.hpp>
#include <iostream>
#include <algorithm>

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
      groundCheckDistance(0.1f), slopeAngle(0.0f), canClimbStep(false),
      groundNormal(0, 1, 0), lastGroundedTime(0.0f), coyoteTime(0.1f),
      velocitySmoothing(0.0f), lastVelocity(0, 0, 0) {
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

    // Update grounded state and terrain analysis
    isGrounded = flyMode || checkGrounded();

    // Update coyote time for better jump feel
    if (isGrounded) {
        lastGroundedTime = 0.0f;
    } else {
        lastGroundedTime += deltaTime;
    }

    // Apply additional forces for better movement feel
    if (!flyMode) {
        applyMovementCorrections(deltaTime);
        handleSlopeMovement(deltaTime);
        handleStepClimbing(deltaTime);
    }
}

bool PhysicsManager::checkGrounded() {
    if (!player || !player->body) return false;

    btTransform playerTransform;
    player->body->getMotionState()->getWorldTransform(playerTransform);
    btVector3 playerPos = playerTransform.getOrigin();

    // Multiple ray casts for better ground detection
    std::vector<btVector3> rayOffsets = {
        btVector3(0, 0, 0),                                    // Center
        btVector3(playerConfig.capsuleRadius * 0.7f, 0, 0),   // Right
        btVector3(-playerConfig.capsuleRadius * 0.7f, 0, 0),  // Left
        btVector3(0, 0, playerConfig.capsuleRadius * 0.7f),   // Forward
        btVector3(0, 0, -playerConfig.capsuleRadius * 0.7f)   // Backward
    };

    float rayLength = playerConfig.capsuleHeight * 0.5f + groundCheckDistance;
    bool groundFound = false;
    float closestHitDistance = rayLength;
    btVector3 avgNormal(0, 0, 0);
    int hitCount = 0;

    for (const btVector3& offset : rayOffsets) {
        btVector3 rayStart = playerPos + offset;
        btVector3 rayEnd = rayStart + btVector3(0, -rayLength, 0);

        btCollisionWorld::ClosestRayResultCallback rayCallback(rayStart, rayEnd);
        dynamicsWorld->rayTest(rayStart, rayEnd, rayCallback);

        if (rayCallback.hasHit()) {
            groundFound = true;
            float hitDistance = rayStart.distance(rayCallback.m_hitPointWorld);
            if (hitDistance < closestHitDistance) {
                closestHitDistance = hitDistance;
            }
            avgNormal += rayCallback.m_hitNormalWorld;
            hitCount++;
        }
    }

    if (groundFound && hitCount > 0) {
        avgNormal /= static_cast<float>(hitCount);
        avgNormal.normalize();
        groundNormal = btToGlm(avgNormal);

        // Calculate slope angle
        slopeAngle = acos(std::clamp(avgNormal.dot(btVector3(0, 1, 0)), -1.0f, 1.0f));

        // Check if slope is too steep (configurable max slope angle)
        float maxSlopeAngle = 45.0f * (M_PI / 180.0f); // 45 degrees in radians
        return slopeAngle <= maxSlopeAngle;
    }

    groundNormal = glm::vec3(0, 1, 0);
    slopeAngle = 0.0f;
    return false;
}

bool PhysicsManager::canJump() const {
    return isGrounded || lastGroundedTime <= coyoteTime;
}

void PhysicsManager::movePlayer(const glm::vec3& moveDirection, bool isRunning) {
    if (!player || !player->body) return;

    float speed = isRunning ? playerConfig.runSpeed : playerConfig.moveSpeed;
    if (flyMode)
        speed += 5;

    // moveDirection is already in world space from camera transformation
    glm::vec3 moveDir = moveDirection * speed;

    if (flyMode) {
        // --- flyMode version ---
        if (glm::length(moveDir) > 0.001f) {
            btVector3 desiredVel(moveDir.x, moveDir.y, moveDir.z);
            player->body->setLinearVelocity(desiredVel);
            player->body->activate(true);
        } else {
            player->body->setLinearVelocity(btVector3(0, 0, 0));
        }
    } else {
        // --- Enhanced ground mode version ---
        btVector3 currentVel = player->body->getLinearVelocity();

        if (glm::length(moveDir) > 0.001f) {
            // Apply slope projection only if we're on a significant slope
            glm::vec3 finalMoveDir = moveDir;
            if (isGrounded && slopeAngle > 0.1f) {
                finalMoveDir = projectMovementOntoSlope(moveDir);
            }

            btVector3 desiredVel;
            if (isGrounded) {
                // Full control on ground
                desiredVel = btVector3(finalMoveDir.x, currentVel.getY(), finalMoveDir.z);

                // Add slight upward force on slopes to prevent sliding
                if (slopeAngle > 0.2f) {
                    float slopeFactor = sin(slopeAngle) * 1.5f;
                    desiredVel.setY(currentVel.getY() + slopeFactor);
                }
            } else {
                // Enhanced air control with momentum preservation
                btVector3 airMove = btVector3(finalMoveDir.x, 0, finalMoveDir.z) * playerConfig.airControl;
                float maxAirSpeed = speed * 1.2f;

                btVector3 newHorizontalVel = btVector3(currentVel.getX(), 0, currentVel.getZ()) + airMove;
                if (newHorizontalVel.length() > maxAirSpeed) {
                    newHorizontalVel = newHorizontalVel.normalized() * maxAirSpeed;
                }

                desiredVel = btVector3(newHorizontalVel.getX(), currentVel.getY(), newHorizontalVel.getZ());
            }

            // Enhanced smooth interpolation with adaptive lerp factor
            float lerpFactor = isGrounded ? 10.0f : 2.0f;
            if (isRunning && isGrounded) lerpFactor *= 1.3f;

            btVector3 newVel = currentVel.lerp(desiredVel, lerpFactor * (1.0f/60.0f));
            player->body->setLinearVelocity(newVel);
            player->body->activate(true);

        } else if (isGrounded) {
            // Enhanced damping
            float dampingFactor = playerConfig.groundDamping;

            // Stronger damping on steep slopes to prevent sliding
            if (slopeAngle > 0.3f) {
                dampingFactor *= 0.6f;
            }

            btVector3 dampedVel(
                currentVel.getX() * dampingFactor,
                currentVel.getY(),
                currentVel.getZ() * dampingFactor
            );
            player->body->setLinearVelocity(dampedVel);
        }

        // Store velocity for smoothing calculations
        lastVelocity = btToGlm(player->body->getLinearVelocity());
    }
}

glm::vec3 PhysicsManager::projectMovementOntoSlope(const glm::vec3& movement) {
    if (slopeAngle < 0.1f || glm::length(movement) < 0.001f) {
        return movement; // Flat ground or no movement, no projection needed
    }

    // Project the movement vector onto the slope plane
    // Remove the component of movement that's perpendicular to the slope
    glm::vec3 projectedMovement = movement - glm::dot(movement, groundNormal) * groundNormal;

    // Maintain the original movement magnitude for consistent speed
    if (glm::length(projectedMovement) > 0.001f) {
        float originalLength = glm::length(movement);
        projectedMovement = glm::normalize(projectedMovement) * originalLength;
    }

    return projectedMovement;
}

void PhysicsManager::handleSlopeMovement(float deltaTime) {
    if (!isGrounded || slopeAngle < 0.1f) return;

    btVector3 currentVel = player->body->getLinearVelocity();

    // Prevent sliding down slopes when not moving
    if (abs(currentVel.getX()) < 0.1f && abs(currentVel.getZ()) < 0.1f) {
        // Apply counter-force to gravity on slopes
        float antiSlideForce = sin(slopeAngle) * 9.81f * playerConfig.mass;
        btVector3 slopeUp = glmToBt(groundNormal) * antiSlideForce;
        player->body->applyCentralForce(slopeUp);
    }
}

void PhysicsManager::handleStepClimbing(float deltaTime) {
    if (!isGrounded) return;

    btTransform playerTransform;
    player->body->getMotionState()->getWorldTransform(playerTransform);
    btVector3 playerPos = playerTransform.getOrigin();
    btVector3 currentVel = player->body->getLinearVelocity();

    // Only check for steps if moving horizontally
    if (abs(currentVel.getX()) < 0.1f && abs(currentVel.getZ()) < 0.1f) return;

    // Cast forward to detect steps
    glm::vec3 horizontalVel = glm::normalize(glm::vec3(currentVel.getX(), 0, currentVel.getZ()));
    btVector3 forwardDir = glmToBt(horizontalVel);

    float maxStepHeight = 0.3f; // Configurable step height
    float stepCheckDistance = playerConfig.capsuleRadius + 0.1f;

    // Check for obstacle ahead
    btVector3 rayStart = playerPos + btVector3(0, maxStepHeight * 0.5f, 0);
    btVector3 rayEnd = rayStart + forwardDir * stepCheckDistance;

    btCollisionWorld::ClosestRayResultCallback forwardRay(rayStart, rayEnd);
    dynamicsWorld->rayTest(rayStart, rayEnd, forwardRay);

    if (forwardRay.hasHit()) {
        // Check if there's a valid step surface above
        btVector3 stepCheckStart = forwardRay.m_hitPointWorld + btVector3(0, maxStepHeight, 0);
        btVector3 stepCheckEnd = stepCheckStart + btVector3(0, -maxStepHeight * 1.5f, 0);

        btCollisionWorld::ClosestRayResultCallback stepRay(stepCheckStart, stepCheckEnd);
        dynamicsWorld->rayTest(stepCheckStart, stepCheckEnd, stepRay);

        if (stepRay.hasHit()) {
            float stepHeight = stepRay.m_hitPointWorld.getY() - (playerPos.getY() - playerConfig.capsuleHeight * 0.5f);

            if (stepHeight > 0.05f && stepHeight <= maxStepHeight) {
                // Apply upward impulse to climb the step
                float stepForce = stepHeight * playerConfig.mass * 15.0f;
                player->body->applyCentralImpulse(btVector3(0, stepForce, 0));
                canClimbStep = true;
            }
        }
    }
}

void PhysicsManager::applyMovementCorrections(float deltaTime) {
    if (!player || !player->body) return;

    btVector3 currentVel = player->body->getLinearVelocity();

    // Limit maximum falling speed
    float maxFallSpeed = -20.0f;
    if (currentVel.getY() < maxFallSpeed) {
        player->body->setLinearVelocity(btVector3(currentVel.getX(), maxFallSpeed, currentVel.getZ()));
    }

    // Apply additional drag in air for more realistic movement
    if (!isGrounded) {
        float airDrag = 0.98f;
        btVector3 draggedVel = currentVel * airDrag;
        player->body->setLinearVelocity(draggedVel);
    }
}

void PhysicsManager::jumpPlayer() {
    if (!player || !player->body || !canJump()) return;

    // Enhanced jump with slope consideration
    btVector3 jumpDirection(0, 1, 0);

    if (isGrounded && slopeAngle > 0.1f) {
        // Jump slightly away from steep slopes
        btVector3 slopeNormal = glmToBt(groundNormal);
        jumpDirection = (jumpDirection + slopeNormal * 0.3f).normalized();
    }

    btVector3 jumpImpulse = jumpDirection * playerConfig.jumpForce;

    // Reduce jump force slightly if in coyote time (not actually grounded)
    if (!isGrounded && lastGroundedTime <= coyoteTime) {
        jumpImpulse *= 0.8f;
    }

    player->body->applyCentralImpulse(jumpImpulse);
    player->body->activate(true);

    // Reset coyote time after jumping
    lastGroundedTime = coyoteTime + 1.0f;
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

float PhysicsManager::getGroundSlopeAngle() const {
    return slopeAngle * (180.0f / M_PI); // Convert to degrees
}

glm::vec3 PhysicsManager::getGroundNormal() const {
    return groundNormal;
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

    // Reset movement state
    lastGroundedTime = 0.0f;
    isGrounded = false;
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