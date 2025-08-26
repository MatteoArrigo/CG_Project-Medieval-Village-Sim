// This has been adapted from the Vulkan tutorial
#include <sstream>

#include <json.hpp>

#include "AnimatedProps.hpp"
#include "modules/Starter.hpp"
#include "modules/Scene.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Animations.hpp"
#include "character/char_manager.hpp"
#include "character/character.hpp"
#include "PhysicsManager.hpp"
#include "Player.hpp"
#include "Utils.hpp"
#include "sun_light.hpp"
#include "InteractionsManager.hpp"
#include "ViewControls.hpp"

//TODO: pensa se aggiungere la cubemap ambient lighting in tutti i fragment shader, non solo l'acqua
// Nota: nell'acqua usiamo ora la equirectangular map per la reflection, non preprocessata
// Si potrebbe implementare IBL nelle altre tecniche, come PBR+IBL, usando radiance cubemap, quindi preprocessata
// Così come è ora, invece, viene sempre considerata luce bianca come luce ambientale da tutte le direzioni

/** If true, gravity and inertia are disabled
 And vertical movement (along y, thus actual fly) is enabled.
 */
const bool FLY_MODE = false;
const std::string SCENE_FILEPATH = "assets/scene.json";

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
// NOTE: Up to now, the point light calculations are present only in terrain and buidlings pipelines.
// If you want the torches to enlight also other meshes, add those calculations in the corresponding pipelines, too

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

struct PbrMRFactorsUBO {
	alignas(4) float metallicFactor;	// scalar
	alignas(4) float roughnessFactor;	// scalar
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

	// --- VULKAN GRAPHICS OBJECTS ---
    // DSL general
    DescriptorSetLayout DSLlightModel, DSLgeomShadow, DSLgeomShadowTime, DSLgeomShadow4Char;
    // DSL for specific pipelines
	DescriptorSetLayout DSLpbr, DSLcharPbr, DSLpbrShadow, DSLskybox, DSLterrain,  DSLwater, DSLgrass, DSLchar, DSLtorches;
    // DSL for shadow mapping
    DescriptorSetLayout DSLshadowMap, DSLshadowMapChar;
    // VD, RP, Pipelines
	VertexDescriptor VDchar, VDnormUV, VDpos, VDtan;
	RenderPass RPshadow, RP;
	Pipeline Pchar, PcharPbr, Pskybox, Pwater, Pgrass, Pterrain, Pprops, Pbuildings, Ptorches;
    Pipeline PshadowMap, PshadowMapChar, PshadowMapSky, PshadowMapWater;

	// Models, textures and Descriptors (values assigned to the uniforms)
	Scene SC;
	std::vector<VertexDescriptorRef>  VDRs;
	std::vector<TechniqueRef> PRs;

	// to provide textual feedback
	TextMaker txt;

	// Controller classes
	PhysicsManager physicsMgr;
    ViewControls* viewControls;
    SunLightManager sunLightManager;
	CharManager charManager;			// Character manager for animations
	Player * player;						// Player manger
    InteractionsManager interactionsManager;
    InteractableState interactableState;
	AnimatedProps* animatedProps;

	// Other application parameters / objects
	float ar;	// Aspect ratio
    Texture Tvoid;
    LightModelUBO lightUbo;

    /** Debug vector present in DSL for shadow map. Basic version is vec4(0,0,0,0)
     * if debugLightView.x == 1.0, the terrain and buildings render only white if lit and black if in shadow
     * if debugLightView.x == 2.0, the terrain and buildings show only the point lights illumination
     * if debugLightView.y == 1.0, the light's clip space is visualized instead of the basic perspective view
     */
    glm::vec4 debugLightView = glm::vec4(0.0);
	glm::vec4 debug1 = glm::vec4(0);        // TODO: secondo me si può togliere

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
		ar = (float)windowWidth / (float)windowHeight;
	}
	
	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		std::cout << "Window resized to: " << w << " x " << h << "\n";
        ar = (float)w / (float)h;
		// Update Render Pass
		RP.width = w;
		RP.height = h;
        // Note: the shadow render pass has fixed square resolution (4096x4096), doesn't need to be resized

		// updates the textual output
		txt.resizeScreen(w, h);
	}

	void localInit() {
        Tvoid.init(this, "assets/textures/void.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

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
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sunLightManager.getNumLights()}
		});
        DSLchar.init(this, {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
        });
        DSLwater.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(IndexUBO), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,2},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, sunLightManager.getNumLights()}
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
		DSLcharPbr.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PbrMRFactorsUBO), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,1},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1,1},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2,1},
		});
		DSLtorches.init(this, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(IndexUBO),1},
            {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0,1},
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
        // Grey as default clear value
		RP.properties[0].clearValue = {0.5f,0.5f,0.5f,1.0f};
        /* Actual creation of the Render Pass for shadow mapping.
            It is done here to be sure the attachment of RPshadow is created and can be linked as input in RP */
        RPshadow.create();


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
		PcharPbr.init(this, &VDchar, "shaders/CharacterVertex.vert.spv", "shaders/CharacterPBR_MR.frag.spv", {&DSLlightModel, &DSLgeomShadow4Char, &DSLcharPbr});
        Pgrass.init(this, &VDtan, "shaders/GrassShader.vert.spv", "shaders/GrassShader.frag.spv", {&DSLlightModel, &DSLgeomShadowTime, &DSLgrass});
        Pterrain.init(this, &VDtan, "shaders/TerrainShader.vert.spv", "shaders/TerrainShader.frag.spv", {&DSLlightModel, &DSLgeomShadow, &DSLterrain});
		Pbuildings.init(this, &VDtan, "shaders/GeneralPBR.vert.spv", "shaders/BuildingPBR.frag.spv", {&DSLlightModel, &DSLgeomShadow, &DSLpbrShadow});
		Pprops.init(this, &VDtan, "shaders/GeneralPBR.vert.spv", "shaders/PropsPBR.frag.spv", {&DSLlightModel, &DSLgeomShadow, &DSLpbr});
		Ptorches.init(this, &VDtan, "shaders/GeneralPBR.vert.spv", "shaders/TorchPinShader.frag.spv", {&DSLlightModel, &DSLgeomShadowTime, &DSLtorches});

        // --------- TECHNIQUES INITIALIZATION ---------
        std::vector<TextureDefs> skyboxTexs;        // automatic fill-up of textures for skybox
        skyboxTexs.reserve(sunLightManager.getNumLights());
        for (int i = 0; i < sunLightManager.getNumLights(); ++i)
            skyboxTexs.push_back({true, i, VkDescriptorImageInfo{}});
        std::vector<TextureDefs> waterTexs;        // automatic fill-up of textures for water
        waterTexs.reserve(sunLightManager.getNumLights()+2);
        waterTexs.push_back({true, 0, VkDescriptorImageInfo{}});
        waterTexs.push_back({true, 1, VkDescriptorImageInfo{}});
        for(int i = 0; i < sunLightManager.getNumLights(); ++i)
            waterTexs.push_back({true, 2 + i, VkDescriptorImageInfo{}});

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
            }}
        }, 1, &VDchar);
        PRs[1].init("CharPBR", {
            {&PshadowMapChar, {{
                    {true, 0, {} },     // Shadow map UBO
            }} },
            {&PcharPbr, {
                {},
                {},
                {
                    {true,  0, {}},     // diffuse
                    {true,  1, {}},     // normal
                    {true,  2, {}},     // specular
                    }
                }}
        }, 3, &VDchar);
        PRs[2].init("SkyBox", {
            {&PshadowMapSky, {} },
            {&Pskybox, {
                     skyboxTexs
            }}
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
            }}
        }, 9, &VDtan);
        PRs[4].init("Water", {
            {&PshadowMapWater, {} },
            {&Pwater, {
                {},
                {},
                waterTexs
                /* water textures, with expected order: 2 normal maps, 6 reflection maps for each lightColor, in order +x, -x, +y, -y, +z, -z
                 * */
            }}
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
            }}
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
            }}
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
            }}
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
            }}
        }, 1, &VDtan);


        // --------- SCENE INITIALIZATION ---------
		// sets the size of the Descriptor Set Pool --> Overprovisioned!
		DPSZs.uniformBlocksInPool = 1000;
		DPSZs.texturesInPool = 1000;
		DPSZs.setsInPool = 1000;
		
        std::cout << "\nLoading the scene\n\n";
		if(SC.init(this, 2, VDRs, PRs, SCENE_FILEPATH) != 0) {
			std::cout << "ERROR LOADING THE SCENE\n";
			exit(0);
		}

		// Characters and animations initialization
		if (charManager.init(SCENE_FILEPATH, SC) != 0) {
			std::cout << "ERROR LOADING CHARACTERs\n";
			exit(0);
		}

        if (interactionsManager.init(SCENE_FILEPATH) != 0) {
			std::cout << "ERROR LOADING INTERACTION POINTS\n";
			exit(0);
		}
        std::cout << "Scanned " << interactionsManager.getAllInteractions().size() << " interactable points\n";

		// initializes the textual output
		txt.init(this, windowWidth, windowHeight);

		// submits the main command buffer
		submitCommandBuffer("main", 0, populateCommandBufferAccess, this);

		txt.print(1.0f, 1.0f, "FPS:",1,"CO",false,false,true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});

		// Initialize PhysicsManager
		if(!physicsMgr.initialize(FLY_MODE)) {
			exit(0);
		}

		// Add player character to the PhysicsManager
		/*
		 * TODO: PhysicsMgr.addPlayerFromModel() creates a player object based on a model reference. The problem is that, at the current state of
		 * this project, getShapeFromModel() is not able to correctly extract a collision shape from the player model we are using.
		 * That method should be re-implemented to better support different model types.
		 */
		physicsMgr.addCapsulePlayer();

		// Add static meshes to the PhysicsManager for collision detection
		physicsMgr.addStaticMeshes(SC.M, SC.I, SC.InstanceCount);

		// Initializes the player Character reference
		// NOTE: the first character in scene.json is supposed to be the player character
		player = new Player(charManager.getCharacters()[0], &physicsMgr);

        lightUbo.nPointLights = 0;
        for (const auto& interaction : interactionsManager.getAllInteractions()) {
            if (interaction.id.find("torch_fire") != std::string::npos) {
                interactableState.torchesOn.push_back(false);
                if (lightUbo.nPointLights > MAX_POINT_LIGHTS) {
                    std::cout << "ERROR: Too many point lights in the scene.\n";
                    std::exit(-1);  // Stop adding if we exceed the limit
                }
                lightUbo.pointLightPositions[lightUbo.nPointLights] = glm::vec4(interaction.position, 1.0f);
                lightUbo.pointLightColors[lightUbo.nPointLights] = glm::vec4(0,0,0,1);
                lightUbo.nPointLights++;
            }
        }

		// Initialize animated props
		animatedProps = new AnimatedProps(&interactionsManager, &interactableState, &SC);

		// Initialize view controls
        viewControls = new ViewControls(FLY_MODE, window, ar, physicsMgr, sunLightManager);
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// creates the render pass
        std::cout << "Creating pipelines and descriptor sets\n";
        // Render passes already created in localInit()

        // Creation of RPshadow must be done before the pipelines, because they use it
        RP.create();

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

		SC.pipelinesAndDescriptorSetsCleanup();
		txt.pipelinesAndDescriptorSetsCleanup();
	}

	void localCleanup() {
		DSLgeomShadow4Char.cleanup();
		DSLpbr.cleanup();
		DSLcharPbr.cleanup();
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
        
        static bool firstTime = true;

        // Handle of command keys
        interactionsManager.updateNearInteractable(physicsMgr.getPlayerPosition());
		static std::shared_ptr<Character> lastCharInteracted;
		if (firstTime)
			 lastCharInteracted = nullptr;
		glm::vec3 playerPos = physicsMgr.getPlayerPosition();
        {
            handleKeyToggle(window, GLFW_KEY_0, debounce, curDebounce, [&]() {
                sunLightManager.nextLight(interactableState);
            });
            handleKeyToggle(window, GLFW_KEY_1, debounce, curDebounce, [&]() {
                debugLightView.x = static_cast<int>(debugLightView.x + 1) % 3;
            });
            handleKeyToggle(window, GLFW_KEY_2, debounce, curDebounce, [&]() {
				viewControls->nextViewMode();
            });

            static int curAnim = 0;
            static AnimBlender *AB = charManager.getCharacters()[0]->getAnimBlender();
            handleKeyToggle(window, GLFW_KEY_9, debounce, curDebounce, [&]() {
                curAnim = (curAnim + 1) % 5;
                AB->Start(curAnim, 0.5);
                std::cout << "Playing anim: " << curAnim << "\n";
            });

            // Handle the E key for Character interaction
            handleKeyToggle(window, GLFW_KEY_E, debounce, curDebounce, [&]() {
            	auto nearestCharacter = charManager.getNearestCharacter(playerPos);
                if (nearestCharacter) {
                    nearestCharacter->interact();
                	lastCharInteracted = nearestCharacter;
                    txt.print(0.5f, 0.1f, nearestCharacter->getCurrentDialogue(), 3, "CO", false, false, true, TAL_CENTER, TRH_CENTER, TRV_TOP, {1,1,1,1}, {0,0,0,0.5});
                    std::cout << "Character in state : " << nearestCharacter->getCurrentState() << "\n";
                } else {
                    std::cout << "No Character nearby to interact with.\n";
                }
            });

            // Handle the Z key for interaction with interaction point (if any in the nearby)
            handleKeyToggle(window, GLFW_KEY_Z, debounce, curDebounce, [&]() {
                interactionsManager.interact(interactableState);
            });
        }

		// when a player walks away from a character he interacted with, it stops the interaction
		if (lastCharInteracted != nullptr && glm::distance(lastCharInteracted->getPosition(), playerPos) > charManager.getMaxDistance()) {
			lastCharInteracted->setIdle();
			lastCharInteracted = nullptr;
			txt.removeText(3);
		}

		// moves the view
		float deltaT = GameLogic();
		player->handleKeyActions(window, deltaT);

        // ----- UPDATE UNIFORMS -----
        //NOTE on code style: write all uniform variables in the following section
        // and assign the constant values across the different model during initialization

        // Common uniforms and general variables
        int instanceId;
        int techniqueId = 1;  // First 2 techniques are for characters, so start from 1 (so that after ++ is 2)
        lightUbo.lightDir = sunLightManager.getDirection();
        lightUbo.lightColor = sunLightManager.getColor();
        lightUbo.eyePos = viewControls->getCameraPos();
        for(int i=0 ; i<lightUbo.nPointLights; i++) {
            lightUbo.pointLightColors[i] = interactableState.torchesOn[i] ?
                    glm::vec4(10,0,0,1) : glm::vec4(0,0,0,1);
        }
        if(firstTime)
            for (int i = 0; i < lightUbo.nPointLights; ++i) {
                const glm::vec4& pos = lightUbo.pointLightPositions[i];
                std::cout << "PointLight " << i << ": (" << pos.x << ", " << pos.y << ", " << pos.z << ", " << pos.w << ")\n";
            }

        ShadowMapUBO shadowUbo{
            .lightVP = sunLightManager.getLightVP()
        };
        ShadowMapUBOChar shadowMapUboChar{
            .lightVP = sunLightManager.getLightVP()
        };
        ShadowClipUBO shadowClipUbo{
            .lightVP = sunLightManager.getLightVP(),
            .debug = debugLightView
        };
        TimeUBO timeUbo{.time = static_cast<float>(glfwGetTime())};
        GeomUBO geomUbo{};
        GeomCharUBO geomCharUbo{
                .debug1 = debug1
        };
        GeomSkyboxUBO geomSkyboxUbo{
            .skyboxTextureIdx = sunLightManager.getIndex(),
            .debug = debugLightView,
        };
        IndexUBO indexUbo;
        TerrainFactorsUBO terrainFactorsUbo{};
        PbrFactorsUBO pbrUbo{};
		PbrMRFactorsUBO pbrMRUbo{};
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
                if(firstTime) std::cout << "\tInstance: " << *(I->id) << "\n";
				std::string techniqueName = *(I->TIp->T->id);
				if (techniqueName == "CharCookTorrance") {
					// CookTorrance technique ubo update
					for(int im = 0; im < TMsp->size(); im++) {
						geomCharUbo.mMat[im]   = I->Wm * AdaptMat * (*TMsp)[im];
						geomCharUbo.mvpMat[im] = viewControls->getViewPrj() * geomCharUbo.mMat[im];
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
						if(*(I->id) == "player" && viewControls->getViewMode()==ViewMode::FIRST_PERSON)
							// If view mode is "first person", the player is moved underground to make it invisible
							// Note: For it to work, the player must be rendered with PBR with exact id "player"
							geomCharUbo.mMat[im] *= glm::translate(glm::mat4(1), glm::vec3(0.0f, -100.0f, 0.0f));
						geomCharUbo.mvpMat[im] = viewControls->getViewPrj() * geomCharUbo.mMat[im];
						geomCharUbo.nMat[im] = glm::inverse(glm::transpose(geomCharUbo.mMat[im]));
                        shadowMapUboChar.model[im] = geomCharUbo.mMat[im];
					}
					geomCharUbo.jointsCount = TMsp->size();

					pbrMRUbo.metallicFactor = I->factor1;
					pbrMRUbo.roughnessFactor = I->factor2;

                    I->DS[0][0]->map(currentImage, &shadowMapUboChar, 0);
                    I->DS[1][0]->map(currentImage, &lightUbo, 0);
                    I->DS[1][1]->map(currentImage, &geomCharUbo, 0);
                    I->DS[1][1]->map(currentImage, &shadowClipUbo, 1);
					I->DS[1][2]->map(currentImage, &pbrMRUbo, 0); // Set 2
				} else {
					std::cout << "ERROR: Unknown technique for character: " << *(I->TIp->T->id) << "\n";
				}
			}
		}

        techniqueId++;
        if(firstTime) std::cout << "Updating technique " << techniqueId << " UBOs\n";
		geomSkyboxUbo.mvpMat = viewControls->getViewPrj() * glm::translate(glm::mat4(1), viewControls->getCameraPos()) * glm::scale(glm::mat4(1), glm::vec3(100.0f));
		SC.TI[techniqueId].I[0].DS[1][0]->map(currentImage, &geomSkyboxUbo, 0);

        // TECHNIQUE Terrain
        techniqueId++;
        if(firstTime) std::cout << "Updating technique " << techniqueId << " UBOs\n";
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            geomUbo.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            geomUbo.mvpMat = viewControls->getViewPrj() * geomUbo.mMat;
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
        geomUbo.mvpMat = viewControls->getViewPrj() * geomUbo.mMat;
        geomUbo.nMat   = glm::inverse(glm::transpose(geomUbo.mMat));
        indexUbo.idx = sunLightManager.getIndex();
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
            geomUbo.mvpMat = viewControls->getViewPrj() * geomUbo.mMat;
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
            geomUbo.mvpMat = viewControls->getViewPrj() * geomUbo.mMat;
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
            geomUbo.mvpMat = viewControls->getViewPrj() * geomUbo.mMat;
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
            geomUbo.mvpMat = viewControls->getViewPrj() * geomUbo.mMat;
            geomUbo.nMat   = glm::inverse(glm::transpose(geomUbo.mMat));

            shadowUbo.model = SC.TI[techniqueId].I[instanceId].Wm;

            std::string torchId = *SC.TI[techniqueId].I[instanceId].id;
            indexUbo.idx = std::stoi(torchId.substr(torchId.find_last_of('.') + 1)); // expects torch id in form "torch_#"

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &shadowUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][0]->map(currentImage, &lightUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &geomUbo, 0);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &shadowClipUbo, 1);
            SC.TI[techniqueId].I[instanceId].DS[1][1]->map(currentImage, &timeUbo, 2);
            SC.TI[techniqueId].I[instanceId].DS[1][2]->map(currentImage, &indexUbo, 0);
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

		// Print current projection mode
		txt.print(-0.98f, -0.99f, "View: "+viewControls->getViewModeStr(), 5, "SS",
				  false, false, true, TAL_LEFT, TRH_LEFT, TRV_TOP,
				  {1,1,1,1}, {0,0,0,1}, {0.5f, 0.5f, 0.5f, 0.2f}, 1,1);

		// Update message for interaction with idle Characters nearby
		if(charManager.getNearestCharacter(physicsMgr.getPlayerPosition()) != nullptr) {
			auto character = charManager.getNearestCharacter(physicsMgr.getPlayerPosition());
			txt.print(0.5f, 0.1f, "Press E to talk with "+character->getName(), 2, "CO", false, false, true, TAL_CENTER, TRH_CENTER, TRV_TOP, {1,1,1,1}, {0,0,0,0.5});
		}
		else {
			txt.removeText(2);		// remove the text to interact if no idle character is nearby
		}

        // Update message for interaction point in the nearby
        if(interactionsManager.isNearInteractable()) {
            auto interaction = interactionsManager.getNearInteractable();
            txt.print(0.96f, -0.97f, "Press Z to interact\nwith "+interaction.id, 4, "SS", false, false, true,
					  TAL_RIGHT, TRH_RIGHT, TRV_TOP, {1,1,1,1}, {0.5,0,0,1},
					  {0.5,0,0,0.5}, 1.2, 1.2);
        }
		else
			txt.removeText(4);		// remove the text if no interaction point is nearby


        txt.updateCommandBuffer();
        firstTime = false;
    }

	float GameLogic() {
		// Integration with the timers and the controllers
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);

		physicsMgr.update(deltaT);
        viewControls->updateFrame(deltaT, m, r, fire);

		// Move the player in the correct position (physics + model update)
        // Note: + 180 degrees to rotate so that he sees in direction of movement
		player->move(viewControls->getMoveDir(), viewControls->getPlayerYaw() + glm::radians(180.0f));

		// Update animated props
		animatedProps->update(deltaT);

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