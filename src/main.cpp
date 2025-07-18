// This has been adapted from the Vulkan tutorial
#include <sstream>

#include <json.hpp>

#include <PhysicsManager.hpp>
#include "modules/TextMaker.hpp"
#include "modules/Animations.hpp"
#include "character/char_manager.hpp"
#include "character/character.hpp"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

//TODO: effetto bloom, così che le torce illuminano anche la parte vicino a loro!
//TODO: se non cambi TorchPinShader.vert, puoi in realtà riusare quello di Props o buildings!

//TODO: nei frag shader di pbr, si potrebbe cambiare ao contribution in base a se è giorno o notte
//TODO: Per ora point lights illuminano solo terreno e buildings, pensa se aggiungerle a altre pipeline (tipo props)
//TODO: pensa se aggiungere la cubemap ambient lighting in tutti i fragment shader, non solo l'acqua
// Nell'acqua la stiamo usando per la parte speculare
// Nei fragment shader come PBR si potrebbe usare per parte ambient
// Così come è ora, invece, viene sempre considerata luce bianca come luce ambientale da tutte le direzioni

/** If true, gravity and inertia are disabled
 And vertical movement (along y, thus actual fly) is enabled.
 */
const bool FLY_MODE = true;
const std::string SCENE_FILEPATH = "assets/scene_reduced.json";

struct VertexChar {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
    glm::vec4 tan;
	glm::uvec4 jointIndices;
	glm::vec4 weights;
};

struct VertexPos {
	glm::vec3 pos;
};

struct VertexNormUV {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};

struct VertexTan {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
	glm::vec4 tan;
};

#define MAX_POINT_LIGHTS 20
struct LightModelUBO {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;

    // For padding mismatch reasons, positions are in vec4 format, but the last coordinate is not used
    alignas(16) glm::vec4 pointLightPositions[MAX_POINT_LIGHTS];
	alignas(16) glm::vec4 pointLightColors[MAX_POINT_LIGHTS];
    alignas(4) int nPointLights;
};

#define MAX_JOINTS 100
struct GeomCharUBO {
	alignas(16) glm::vec4 debug1;
	alignas(16) glm::mat4 mvpMat[MAX_JOINTS];
	alignas(16) glm::mat4 mMat[MAX_JOINTS];
	alignas(16) glm::mat4 nMat[MAX_JOINTS];
    alignas(4) int jointsCount;
};

struct GeomUBO {
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
	alignas(16) glm::mat4 model[MAX_JOINTS];
};
struct ShadowClipUBO {
	alignas(16) glm::mat4 lightVP;
	alignas(16) glm::vec4 debug;
};

struct GeomSkyboxUBO {
	alignas(16) glm::mat4 mvpMat;
    alignas(4) int skyboxTextureIdx;
    alignas(16) glm::vec4 debug;
};

struct TimeUBO {
    alignas(4) float time; // scalar
};

struct IndexUBO {
    alignas(4) int idx;
};

struct PbrFactorsUBO {
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
	
	// --- DESCRIPTOR SET LAYOUTS ---
    // DSL general
    DescriptorSetLayout DSLlightModel, DSLgeomShadow, DSLgeomShadowTime, DSLgeomShadow4Char;
    // DSL for specific pipelines
	DescriptorSetLayout DSLpbr, DSLpbrShadow, DSLskybox, DSLterrain, DSLwater,
                        DSLgrass, DSLchar, DSLtorches;
    // DSL for shadow mapping
    DescriptorSetLayout DSLshadowMap, DSLshadowMapChar;

    DescriptorSetLayout DSLpostProcess;


	// Vertex formants, Pipelines [Shader couples] and Render passes
	VertexDescriptor VDchar;
	VertexDescriptor VDnormUV;
	VertexDescriptor VDpos;
	VertexDescriptor VDtan;
	RenderPass RPshadow, RP, RPbloom;
	Pipeline Pchar, PcharPbr, Pskybox, Pwater, Pgrass, Pterrain, Pprops, Pbuildings, Ptorches;
    Pipeline PshadowMap, PshadowMapChar, PshadowMapSky, PshadowMapWater;
    Pipeline Pbloom;

    Texture Tvoid;

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

    //TODO: produci le reflection cubemap associate alle skyboxes
    const std::vector<glm::vec4> lightColors{
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),      // full day
        glm::vec4(1.0f, 0.3f, 0.3f, 1.0f),      // sunset
        glm::vec4(0.2f, 0.2f, 0.4f, 1.0f),      // night with light     // TODO: trova una skybox con la luna ben visibile
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),      // night, full dark
    };
    const int nLightColors = static_cast<int>(lightColors.size());
    /**
     * Index of the current light color in scene wrt the array lightColors
     * NOTE: Also rendered skybox should be changed accordingly, and for this
     *   lightColors.size() textures for skybox instance are expected in the json file
     */
    int lightColorIdx = 3;
    /**
     * Matrix defining the light rotation to apply to +z axis to get the light direction.
     * It is used
     *  - applied to +z axis to compute light direction
     *  - applied in its inverse form to compute the light projection matrix for shadow map
     */
    const glm::mat4 lightRotation = glm::rotate(glm::mat4(1), glm::radians(-31.0f),
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
    const float lightWorldLeft = -110.0f, lightWorldRight = 110.0f;
    const float lightWorldBottom = lightWorldLeft * 1.0f+50, lightWorldTop = lightWorldRight * 1.0f-50;     // Now the shadow map is square (2048x2048)
    const float lightWorldNear = -150.0f, lightWorldFar = 200.0f;
    //TODO: capisci se i bounds trovati per ortho vanno sempre bene o devono essere dinamici
    // Tipo se devono variare con la player position
    /**
     * Actual projection matrix used to render the scene from light pov, in shadow mapping render pass
     */
    glm::mat4 lightVP;
    /** Debug vector present in DSL for shadow map. Basic version is vec4(0,0,0,0)
     * if debugLightView.x == 1.0, the terrain and buildings render only white if lit and black if in shadow
     * if debugLightView.x == 2.0, the terrain and buildings show only the point lights illumination
     * if debugLightView.y == 1.0, the light's clip space is visualized instead of the basic perspective view
     */
    glm::vec4 debugLightView = glm::vec4(0.0);

	glm::vec4 debug1 = glm::vec4(0);

	// To manage NPSs
	CharManager charManager;

    // Here you set the main application parameters
	void setWindowParameters() {
		// window size, title
//		windowWidth = 1920;
//		windowHeight = 1080;
		windowWidth = 1500;
		windowHeight = 1000;
		windowTitle = "CGProject - Medieval Village Sim";
    	windowResizable = GLFW_FALSE;

		// Initial aspect ratio
		Ar = (float)windowWidth / (float)windowHeight;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		Ar = (float)w / (float)h;
		// Update Render Pass
		RP.width = w;
		RP.height = h;
        // Note: the shadow render pass has fixed square resolution (4096x4096), doesn't need to be resized

		// updates the textual output
		txt.resizeScreen(w, h);
	}

	void localInit() {
        Tvoid.init(this, "assets/textures/void.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

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
        lightVP = lightProj * glm::inverse(lightRotation); // inverse because we need to invert the rotation of the world scene to get the light's view

		// --------- DSL INITIALIZATION ---------
		DSLshadowMap.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowMapUBO), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
        });
        DSLshadowMapChar.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowMapUBOChar), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
        });
		DSLlightModel.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(LightModelUBO), 1}
        });
		DSLgeomShadow.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(GeomUBO),       1},
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowClipUBO), 1},
		});
		DSLgeomShadowTime.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(GeomUBO),       1},
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowClipUBO), 1},
			{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(TimeUBO),       1},
		});
		DSLgeomShadow4Char.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(GeomCharUBO),   1},
            {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShadowClipUBO), 1},
        });
		DSLskybox.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(GeomSkyboxUBO), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, nLightColors}
		});
        DSLchar.init(this, {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
        });
        DSLwater.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(IndexUBO), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,2},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2,                 nLightColors},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2+1*nLightColors,  nLightColors},
			{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2+2*nLightColors,  nLightColors},
			{5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2+3*nLightColors,  nLightColors},
			{6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2+4*nLightColors,  nLightColors},
			{7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2+5*nLightColors,  nLightColors}
		});
        DSLgrass.init(this, {
			{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1}
		});
        DSLterrain.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(TerrainFactorsUBO), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
            {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1},
            {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1},
            {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1},
            {6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1},
            {7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6, 1},
            {8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7, 1},
            {9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 8, 1},
            {10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 9, 1},
        });
		DSLpbrShadow.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PbrFactorsUBO), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
            {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1},
            {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1},
            {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1}
        });
		DSLpbr.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PbrFactorsUBO), 1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,1},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1,1},
            {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2,1},
            {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3,1},
        });
		DSLtorches.init(this, {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,1},
        });
		DSLpostProcess.init(this, {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,1},
        });


        // --------- VERTEX DESCRIPTORS INITIALIZATION ---------
        VDchar.init(this, {
                {0, sizeof(VertexChar), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(VertexChar, pos),            sizeof(glm::vec3),  POSITION},
                {0, 1, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(VertexChar, norm),           sizeof(glm::vec3),  NORMAL},
                {0, 2, VK_FORMAT_R32G32_SFLOAT,       offsetof(VertexChar, UV),             sizeof(glm::vec2),  UV},
                {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexChar, tan),            sizeof(glm::vec4),  TANGENT},
                {0, 4, VK_FORMAT_R32G32B32A32_UINT,   offsetof(VertexChar, jointIndices),   sizeof(glm::uvec4), JOINTINDEX},
                {0, 5, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexChar, weights),        sizeof(glm::vec4),  JOINTWEIGHT}
        });
        VDpos.init(this, {
                {0, sizeof(VertexPos), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPos, pos), sizeof(glm::vec3), POSITION}
        });
        VDnormUV.init(this, {
                {0, sizeof(VertexNormUV), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexNormUV, pos),  sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexNormUV, norm), sizeof(glm::vec3), NORMAL},
                {0, 2, VK_FORMAT_R32G32_SFLOAT,    offsetof(VertexNormUV, UV),   sizeof(glm::vec2), UV}
        });
        VDtan.init(this, {
                {0, sizeof(VertexTan), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT,      offsetof(VertexTan, pos),   sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32B32_SFLOAT,      offsetof(VertexTan, norm),  sizeof(glm::vec3), NORMAL},
                {0, 2, VK_FORMAT_R32G32_SFLOAT,         offsetof(VertexTan, UV),    sizeof(glm::vec2), UV},
                {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT,   offsetof(VertexTan, tan),   sizeof(glm::vec4), TANGENT}
        });

		VDRs.resize(4);
		VDRs[0].init("VDchar",  &VDchar);
		VDRs[1].init("VDnormUV",&VDnormUV);
		VDRs[2].init("VDpos",   &VDpos);
		VDRs[3].init("VDtan",   &VDtan);

        // --------- RENDER PASSES INITIALIZATION ---------
        /* RP 1: used for shadow map, rendered from light point of view
         * The options set, different by default ones, are for writing a depth buffer instead of color
         * All related options are set in the RenderPass::getStandardAttchmentsProperties specifing AT_DEPTH_ONLY
         *      (e.g. depth write enabled, color write disabled, initial clear value in stencil of 1.0, ...)
         * Since count=-1, the swapChain size is set as for main RP, updating shadows at each frame */
        RPshadow.init(this, 4096, 4096,-1,
                      RenderPass::getStandardAttchmentsProperties(StockAttchmentsConfiguration::AT_DEPTH_ONLY, this),
                      RenderPass::getStandardDependencies(StockAttchmentsDependencies::ATDEP_DEPTH_TRANS), true);
		/* RP 2: used for the main rendering
		 * Now default options of starter.hpp are used, so it will write color and depth  */
        RP.init(this);
		RP.properties[0].clearValue = {0.0f,0.9f,1.0f,1.0f};
        /* Actual creation of the Render Pass for shadow mapping.
            It is done here to be sure the attachment of RPshadow is created and can be linked as input in RP */

        RPbloom.init(this, -1, -1, -1,
                     RenderPass::getStandardAttchmentsProperties(StockAttchmentsConfiguration::AT_BLOOM_POST_PROCESS, this),
                     RenderPass::getStandardDependencies(StockAttchmentsDependencies::ATDEP_COLOR_RAW), true);

        RPshadow.create();
        RP.create();
        RPbloom.create();


        PshadowMap.init(this, &VDtan, "shaders/shadowMapShader.vert.spv", "shaders/shadowMapShader.frag.spv", {&DSLshadowMap});
        PshadowMap.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);  // or VK_COMPARE_OP_LESS
        PshadowMap.setCullMode(VK_CULL_MODE_BACK_BIT);
        PshadowMap.setPolygonMode(VK_POLYGON_MODE_FILL);

        PshadowMapChar.init(this, &VDchar, "shaders/shadowMapShaderChar.vert.spv", "shaders/shadowMapShader.frag.spv", {&DSLshadowMapChar});
        PshadowMapChar.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);  // or VK_COMPARE_OP_LESS
        PshadowMapChar.setCullMode(VK_CULL_MODE_BACK_BIT);
        PshadowMapChar.setPolygonMode(VK_POLYGON_MODE_FILL);

        PshadowMapSky.init(this, &VDpos, "shaders/shadowMapShaderSky.vert.spv", "shaders/shadowMapShaderEmpty.frag.spv", {});
        PshadowMapWater.init(this, &VDnormUV, "shaders/shadowMapShaderWater.vert.spv", "shaders/shadowMapShaderEmpty.frag.spv", {});

		Pskybox.init(this, &VDpos, "shaders/SkyBoxShader.vert.spv", "shaders/SkyBoxShader.frag.spv", {&DSLskybox});
		Pskybox.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		Pskybox.setCullMode(VK_CULL_MODE_BACK_BIT);
		Pskybox.setPolygonMode(VK_POLYGON_MODE_FILL);

        Pwater.init(this, &VDnormUV, "shaders/WaterShader.vert.spv", "shaders/WaterShader.frag.spv", {&DSLlightModel, &DSLgeomShadowTime, &DSLwater});
        Pwater.setTransparency(true);

		Pchar.init(this, &VDchar, "shaders/CharacterVertex.vert.spv", "shaders/CharacterCookTorrance.frag.spv", {&DSLlightModel, &DSLgeomShadow4Char, &DSLchar});
		PcharPbr.init(this, &VDchar, "shaders/CharacterVertex.vert.spv", "shaders/CharacterPBR.frag.spv", {&DSLlightModel, &DSLgeomShadow4Char, &DSLpbr});
        Pgrass.init(this, &VDtan, "shaders/GrassShader.vert.spv", "shaders/GrassShader.frag.spv", {&DSLlightModel, &DSLgeomShadowTime, &DSLgrass});
        Pterrain.init(this, &VDtan, "shaders/TerrainShader.vert.spv", "shaders/TerrainShader.frag.spv", {&DSLlightModel, &DSLgeomShadow, &DSLterrain});
		Pbuildings.init(this, &VDtan, "shaders/BuildingPBR.vert.spv", "shaders/BuildingPBR.frag.spv", {&DSLlightModel, &DSLgeomShadow, &DSLpbrShadow});
		Pprops.init(this, &VDtan, "shaders/PropsPBR.vert.spv", "shaders/PropsPBR.frag.spv", {&DSLlightModel, &DSLgeomShadow, &DSLpbr});
		Ptorches.init(this, &VDtan, "shaders/TorchPinShader.vert.spv", "shaders/TorchPinShader.frag.spv", {&DSLlightModel, &DSLgeomShadowTime, &DSLtorches});

        Pbloom.init(this, &VDchar, "shaders/BloomShader.vert.spv", "shaders/BloomShader.frag.spv", {&DSLpostProcess});

        // --------- TECHNIQUES INITIALIZATION ---------
        std::vector<TextureDefs> skyboxTexs;        // automatic fill-up of textures for skybox
        skyboxTexs.reserve(nLightColors);
        for (int i = 0; i < nLightColors; ++i)
            skyboxTexs.push_back({true, i, VkDescriptorImageInfo{}});
        std::vector<TextureDefs> waterTexs;        // automatic fill-up of textures for water
        waterTexs.reserve(nLightColors*6+2);
        waterTexs.push_back({true, 0, VkDescriptorImageInfo{}});
        waterTexs.push_back({true, 1, VkDescriptorImageInfo{}});
        for(int j = 0; j < 6; ++j)
            for(int i = 0; i < nLightColors; ++i)
                waterTexs.push_back({true, 2 + 6 * i + j, VkDescriptorImageInfo{}});

        PRs.resize(9);
		PRs[0].init("CharCookTorrance", {
            {&PshadowMapChar, {{
                {true, 0, {} },     // Shadow map UBO
            }} },
            {&Pchar, {
                {},
                {},
                {
                    {true,  0, {} }
                }
            }},
            {&Pbloom, {{
              {false, -1, Tvoid.getViewAndSampler()}
            }} }
        }, 1, &VDchar);
        PRs[1].init("CharPBR", {
            {&PshadowMapChar, {{
                    {true, 0, {} },     // Shadow map UBO
            }} },
            {&PcharPbr, {
                {},
                {},
                {
                    {true,  0, {}},     // albedo
                    {true,  1, {}},     // normal
                    {true,  2, {}},     // specular / glossiness
                    {true,  3, {}}     // ambient occlusion
                }
            }},
            {&Pbloom, {{
               {false, -1, Tvoid.getViewAndSampler()}
            }} }
        }, 4, &VDchar);
        PRs[2].init("SkyBox", {
            {&PshadowMapSky, {} },
            {&Pskybox, {
                     skyboxTexs
            }},
            {&Pbloom, {{
               {false, -1, Tvoid.getViewAndSampler()}
            }} }
        }, static_cast<int>(skyboxTexs.size()), &VDpos);
        PRs[3].init("Terrain", {
            {&PshadowMap, {{
                {true, 1, {} },     // Shadow map UBO
            }} },
            {&Pterrain,   {
                {},
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
                }
            }},
            {&Pbloom, {{
               {false, -1, Tvoid.getViewAndSampler()}
            }} }
        }, 9, &VDtan);
        PRs[4].init("Water", {
            {&PshadowMapWater, {} },
            {&Pwater, {
                {},
                {},
                waterTexs
                /* water textures, with expected order: 2 normal maps, 6 reflection maps for each lightColor,
                 *      in order +x, -x, +y, -y, +z, -z */
            }},
            {&Pbloom, {{
               {false, -1, Tvoid.getViewAndSampler()}
            }} }
        }, static_cast<int>(waterTexs.size()), &VDnormUV);
        PRs[5].init("Grass", {
            {&PshadowMap, {{
                {true, 0, {} },     // Shadow map UBO
            }} },
            {&Pgrass,     {
                {},
                {},
                {
                    {true, 0, {}},
                    {true, 1, {}}
                }
            }},
            {&Pbloom, {{
               {false, -1, Tvoid.getViewAndSampler()}
            }} }
        }, 2, &VDtan);
        PRs[6].init("Buildings", {
            {&PshadowMap, {{
                {true, 0, {} },     // Shadow map UBO
            }} },
            {&Pbuildings, {
                {},
                {},
                {
                    {true,  0, {}},     // albedo
                    {true,  1, {}},     // normal
                    {true,  2, {}},     // specular / glossiness
                    {true,  3, {}},     // ambient occlusion
                    {false,  4, RPshadow.attachments[0].getViewAndSampler() }
                }
            }},
            {&Pbloom, {{
               {false, -1, Tvoid.getViewAndSampler()}
            }} }
        }, 4, &VDtan);
        PRs[7].init("Props", {
            {&PshadowMap, {{
                {true, 0, {} },     // Shadow map UBO
            }} },
            {&Pprops, {
                {},
                {},
                {
                    {true,  0, {}},     // albedo
                    {true,  1, {}},     // normal
                    {true,  2, {}},     // specular / glossiness
                    {true,  3, {}}     // ambient occlusion
                }
            }},
            {&Pbloom, {{
               {false, -1, Tvoid.getViewAndSampler()}
            }} }
        }, 4, &VDtan);
        PRs[8].init("Torches", {
            {&PshadowMap, {{
                {false, 0, Tvoid.getViewAndSampler() },     // Shadow map UBO
            }} },
            {&Ptorches, {
                {},
                {},
                {
                        {true, 0, {}}
                }
            }},
            {&Pbloom, {{
               {false, -1, Tvoid.getViewAndSampler()}
            }} }
        }, 1, &VDtan);


        // --------- SCENE INITIALIZATION ---------
		// sets the size of the Descriptor Set Pool --> Overprovisioned!
		DPSZs.uniformBlocksInPool = 1000;
		DPSZs.texturesInPool = 1000;
		DPSZs.setsInPool = 1000;
		
        std::cout << "\nLoading the scene\n\n";
		if(SC.init(this, 3, VDRs, PRs, SCENE_FILEPATH) != 0) {
			std::cout << "ERROR LOADING THE SCENE\n";
			exit(0);
		}

		// Characters and animations initialization
		if (charManager.init(SCENE_FILEPATH, SC) != 0) {
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

        // Creation of RPshadow must be done before the pipelines, because they use it

        std::cout << "Creating pipelines\n";
        std::cout << "\t1: Creating Pchar\n";
        Pchar.create(&RP);
        std::cout << "\t2: Creating PcharPbr\n";
        PcharPbr.create(&RP);
        std::cout << "\t3: Creating Pskybox\n";
        Pskybox.create(&RP);
        std::cout << "\t4: Creating Pwater\n";
        Pwater.create(&RP);
        std::cout << "\t5: Creating Pgrass\n";
        Pgrass.create(&RP);
        std::cout << "\t6: Creating Pterrain\n";
        Pterrain.create(&RP);
        std::cout << "\t7: Creating Pprops\n";
        Pprops.create(&RP);
        std::cout << "\t8: Creating Ptorches\n";
        Ptorches.create(&RP);
        std::cout << "\t9: Creating Pbuildings\n";
        Pbuildings.create(&RP);
        std::cout << "\t10: Creating PshadowMap\n";
        PshadowMap.create(&RPshadow);
        std::cout << "\t11: Creating PshadowMapChar\n";
        PshadowMapChar.create(&RPshadow);
        std::cout << "\t12: Creating PshadowMapSky\n";
        PshadowMapSky.create(&RPshadow);
        std::cout << "\t13: Creating PshadowMapWater\n";
        PshadowMapWater.create(&RPshadow);


        Pbloom.create(&RPbloom, false);


        std::cout << "Creating descriptor sets\n";
		SC.pipelinesAndDescriptorSetsInit();
		txt.pipelinesAndDescriptorSetsInit();

        std::cout << "pipelinesAndDescriptorSetsInit done\n";
    }

	void pipelinesAndDescriptorSetsCleanup() {
		Pchar.cleanup();
        PcharPbr.cleanup();
		Pskybox.cleanup();
        Pwater.cleanup();
        Pgrass.cleanup();
		Pterrain.cleanup();
        Pprops.cleanup();
        Ptorches.cleanup();
        Pbuildings.cleanup();
        PshadowMap.cleanup();
        PshadowMapChar.cleanup();
        PshadowMapSky.cleanup();
        PshadowMapWater.cleanup();
		RPshadow.cleanup();
        RP.cleanup();

        RPbloom.cleanup();
        Pbloom.cleanup();

		SC.pipelinesAndDescriptorSetsCleanup();
		txt.pipelinesAndDescriptorSetsCleanup();
	}

	void localCleanup() {
		DSLgeomShadow4Char.cleanup();
		DSLpbr.cleanup();
        DSLchar.cleanup();
        DSLpbrShadow.cleanup();
		DSLskybox.cleanup();
        DSLgeomShadowTime.cleanup();
        DSLgeomShadow.cleanup();
        DSLwater.cleanup();
        DSLgrass.cleanup();
		DSLterrain.cleanup();
		DSLlightModel.cleanup();
        DSLshadowMap.cleanup();
        DSLshadowMapChar.cleanup();
        DSLpostProcess.cleanup();

		Pchar.destroy();
        PcharPbr.destroy();
		Pskybox.destroy();
        Pwater.destroy();
        Pgrass.destroy();
        Pprops.destroy();
        Ptorches.destroy();
        Pbuildings.destroy();
		Pterrain.destroy();
        PshadowMap.destroy();
        PshadowMapChar.destroy();
        PshadowMapSky.destroy();
        PshadowMapWater.destroy();

        RPshadow.destroy();
		RP.destroy();

        RPbloom.destroy();
        Pbloom.destroy();

		SC.localCleanup();	
		txt.localCleanup();

        Tvoid.cleanup();
		charManager.cleanup();
	}

	static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params) {
		// Simple trick to avoid having always 'T->' in the code that populates the command buffer!
        std::cout << "Populating command buffer for " << currentImage << "\n";
		CGProject *T = (CGProject *)Params;
		T->populateCommandBuffer(commandBuffer, currentImage);
	}
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        //NOTE: shadow render pass has equal swap chain size of main pass, hence the same currentImage

        std::cout << "\tSHADOW RENDER PASS" << "\n";

        RPshadow.begin(commandBuffer, currentImage);
        SC.populateCommandBuffer(commandBuffer, 0, currentImage);
        RPshadow.end(commandBuffer);

        std::cout << "\tMAIN RENDER PASS" << "\n";

        RP.begin(commandBuffer, currentImage);
        SC.populateCommandBuffer(commandBuffer, 1, currentImage);
        RP.end(commandBuffer);

        std::cout << "\tPOST-PROCESS RENDER PASS" << "\n";

        RPbloom.begin(commandBuffer, currentImage);
        std::cout << "\tPOST-PROCESS RENDER PASS" << "\n";
        SC.populateCommandBuffer(commandBuffer, 2, currentImage);
        std::cout << "\tPOST-PROCESS RENDER PASS" << "\n";
        RPbloom.end(commandBuffer);
        std::cout << "\tPOST-PROCESS RENDER PASS" << "\n";
    }

	void updateUniformBuffer(uint32_t currentImage) {
		static bool debounce = false;
		static int curDebounce = 0;
        
        static bool firstTime = true;

        // TODO Detect running by pressing SHIFT key (currently hardcoded as walking all the time)

        // Handle of command keys
        {
            handleKeyToggle(window, GLFW_KEY_0, debounce, curDebounce, [&]() {
                lightColorIdx = (lightColorIdx + 1) % nLightColors;
            });
            handleKeyToggle(window, GLFW_KEY_1, debounce, curDebounce, [&]() {
                debugLightView.x = static_cast<int>(debugLightView.x + 1) % 3;
            });
            handleKeyToggle(window, GLFW_KEY_2, debounce, curDebounce, [&]() {
                debugLightView.y = 1.0 - debugLightView.y;
            });
            handleKeyToggle(window, GLFW_KEY_SPACE, debounce, curDebounce, [&]() {
                PhysicsMgr.jumpPlayer();
            });

            static int curAnim = 0;
            static AnimBlender *AB = charManager.getCharacters()[0]->getAnimBlender();
            handleKeyToggle(window, GLFW_KEY_O, debounce, curDebounce, [&]() {
                curAnim = (curAnim + 1) % 5;
                AB->Start(curAnim, 0.5);
                std::cout << "Playing anim: " << curAnim << "\n";
            });

            // Handle the E key for Character interaction
            handleKeyToggle(window, GLFW_KEY_E, debounce, curDebounce, [&]() {
                glm::vec3 playerPos = cameraPos; // TODO: replace with actual player position when implemented
                auto nearest = charManager.getNearestCharacter(playerPos);
                if (nearest) {
                    nearest->interact();
                    txt.print(0.5f, 0.1f, nearest->getCurrentDialogue(), 1, "CO", false, false, true, TAL_CENTER, TRH_CENTER, TRV_TOP, {1,1,1,1}, {0,0,0,0.5});
                    std::cout << "Character in state : " << nearest->getState() << "\n";
                } else {
                    std::cout << "No Character nearby to interact with.\n";
                }
            });
        }

		// moves the view
		float deltaT = GameLogic();

        // ----- UPDATE UNIFORMS -----
        //NOTE on code style: write all uniform variables in the following section
        // and assign the constant values across the different model during initialization

        // Common uniforms and general variables
        int instanceId;
        int techniqueId = 1;  // First 2 techniques are for characters, so start from 1 (so that after ++ is 2)
		LightModelUBO lightUbo{
            .lightDir = lightDir,
            .lightColor = lightColors[lightColorIdx],
            .eyePos = cameraPos,
            .nPointLights = 0,
        };
        for (const auto& kv : SC.placeholderPos) {
            if (kv.first.find("torch_fire") != std::string::npos) {
                if (lightUbo.nPointLights > MAX_POINT_LIGHTS) {
                    std::cout << "ERROR: Too many point lights in the scene.\n";
                    std::exit(-1);  // Stop adding if we exceed the limit
                }
                lightUbo.pointLightPositions[lightUbo.nPointLights] = glm::vec4(kv.second, 1.0f);
                lightUbo.pointLightColors[lightUbo.nPointLights] = glm::vec4(10,0,0,1);
                lightUbo.nPointLights++;
            }
        }

        if(firstTime)
            for (int i = 0; i < lightUbo.nPointLights; ++i) {
                const glm::vec4& pos = lightUbo.pointLightPositions[i];
                std::cout << "PointLight " << i << ": (" << pos.x << ", " << pos.y << ", " << pos.z << ", " << pos.w << ")\n";
            }

        ShadowMapUBO shadowUbo{
            .lightVP = lightVP
        };
        ShadowMapUBOChar shadowMapUboChar{
            .lightVP = lightVP
        };
        ShadowClipUBO shadowClipUbo{
            .lightVP = lightVP,
            .debug = debugLightView
        };
        TimeUBO timeUbo{.time = static_cast<float>(glfwGetTime())};
        GeomUBO geomUbo{};
        GeomCharUBO geomCharUbo{
                .debug1 = debug1
        };
        GeomSkyboxUBO geomSkyboxUbo{
            .skyboxTextureIdx = lightColorIdx,
            .debug = debugLightView,
        };
        IndexUBO indexUbo{lightColorIdx};
        TerrainFactorsUBO terrainFactorsUbo{};
        PbrFactorsUBO pbrUbo{};
		glm::mat4 AdaptMat =
			glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)) *
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f,0.0f,0.0f));

		// TECHNIQUE Character
        const float SpeedUpAnimFact = 0.85f;
        for (std::shared_ptr<Character> C : charManager.getCharacters()) {
            if(firstTime) std::cout << "Updating character: " << C->getName() << "\n";

			SkeletalAnimation* SKA = C->getSkeletalAnimation();
			AnimBlender* AB = C->getAnimBlender();
			// updated the animation
			AB->Advance(deltaT * SpeedUpAnimFact);

			// Skeletal Sampling
			SKA->Sample(*AB);
			std::vector<glm::mat4> *TMsp = SKA->getTransformMatrices();
			for (Instance* I : C->getInstances()) {
				std::string techniqueName = *(I->TIp->T->id);
				if (techniqueName == "CharCookTorrance") {
					// CookTorrance technique ubo update
					for(int im = 0; im < TMsp->size(); im++) {
						geomCharUbo.mMat[im]   = I->Wm * AdaptMat * (*TMsp)[im];
						geomCharUbo.mvpMat[im] = ViewPrj * geomCharUbo.mMat[im];
						geomCharUbo.nMat[im] = glm::inverse(glm::transpose(geomCharUbo.mMat[im]));
                        shadowMapUboChar.model[im] = geomCharUbo.mMat[im];
					}
					geomCharUbo.jointsCount = TMsp->size();

                    I->DS[0][0]->map(currentImage, &shadowMapUboChar, 0);
                    I->DS[1][0]->map(currentImage, &lightUbo, 0);
                    I->DS[1][1]->map(currentImage, &geomCharUbo, 0);
                    I->DS[1][1]->map(currentImage, &shadowClipUbo, 1);
				} else if(techniqueName == "CharPBR") {
					for(int im = 0; im < TMsp->size(); im++) {
						geomCharUbo.mMat[im]   = I->Wm * AdaptMat * (*TMsp)[im];
						geomCharUbo.mvpMat[im] = ViewPrj * geomCharUbo.mMat[im];
						geomCharUbo.nMat[im] = glm::inverse(glm::transpose(geomCharUbo.mMat[im]));
                        shadowMapUboChar.model[im] = geomCharUbo.mMat[im];
					}
					geomCharUbo.jointsCount = TMsp->size();

					pbrUbo.diffuseFactor = I->diffuseFactor;
					pbrUbo.specularFactor = I->specularFactor;
					pbrUbo.glossinessFactor = I->factor1;
					pbrUbo.aoFactor = I->factor2;

                    I->DS[0][0]->map(currentImage, &shadowMapUboChar, 0);
                    I->DS[1][0]->map(currentImage, &lightUbo, 0);
                    I->DS[1][1]->map(currentImage, &geomCharUbo, 0);
                    I->DS[1][1]->map(currentImage, &shadowClipUbo, 1);
					I->DS[1][2]->map(currentImage, &pbrUbo, 0); // Set 2
				} else {
					std::cout << "ERROR: Unknown technique for character: " << *(I->TIp->T->id) << "\n";
				}
			}
		}

        techniqueId++;
        if(firstTime) std::cout << "Updating technique " << techniqueId << " UBOs\n";
		geomSkyboxUbo.mvpMat = ViewPrj * glm::translate(glm::mat4(1), cameraPos) * glm::scale(glm::mat4(1), glm::vec3(100.0f));
		SC.TI[techniqueId].I[0].DS[1][0]->map(currentImage, &geomSkyboxUbo, 0);

        // TECHNIQUE Terrain
        techniqueId++;
        if(firstTime) std::cout << "Updating technique " << techniqueId << " UBOs\n";
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            geomUbo.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            geomUbo.mvpMat = ViewPrj * geomUbo.mMat;
            geomUbo.nMat   = glm::inverse(glm::transpose(geomUbo.mMat));

            terrainFactorsUbo.maskBlendFactor = SC.TI[techniqueId].I[instanceId].factor1;
            terrainFactorsUbo.tilingFactor = SC.TI[techniqueId].I[instanceId].factor2;

            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &lightUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &geomUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &shadowClipUbo, 1);
            SC.TI[techniqueId].I[instanceId].DS[1][2]->map(currentImage, &terrainFactorsUbo, 0);
        }

        // TECHNIQUE Water
        techniqueId++;
        if(firstTime) std::cout << "Updating technique " << techniqueId << " UBOs\n";
        geomUbo.mMat   = SC.TI[techniqueId].I[0].Wm;
        geomUbo.mvpMat = ViewPrj * geomUbo.mMat;
        geomUbo.nMat   = glm::inverse(glm::transpose(geomUbo.mMat));
        SC.TI[techniqueId].I[0].DS[1][0]->map(currentImage, &lightUbo, 0);
        SC.TI[techniqueId].I[0].DS[1][1]->map(currentImage, &geomUbo, 0);
        SC.TI[techniqueId].I[0].DS[1][1]->map(currentImage, &shadowClipUbo, 1);
        SC.TI[techniqueId].I[0].DS[1][1]->map(currentImage, &timeUbo, 2);
        SC.TI[techniqueId].I[0].DS[1][2]->map(currentImage, &indexUbo, 0);

        // TECHNIQUE Vegetation/Grass
        techniqueId++;
        if(firstTime) std::cout << "Updating technique " << techniqueId << " UBOs\n";
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            geomUbo.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            geomUbo.mvpMat = ViewPrj * geomUbo.mMat;
            geomUbo.nMat   = glm::inverse(glm::transpose(geomUbo.mMat));

            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &lightUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &geomUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &shadowClipUbo, 1);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &timeUbo, 2);
        }

        // TECHNIQUE Buildings (PBR)
        techniqueId++;
        if(firstTime) std::cout << "Updating technique " << techniqueId << " UBOs\n";
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            geomUbo.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            geomUbo.mvpMat = ViewPrj * geomUbo.mMat;
            geomUbo.nMat   = glm::inverse(glm::transpose(geomUbo.mMat));

            pbrUbo.diffuseFactor = SC.TI[techniqueId].I[instanceId].diffuseFactor;
            pbrUbo.specularFactor = SC.TI[techniqueId].I[instanceId].specularFactor;
            pbrUbo.glossinessFactor = SC.TI[techniqueId].I[instanceId].factor1;
            pbrUbo.aoFactor = SC.TI[techniqueId].I[instanceId].factor2;

            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &lightUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &geomUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &shadowClipUbo, 1);
            SC.TI[techniqueId].I[instanceId].DS[1][2]->map(currentImage, &pbrUbo, 0);
        }

        // TECHNIQUE Props (PBR)
        techniqueId++;
        if(firstTime) std::cout << "Updating technique " << techniqueId << " UBOs\n";
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            geomUbo.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            geomUbo.mvpMat = ViewPrj * geomUbo.mMat;
            geomUbo.nMat   = glm::inverse(glm::transpose(geomUbo.mMat));

            pbrUbo.diffuseFactor = SC.TI[techniqueId].I[instanceId].diffuseFactor;
            pbrUbo.specularFactor = SC.TI[techniqueId].I[instanceId].specularFactor;
            pbrUbo.glossinessFactor = SC.TI[techniqueId].I[instanceId].factor1;
            pbrUbo.aoFactor = SC.TI[techniqueId].I[instanceId].factor2;

            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &lightUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &geomUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &shadowClipUbo, 1);
            SC.TI[techniqueId].I[instanceId].DS[1][2]->map(currentImage, &pbrUbo, 0);
        }

        // TECHNIQUE Torch Pin
        techniqueId++;
        if(firstTime) std::cout << "Updating technique " << techniqueId << " UBOs\n";
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            geomUbo.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            geomUbo.mvpMat = ViewPrj * geomUbo.mMat;
            geomUbo.nMat   = glm::inverse(glm::transpose(geomUbo.mMat));

            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &lightUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &geomUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &shadowClipUbo, 1);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &timeUbo, 2);
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
        firstTime = false;
    }


/**
 * Handles key toggle events with debounce logic.
 *
 * @param window       Pointer to the GLFW window.
 * @param key          The GLFW key code to check.
 * @param debounce     Reference to a debounce flag to prevent repeated triggers.
 * @param curDebounce  Reference to the currently debounced key.
 * @param action       Function to execute when the key is toggled.
 *
 * When the specified key is pressed, the action is executed only once until the key is released.
 * This prevents multiple triggers from a single key press.
 */
void handleKeyToggle(GLFWwindow* window, int key, bool& debounce, int& curDebounce, const std::function<void()>& action) {
    if (glfwGetKey(window, key)) {
        if (!debounce) {
            debounce = true;
            curDebounce = key;
            action();  // Execute the custom logic
        }
    } else if (curDebounce == key && debounce) {
        debounce = false;
        curDebounce = 0;
    }
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