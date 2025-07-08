// This has been adapted from the Vulkan tutorial
#include <sstream>

#include <json.hpp>

#include <PhysicsManager.hpp>
//#include "modules/Starter.hpp"
//#include "modules/Scene.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Animations.hpp"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "character/char_manager.hpp"
#include "character/character.hpp"

//TODO generale: Ripulisci e Commenta tutto il main

/*TODO:
    - quando ridimensiono la finestra crasha, perchè?
    - aggiusta character che non viene renderizzato
    - prova a gestire meglio il sample della shadow map in terrain shader, soprattuto per
       le coordinate che escono fuori dal light clip space e per il bias term (magari rendilo dinamico...)
    - Pensa se aggiungere supporto alle ombre anche su altre tecniche
    - attenziona il fatto che le ombre hanno i bordi jagged (seghettati)
    - Alcune mesh come vegetation hanno maskMode alpha, quindi l'ombra dovrebbe esserci
        solo per i punti con la texture.a > 0.5, come gestire questo?
*/

/** If true, gravity and inertia are disabled
 And vertical movement (along y, thus actual fly) is enabled.
 */
const bool FLY_MODE = true;

// The uniform buffer object used in this example
struct VertexChar {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
	glm::uvec4 jointIndices;
	glm::vec4 weights;
};

struct VertexSimp {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};

struct skyBoxVertex {
	glm::vec3 pos;
};

struct VertexTan {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
	glm::vec4 tan;
};

struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
};

struct UniformBufferObjectChar {
	alignas(16) glm::vec4 debug1;
	alignas(16) glm::mat4 mvpMat[65];
	alignas(16) glm::mat4 mMat[65];
	alignas(16) glm::mat4 nMat[65];
};

struct UniformBufferObjectSimp {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct ShadowMapUBO {
	alignas(16) glm::mat4 lightVP;
	alignas(16) glm::mat4 model;
};
struct ShadowMapUBOChar {
	alignas(16) glm::mat4 lightVP;
	alignas(16) glm::mat4 model[65];
};
struct ShadowClipUBO {
	alignas(16) glm::mat4 lightVP;
	alignas(16) glm::vec4 debug;
};

struct skyBoxUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
};

struct DebugUBO {
    alignas(16) glm::vec4 debug;
};

struct TimeUBO {
    alignas(4) float time; // scalar
};

struct SgAoMaterialFactorsUBO {
    alignas(16) glm::vec3 diffuseFactor;      // (RGBA)
    alignas(16) glm::vec3 specularFactor;     // (RGB)
    alignas(4) float glossinessFactor;  // scalar
    alignas(4) float aoFactor; // scalar
} ;

struct TerrainFactorsUBO {
    alignas(4) float maskBlendFactor;
    alignas(4) float tilingFactor;
};

// MAIN ! 
class CGProject : public BaseProject {
	protected:
	// Here you list all the Vulkan objects you need:
	
	// Descriptor Layouts [what will be passed to the shaders]
	DescriptorSetLayout DSLlocalChar, DSLlocalSimp, DSLlocalPBR,
        DSLglobal, DSLskyBox, DSLterrain, DSLsgAoFactors, DSLterrainFactors,
        DSLwaterVert, DSLwaterFrag, DSLgrass;
    DescriptorSetLayout DSLshadowMap, DSLshadowMapChar;

	// Vertex formants, Pipelines [Shader couples] and Render passes
	VertexDescriptor VDchar;
	VertexDescriptor VDsimp;
	VertexDescriptor VDskyBox;
	VertexDescriptor VDtan;
	RenderPass RPshadow, RP;
	Pipeline Pchar, PsimpObj, PskyBox, Pterrain, P_PBR_SpecGloss, PWater, Pgrass;
    Pipeline PshadowMap, PshadowMapChar, PshadowMapSky, PshadowMapWater;

	// Models, textures and Descriptors (values assigned to the uniforms)
	Scene SC;
	std::vector<VertexDescriptorRef>  VDRs;
	std::vector<TechniqueRef> PRs;

	// PhysicsManager for collision detection
	PhysicsManager PhysicsMgr;
	PlayerConfig physicsConfig;

	// to provide textual feedback
	TextMaker txt;
	
	// Other application parameters
	float Ar;	// Aspect ratio

	glm::mat4 ViewPrj;
	glm::mat4 World;
	glm::vec3 cameraPos;

    // Camera rotation controls
	float Yaw = glm::radians(0.0f);
	float Pitch = glm::radians(0.0f);
	float Roll = glm::radians(0.0f);
    float relDir = glm::radians(0.0f);
    float dampedRelDir = glm::radians(0.0f);
    glm::vec3 dampedCamPos = physicsConfig.startPosition;
    // Player starting point
    const glm::vec3 StartingPosition = physicsConfig.startPosition;
    // Camera FOV-y, Near Plane and Far Plane
    const float FOVy = glm::radians(45.0f);
    const float worldNearPlane = 0.1f;
    const float worldFarPlane = 500.f;
    // Camera target height and distance
    float camHeight = 1.5;
    float camDist = 5;
    // Camera Pitch limits
    const float minPitch = glm::radians(-40.0f);
    const float maxPitch = glm::radians(80.0f);
    // Rotation and motion speed
    const float ROT_SPEED = glm::radians(120.0f);
    const float MOVE_SPEED_BASE = physicsConfig.moveSpeed;
    const float MOVE_SPEED_RUN = physicsConfig.runSpeed;
    const float JUMP_FORCE = physicsConfig.jumpForce;
    const float MAX_CAM_DIST = 7.5;
    const float MIN_CAM_DIST = 1.5;


//    const glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    const glm::vec4 lightColor = glm::vec4(1.0f, 0.4f, 0.4f, 1.0f);
    /**
     * Matrix defining the light rotation to apply to +z axis to get the light direction.
     * It is used
     *  - applied to +z axis to compute light direction
     *  - applied in its inverse form to compute the light projection matrix for shadow map
     */
    const glm::mat4 lightRotation = glm::rotate(glm::mat4(1), glm::radians(-29.0f),
        glm::vec3(0.0f,1.0f,0.0f)) * glm::rotate(glm::mat4(1), glm::radians(-30.0f),
         glm::vec3(1.0f,0.0f,0.0f)) * glm::rotate(glm::mat4(1), glm::radians(0.0f),
         glm::vec3(0.0f,0.0f,1.0f));
    /**
     * Directional of the unique directional light in the scene --> Represents the sun light
     * It points towards the light source
     */
    const glm::vec3 lightDir = glm::vec3(lightRotation * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    /**
     * Parameters used for orthogonal projection of the scene from light pov, in shadow mapping render pass
     */
    const float lightWorldLeft = -150.0f, lightWorldRight = 150.0f;
    const float lightWorldBottom = lightWorldLeft * 1.0f, lightWorldTop = lightWorldRight * 1.0f;     // Now the shadow map is square (2048x2048)
    const float lightWorldNear = -150.0f, lightWorldFar = 200.0f;
    /**
     * Actual projection matrix used to render the scene from light pov, in shadow mapping render pass
     */
    glm::mat4 lightVP;
    //TODO: capisci se i bounds trovati per ortho vanno sempre bene o devono essere dinamici
    /** Debug vector present in DSL for shadow map. Basic version is vec4(0,0,0,0)
     * if debugLightView.x == 1.0, the terrain renders only white if lit and black if in shadow
     * if debugLightView.y == 1.0, the light's clip space is visualized instead of the basic perspective view
     */
    glm::vec4 debugLightView = glm::vec4(0.0);

	glm::vec4 debug1 = glm::vec4(0);

	// To manage NPSs
	CharManager charManager;

    // Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 1000;
		windowHeight = 750;
		windowTitle = "CGProject - Medieval Village Sim";
    	windowResizable = GLFW_FALSE;
        // TODO: da fixare, se imposto resizable, quando faccio resize l'app crasha

		// Initial aspect ratio
		Ar = 4.0f / 3.0f;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		Ar = (float)w / (float)h;
		// Update Render Pass
		RP.width = w;
		RP.height = h;
        // Note: the shadow render pass has fixed square resolution (2048x2048), doesn't need to be resized

		// updates the textual output
		txt.resizeScreen(w, h);
	}

	void localInit() {

        // ------ LIGHT PROJECTINO MAT COMPUTATION ------
        /* Light projection matrix is computed using an orthographic projection
         * A rotation is beforehand applied, to take into account the light direction --> projection from light's pov
         *      To do this, the inverse of lightRotation matrix is applied
         *      (inverse because we need to invert the rotation of the world scene to get the light's view)
         * We need as output NDC coordinates (Normalized Device/Screen Coord) the range [-1,1] for x and y, and [0,1] for z
         * To do this used glm::orth, fixing
            - y: is inverted wrt to vulkan    --> apply scale of factor -1
            - z: in vulkan is [0,1], but ortho (for glm) computes it in [-1,1]    --> apply scale of factor 0.5 and translation of 0.5
        */
        auto vulkanCorrection =
                glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, 0.5f)) *   // translation of axis z
                glm::scale(glm::mat4(1.0), glm::vec3(1.0f, -1.0f, 0.5f));       // scale of axis y and z
        glm::mat4 lightProj = vulkanCorrection * glm::ortho(lightWorldLeft, lightWorldRight, lightWorldBottom, lightWorldTop, lightWorldNear, lightWorldFar);
        lightVP = lightProj * glm::inverse(lightRotation);


		// --------- DSL INITIALIZATION ---------
		DSLshadowMap.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowMapUBO), 1}
        });
        DSLshadowMapChar.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowMapUBOChar), 1}
        });
		DSLglobal.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
        });
		DSLlocalChar.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectChar), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
        });
		DSLlocalSimp.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1}
        });
		DSLskyBox.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(skyBoxUniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(DebugUBO), 1},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
		});
		DSLwaterVert.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(TimeUBO), 1},
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp),1},
			{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowClipUBO),1},
		});
        DSLwaterFrag.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(GlobalUniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1},
			{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1},
			{5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1},
			{6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1},
			{7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6, 1},
			{8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7, 1}
		});
        DSLgrass.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp), 1},
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(TimeUBO), 1},
			{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowClipUBO), 1},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1}
		});
        DSLterrain.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,           1},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1,           1},
            {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2,           1},
            {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3,           1},
            {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4,           1},
            {6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5,           1},
            {7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6,           1},
            {8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7,           1},
            {9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 8,           1},
            {10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 9,          1},
            {11, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowClipUBO), 1},
        });
        DSLterrainFactors.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(TerrainFactorsUBO), 1},
        });
		DSLlocalPBR.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,                     1},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1,                     1},
            {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2,                     1},
            {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3,                     1},
            {5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowClipUBO),            1},
        });
        DSLsgAoFactors.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SgAoMaterialFactorsUBO), 1},
        });


        // --------- VERTEX DESCRIPTORS INITIALIZATION ---------
        VDchar.init(this, {
                {0, sizeof(VertexChar), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(VertexChar, pos),            sizeof(glm::vec3),  POSITION},
                {0, 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(VertexChar, norm),           sizeof(glm::vec3),  NORMAL},
                {0, 2, VK_FORMAT_R32G32_SFLOAT,       offsetof(VertexChar, UV),             sizeof(glm::vec2),  UV},
                {0, 3, VK_FORMAT_R32G32B32A32_UINT,   offsetof(VertexChar, jointIndices),   sizeof(glm::uvec4), JOINTINDEX},
                {0, 4, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexChar, weights),        sizeof(glm::vec4),  JOINTWEIGHT}
        });
        VDsimp.init(this, {
                {0, sizeof(VertexSimp), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexSimp, pos),  sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexSimp, norm), sizeof(glm::vec3), NORMAL},
                {0, 2, VK_FORMAT_R32G32_SFLOAT,    offsetof(VertexSimp, UV),   sizeof(glm::vec2), UV}
        });
        VDskyBox.init(this, {
                {0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos), sizeof(glm::vec3),POSITION}
        });
        VDtan.init(this, {
                {0, sizeof(VertexTan), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,      offsetof(VertexTan, pos),   sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32B32_SFLOAT,      offsetof(VertexTan, norm),  sizeof(glm::vec3), NORMAL},
                {0, 2, VK_FORMAT_R32G32_SFLOAT,         offsetof(VertexTan, UV),    sizeof(glm::vec2), UV},
                {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT,   offsetof(VertexTan, tan),   sizeof(glm::vec4), TANGENT}
        });

        // Specification of names and mappings of VDs for scene.json
		VDRs.resize(4);
		VDRs[0].init("VDchar",   &VDchar);
		VDRs[1].init("VDsimp",   &VDsimp);
		VDRs[2].init("VDskybox", &VDskyBox);
		VDRs[3].init("VDtan",    &VDtan);


        // --------- RENDER PASSES INITIALIZATION ---------
        /* RP 1: used for shadow map, rendered from light point of view
         * The options set, different by default ones, are for writing a depth buffer instead of color
         * All related options are set in the RenderPass::getStandardAttchmentsProperties specifing AT_DEPTH_ONLY
         *      (e.g. depth write enabled, color write disabled, initial clear value in stencil of 1.0, ...)  */
        RPshadow.init(this, 2048, 2048,-1,
                      RenderPass::getStandardAttchmentsProperties(StockAttchmentsConfiguration::AT_DEPTH_ONLY, this),
                      RenderPass::getStandardDependencies(StockAttchmentsDependencies::ATDEP_DEPTH_TRANS), true);
		/* RP 2: used for the main rendering
		 * Now default options of starter.hpp are used, so it will write color and depth  */
        RP.init(this);
		RP.properties[0].clearValue = {0.0f,0.9f,1.0f,1.0f};

        /* Actual creation of the Render Passes
         * It is done here to be sure the attachment of RPshadow is created and can be linked as input in RP */
        RPshadow.create();
        RP.create();

        PshadowMap.init(this, &VDtan, "shaders/shadowMapShader.vert.spv", "shaders/shadowMapShader.frag.spv", {&DSLshadowMap});
        PshadowMap.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);  // or VK_COMPARE_OP_LESS
        PshadowMap.setCullMode(VK_CULL_MODE_BACK_BIT);
        PshadowMap.setPolygonMode(VK_POLYGON_MODE_FILL);

        PshadowMapChar.init(this, &VDchar, "shaders/shadowMapShaderChar.vert.spv", "shaders/shadowMapShader.frag.spv", {&DSLshadowMapChar});
        PshadowMapChar.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);  // or VK_COMPARE_OP_LESS
        PshadowMapChar.setCullMode(VK_CULL_MODE_BACK_BIT);
        PshadowMapChar.setPolygonMode(VK_POLYGON_MODE_FILL);

        PshadowMapSky.init(this, &VDskyBox, "shaders/shadowMapShaderSky.vert.spv", "shaders/shadowMapShader.frag.spv", {});
        PshadowMapWater.init(this, &VDsimp, "shaders/shadowMapShaderWater.vert.spv", "shaders/shadowMapShader.frag.spv", {});

		Pchar.init(this, &VDchar, "shaders/CharacterVertex.vert.spv", "shaders/CharacterCookTorrance.frag.spv", {&DSLglobal, &DSLlocalChar});
		PsimpObj.init(this, &VDsimp, "shaders/GeneralSimplePosNormUV.vert.spv", "shaders/GeneralCookTorrance.frag.spv", {&DSLglobal, &DSLlocalSimp});

		PskyBox.init(this, &VDskyBox, "shaders/SkyBoxShader.vert.spv", "shaders/SkyBoxShader.frag.spv", {&DSLskyBox});
		PskyBox.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		PskyBox.setCullMode(VK_CULL_MODE_BACK_BIT);
		PskyBox.setPolygonMode(VK_POLYGON_MODE_FILL);

        PWater.init(this, &VDsimp, "shaders/WaterShader.vert.spv", "shaders/WaterShader.frag.spv", {&DSLwaterVert, &DSLwaterFrag});
        PWater.setTransparency(true);

        Pgrass.init(this, &VDtan, "shaders/GrassShader.vert.spv", "shaders/GrassShader.frag.spv", {&DSLglobal, &DSLgrass});
        Pterrain.init(this, &VDtan, "shaders/TerrainShader.vert.spv", "shaders/TerrainShader.frag.spv", {&DSLglobal, &DSLterrain, &DSLterrainFactors});
		P_PBR_SpecGloss.init(this, &VDtan, "shaders/PBR_SpecGloss.vert.spv", "shaders/PBR_SpecGloss.frag.spv", {&DSLglobal, &DSLlocalPBR, &DSLsgAoFactors});


        // --------- TECHNIQUES INITIALIZATION ---------
        PRs.resize(7);
		PRs[0].init("CookTorranceChar", {
            {&PshadowMapChar, {{}} },
            {&Pchar, {
                {},
                {
                    {true,  0, {} }
                }
            }}
        }, 1, &VDchar);
		PRs[1].init("CookTorranceNoiseSimp", {
            {&PshadowMapWater, {{}} },
            {&PsimpObj,   {
                {},
                {
                  {true,  0, {}},
                  {true,  1, {}}
                }
            }}
        },2, &VDsimp);
		PRs[2].init("SkyBox", {
            {&PshadowMapSky, {} },
            {&PskyBox,    {
                {
                    {true,  0, {}}
                }
            }}
        }, 1, &VDskyBox);
        PRs[3].init("Terrain", {
            {&PshadowMap, {{}} },
            {&Pterrain,   {
                {},
                {
                    {true,  0, {} },
                    {true,  1, {} },
                    {true,  2, {} },
                    {true,  3, {} },
                    {true,  4, {} },
                    {true,  5, {} },
                    {true,  6, {} },
                    {true,  7, {} },
                    {true,  8, {} },
                    {false,  9, RPshadow.attachments[0].getViewAndSampler() }
                },
                {}
            }}
        }, 9, &VDtan);
        PRs[4].init("Water", {
            {&PshadowMapWater, {} },
            {&PWater, {
                {},
                {
                    {true, 0, {} },
                    {true, 1, {} },     // 6 textures for cubemap faces
                    {true, 2, {} },     // Order is: +x, -x, +y, -y, +z, -z
                    {true, 3, {} },
                    {true, 4, {} },
                    {true, 5, {} },
                    {true, 6, {} },
                    {true, 7, {} }
                }
            }}
        }, 8, &VDsimp);
        PRs[5].init("Grass", {
            {&PshadowMap, {{}} },
            {&Pgrass,     {
                {},
                {
                    {true, 0, {}},
                    {true, 1, {}}
                }
            }}
        }, 2, &VDtan);
        PRs[6].init("PBR_sg", {
            {&PshadowMap, {{}} },
            {&P_PBR_SpecGloss, {
                {},
                {
                    {true,  0, {}},     // albedo
                    {true,  1, {}},     // normal
                    {true,  2, {}},     // specular / glossiness
                    {true,  3, {}},     // ambient occlusion
                },
                {}
            }}
        }, 4, &VDtan);


        // --------- SCENE INITIALIZATION ---------
		// sets the size of the Descriptor Set Pool --> Overprovisioned!
		DPSZs.uniformBlocksInPool = 1000;
		DPSZs.texturesInPool = 1000;
		DPSZs.setsInPool = 1000;
		
        std::cout << "\nLoading the scene\n\n";
		if(SC.init(this, 2, VDRs, PRs, "assets/models/scene.json") != 0) {
			std::cout << "ERROR LOADING THE SCENE\n";
			exit(0);
		}

		// Characters and animations initialization
		if (charManager.init("assets/models/scene.json", SC.As) != 0) {
			std::cout << "ERROR LOADING CHARACTERs\n";
			exit(0);
		}

		// initializes the textual output
		txt.init(this, windowWidth, windowHeight);

		// submits the main command buffer
		submitCommandBuffer("main", 0, populateCommandBufferAccess, this);

		txt.print(1.0f, 1.0f, "FPS:",1,"CO",false,false,true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});

		// Initialize PhysicsManager
		if(!PhysicsMgr.initialize(FLY_MODE)) {
			exit(0);
		}

		// Add static meshes for collision detection
		PhysicsMgr.addStaticMeshes(SC.M, SC.I, SC.InstanceCount);
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// creates the render pass
        std::cout << "Creating pipelines and descriptor sets\n";
        // Render passes already created in localInit()

        std::cout << "Creating pipelines\n";
		Pchar.create(&RP);
		PsimpObj.create(&RP);
		PskyBox.create(&RP);
        PWater.create(&RP);
        Pgrass.create(&RP);
		Pterrain.create(&RP);
        P_PBR_SpecGloss.create(&RP);
        PshadowMap.create(&RPshadow);
        PshadowMapChar.create(&RPshadow);
        PshadowMapSky.create(&RPshadow);
        PshadowMapWater.create(&RPshadow);

        std::cout << "Creating descriptor sets\n";

		SC.pipelinesAndDescriptorSetsInit();
		txt.pipelinesAndDescriptorSetsInit();

        std::cout << "pipelinesAndDescriptorSetsInit done\n";
    }

	void pipelinesAndDescriptorSetsCleanup() {
		Pchar.cleanup();
		PsimpObj.cleanup();
		PskyBox.cleanup();
        PWater.cleanup();
        Pgrass.cleanup();
		Pterrain.cleanup();
        P_PBR_SpecGloss.cleanup();
        PshadowMap.cleanup();
        PshadowMapChar.cleanup();
        PshadowMapSky.cleanup();
        PshadowMapWater.cleanup();
		RPshadow.cleanup();
        RP.cleanup();

		SC.pipelinesAndDescriptorSetsCleanup();
		txt.pipelinesAndDescriptorSetsCleanup();
	}

	void localCleanup() {
		DSLlocalChar.cleanup();
		DSLlocalSimp.cleanup();
		DSLlocalPBR.cleanup();
        DSLsgAoFactors.cleanup();
		DSLskyBox.cleanup();
        DSLwaterVert.cleanup();
        DSLwaterFrag.cleanup();
        DSLgrass.cleanup();
		DSLterrain.cleanup();
        DSLterrainFactors.cleanup();
		DSLglobal.cleanup();
        DSLshadowMap.cleanup();
        DSLshadowMapChar.cleanup();

		Pchar.destroy();	
		PsimpObj.destroy();
		PskyBox.destroy();		
        PWater.destroy();
        Pgrass.destroy();
        P_PBR_SpecGloss.destroy();
		Pterrain.destroy();
        PshadowMap.destroy();
        PshadowMapChar.destroy();
        PshadowMapSky.destroy();
        PshadowMapWater.destroy();

        RPshadow.destroy();
		RP.destroy();

		SC.localCleanup();	
		txt.localCleanup();

		charManager.cleanup();
	}

	static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params) {
		// Simple trick to avoid having always 'T->' in the code that populates the command buffer!
        std::cout << "Populating command buffer for " << currentImage << "\n";
		CGProject *T = (CGProject *)Params;
		T->populateCommandBuffer(commandBuffer, currentImage);
	}
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        //TODO: chatgpt usa solo imageId=0 per RPshadow, ma così sembra funzionare... capisci meglio
        RPshadow.begin(commandBuffer, currentImage);
        SC.populateCommandBuffer(commandBuffer, 0, currentImage);
        RPshadow.end(commandBuffer);

		RP.begin(commandBuffer, currentImage);
		SC.populateCommandBuffer(commandBuffer, 1, currentImage);
        RP.end(commandBuffer);
	}

	void updateUniformBuffer(uint32_t currentImage) {
		static bool debounce = false;
		static int curDebounce = 0;

        // TODO Detect running by pressing SHIFT key (currently hardcoded as walking all the time)

        // Handle of command keys
        {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
                glfwSetWindowShouldClose(window, GL_TRUE);
            }

            if (glfwGetKey(window, GLFW_KEY_1)) {
                if (!debounce) {
                    debounce = true;
                    curDebounce = GLFW_KEY_1;
                    std::cout << "Toggling debug light view\n";

                    debugLightView.x = 1.0 - debugLightView.x;
                }
            } else if (curDebounce == GLFW_KEY_1 && debounce) {
                debounce = false;
                curDebounce = 0;
                std::cout << "Debug light view toggled\n";
            }
            if (glfwGetKey(window, GLFW_KEY_2)) {
                if (!debounce) {
                    debounce = true;
                    curDebounce = GLFW_KEY_2;

                    debugLightView.y = 1.0 - debugLightView.y;
                }
            } else if (curDebounce == GLFW_KEY_2 && debounce) {
                debounce = false;
                curDebounce = 0;
            }

            if (glfwGetKey(window, GLFW_KEY_SPACE)) {
                if (!debounce) {
                    debounce = true;
                    curDebounce = GLFW_KEY_SPACE;

				debug1.z = (float)(((int)debug1.z + 1) % 65);
            std::cout << "Showing bone index: " << debug1.z << "\n";
			}
		} else {
			if((curDebounce == GLFW_KEY_P) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		static int curAnim = 0;
		static AnimBlender* AB = charManager.getCharacters()[0]->getAnimBlender();
		if(glfwGetKey(window, GLFW_KEY_O)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_O;

				curAnim = (curAnim + 1) % 5;
				AB->Start(curAnim, 0.5);
				std::cout << "Playing anim: " << curAnim << "\n";
			}
		} else {
			if((curDebounce == GLFW_KEY_O) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            if (!debounce) {
                debounce = true;
                curDebounce = GLFW_KEY_SPACE;

                PhysicsMgr.jumpPlayer();
            }
        } else if (curDebounce == GLFW_KEY_SPACE && debounce) {
            debounce = false;
            curDebounce = 0;
        }

		// Handle the E key for Character interaction
		glm::vec3 playerPos = cameraPos;		// TODO: sostituire con la posizione del giocatore una volta implementato il player
		if(glfwGetKey(window, GLFW_KEY_E)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_E;

				auto nearest = charManager.getNearestCharacter(playerPos);
				// glm::distance(nearest->getPosition(), playerPos))
				if (nearest) {
					nearest->interact();
					txt.print(0.5f, 0.1f, nearest->getCurrentDialogue(), 1, "CO", false, false, true, TAL_CENTER, TRH_CENTER, TRV_TOP, {1,1,1,1}, {0,0,0,0.5});
					std::cout << "Character in state : " << nearest->getState() << "\n";
				}
				else {
					std::cout << "No Character nearby to interact with.\n";
				}
			}
		} else {
			if((curDebounce == GLFW_KEY_E) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		// moves the view
		float deltaT = GameLogic();

		// updated the animation
		const float SpeedUpAnimFact = 0.85f;
		AB->Advance(deltaT * SpeedUpAnimFact);
		// defines the global parameters for the uniform
		const glm::mat4 lightView = glm::rotate(glm::mat4(1), glm::radians(-30.0f), glm::vec3(0.0f,1.0f,0.0f)) * glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(1.0f,0.0f,0.0f));
		const glm::vec3 lightDir = glm::vec3(lightView * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));


        // ----- UPDATE UNIFORMS -----
        //TODO per ora le prime 2 tecniche, che non stiamo usando, non hanno implementato
        // il passaggio a visione ortografica quando premo 2

        // Common uniforms and general variables
        int instanceId;
        int techniqueId = -1;
		GlobalUniformBufferObject gubo{
            .lightDir = lightDir,
            .lightColor = lightColor,
            .eyePos = cameraPos
        };
        ShadowMapUBO shadowUbo{
            .lightVP = lightVP
        };
        ShadowClipUBO shadowClipUbo{
            .lightVP = lightVP,
            .debug = debugLightView
        };
        DebugUBO debugUbo{debugLightView};
        UniformBufferObjectSimp ubos{};
        TimeUBO timeUbo{.time = static_cast<float>(glfwGetTime())};
//        TimeUBO timeUbo{.time = glfwGetTime()};


		// TECHNIQUE Character
        techniqueId++;
		UniformBufferObjectChar uboc{
            .debug1 = debug1
        };
        ShadowMapUBOChar shadowCharUbo {
            .lightVP = lightVP,
        };
		glm::mat4 AdaptMat =
			glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f,0.0f,0.0f));
		static SkeletalAnimation* SKA = charManager.getCharacters()[0]->getSkeletalAnimation();
		SKA->Sample(*AB);
		std::vector<glm::mat4> *TMsp = SKA->getTransformMatrices();

        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
			for(int im = 0; im < TMsp->size(); im++) {
				uboc.mMat[im]   = SC.TI[techniqueId].I[instanceId].Wm * AdaptMat * (*TMsp)[im];
				uboc.mvpMat[im] = ViewPrj * uboc.mMat[im];
				uboc.nMat[im] = glm::inverse(glm::transpose(uboc.mMat[im]));
                shadowCharUbo.model[im] = SC.TI[techniqueId].I[instanceId].Wm * AdaptMat * (*TMsp)[im];
			}

			SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowCharUbo, 0); // Set 0
			SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &gubo, 0); // Set 0
			SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &uboc, 0);  // Set 1
		}

		// TECHNIQUE Simple objects
        techniqueId++;
		for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
			ubos.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
			ubos.mvpMat = ViewPrj * ubos.mMat;
			ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));
            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
			SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &gubo, 0); // Set 0
			SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &ubos, 0);  // Set 1
		}

        // TECHNIQUE SkyBox
        techniqueId++;
		skyBoxUniformBufferObject sbubo{
		    .mvpMat = ViewPrj * glm::translate(glm::mat4(1), cameraPos) * glm::scale(glm::mat4(1), glm::vec3(100.0f))
        };
		SC.TI[techniqueId].I[0].DS[1][0]->map(currentImage, &sbubo, 0);
		SC.TI[techniqueId].I[0].DS[1][0]->map(currentImage, &debugUbo, 1);

        // TECHNIQUE Terrain
        techniqueId++;
        TerrainFactorsUBO terrainFactorsUbo{};
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            ubos.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            ubos.mvpMat = ViewPrj * ubos.mMat;
            ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));

            terrainFactorsUbo.maskBlendFactor = SC.TI[techniqueId].I[instanceId].factor1;
            terrainFactorsUbo.tilingFactor = SC.TI[techniqueId].I[instanceId].factor2;

            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &gubo, 0); // Set 0
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &ubos, 0);  // Set 1
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &shadowClipUbo, 11);
            SC.TI[techniqueId].I[instanceId].DS[1][2]->map(currentImage, &terrainFactorsUbo, 0);  // Set 2
        }

        // TECHNIQUE Water
        techniqueId++;
        ubos.mMat   = SC.TI[techniqueId].I[0].Wm;
        ubos.mvpMat = ViewPrj * ubos.mMat;
        ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));
        SC.TI[techniqueId].I[0].DS[1][0]->map(currentImage, &timeUbo, 0);
        SC.TI[techniqueId].I[0].DS[1][0]->map(currentImage, &ubos, 1);
        SC.TI[techniqueId].I[0].DS[1][0]->map(currentImage, &shadowClipUbo, 2);
        SC.TI[techniqueId].I[0].DS[1][1]->map(currentImage, &gubo, 0);

        // TECHNIQUE Vegetation/Grass
        techniqueId++;
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            ubos.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            ubos.mvpMat = ViewPrj * ubos.mMat;
            ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));

            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &gubo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &ubos, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &timeUbo, 1);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &shadowClipUbo, 2);
        }

        // TECHNIQUE PBR_SpecGloss
        techniqueId++;
        SgAoMaterialFactorsUBO sgAoUbo{};
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            ubos.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            ubos.mvpMat = ViewPrj * ubos.mMat;
            ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));

            sgAoUbo.diffuseFactor = SC.TI[techniqueId].I[instanceId].diffuseFactor;
            sgAoUbo.specularFactor = SC.TI[techniqueId].I[instanceId].specularFactor;
            sgAoUbo.glossinessFactor = SC.TI[techniqueId].I[instanceId].factor1;
            sgAoUbo.aoFactor = SC.TI[techniqueId].I[instanceId].factor2;

            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &gubo, 0); // Set 0
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &ubos, 0); // Set 1
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &shadowClipUbo, 5); // Set 1
            SC.TI[techniqueId].I[instanceId].DS[1][2]->map(currentImage, &sgAoUbo, 0); // Set 2
        }


        // ---- updates the FPS -----
		static float elapsedT = 0.0f;
		static int countedFrames = 0;
		
		countedFrames++;
		elapsedT += deltaT;
		if(elapsedT > 1.0f) {
			float Fps = (float)countedFrames / elapsedT;
			
			std::ostringstream oss;
			oss << "FPS: " << Fps << "\n";

			txt.print(1.0f, 1.0f, oss.str(), 1, "CO", false, false, true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
			
			elapsedT = 0.0f;
		    countedFrames = 0;
		}
		
		txt.updateCommandBuffer();
    }

	float GameLogic() {
		// Integration with the timers and the controllers
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);
		float MOVE_SPEED = fire ? MOVE_SPEED_RUN : MOVE_SPEED_BASE;

		// Step the physics simulation
		PhysicsMgr.update(deltaT);

		// Get current player position from physics body
		glm::vec3 Pos = PhysicsMgr.getPlayerPosition();

		camDist = (MIN_CAM_DIST + MAX_CAM_DIST) / 2.0f;

		// Update camera rotation
		Yaw = Yaw - ROT_SPEED * deltaT * r.y;
		Pitch = Pitch - ROT_SPEED * deltaT * r.x;
		Pitch = Pitch < minPitch ? minPitch : (Pitch > maxPitch ? maxPitch : Pitch);

		// Calculate movement direction based on camera orientation
		glm::vec3 ux = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(1,0,0,1);
		glm::vec3 uz = glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0)) * glm::vec4(0,0,-1,1);

		// Calculate desired movement vector
		glm::vec3 moveDir = MOVE_SPEED * m.x * ux - MOVE_SPEED * m.z * uz;
        if(FLY_MODE)
            moveDir += MOVE_SPEED * m.y * glm::vec3(0,1,0);

		// Apply movement force to physics body
		PhysicsMgr.movePlayer(moveDir, fire);

		// Camera height adjustment
		camHeight += MOVE_SPEED * 0.1f * (glfwGetKey(window, GLFW_KEY_Q) ? 1.0f : 0.0f) * deltaT;
		camHeight -= MOVE_SPEED * 0.1f * (glfwGetKey(window, GLFW_KEY_E) ? 1.0f : 0.0f) * deltaT;
		camHeight = glm::clamp(camHeight, 0.5f, 3.0f);

		// Exponential smoothing factor for camera damping
		float ef = exp(-10.0 * deltaT);

		// Rotational independence from view with damping
		if(glm::length(glm::vec3(moveDir.x, 0.0f, moveDir.z)) > 0.001f) {
			relDir = Yaw + atan2(moveDir.x, moveDir.z);
			dampedRelDir = dampedRelDir > relDir + 3.1416f ? dampedRelDir - 6.28f :
						   dampedRelDir < relDir - 3.1416f ? dampedRelDir + 6.28f : dampedRelDir;
		}
		dampedRelDir = ef * dampedRelDir + (1.0f - ef) * relDir;

		// Final world matrix computation using physics position
		World = glm::translate(glm::mat4(1), Pos) * glm::rotate(glm::mat4(1.0f), dampedRelDir, glm::vec3(0,1,0));

		// Projection
		glm::mat4 Prj = glm::perspective(FOVy, Ar, worldNearPlane, worldFarPlane);
		Prj[1][1] *= -1;

		// View
		// Target position based on physics body position
		glm::vec3 target = Pos + glm::vec3(0.0f, camHeight, 0.0f);

		// Camera position, depending on Yaw parameter
		glm::mat4 camWorld = glm::translate(glm::mat4(1), Pos) * glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0,1,0));
		cameraPos = camWorld * glm::vec4(0.0f, camHeight + camDist * sin(Pitch), camDist * cos(Pitch), 1.0);

		// Damping of camera
		dampedCamPos = ef * dampedCamPos + (1.0f - ef) * cameraPos;

		glm::mat4 View = glm::lookAt(dampedCamPos, target, glm::vec3(0,1,0));

		ViewPrj = Prj * View;

		return deltaT;
	}
};

// This is the main: probably you do not need to touch this!
int main() {
    CGProject app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}