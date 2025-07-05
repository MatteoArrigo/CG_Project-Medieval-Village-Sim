// This has been adapted from the Vulkan tutorial
#include <sstream>

#include <json.hpp>

#include "PhysicsManager.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Animations.hpp"

// If true, gravity and inertia are disabled
// And vertical movement (along y) is enabled.
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

struct skyBoxUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
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

	// Vertex formants, Pipelines [Shader couples] and Render passes
	VertexDescriptor VDchar;
	VertexDescriptor VDsimp;
	VertexDescriptor VDskyBox;
	VertexDescriptor VDtan;
	RenderPass RP;
	Pipeline Pchar, PsimpObj, PskyBox, Pterrain, P_PBR_SpecGloss, PWater, Pgrass;
	//*DBG*/Pipeline PDebug;

	// Models, textures and Descriptors (values assigned to the uniforms)
	Scene SC;
	std::vector<VertexDescriptorRef>  VDRs;
	std::vector<TechniqueRef> PRs;
	//*DBG*/Model MS;
	//*DBG*/DescriptorSet SSD;

	// PhysicsManager for collision detection
	PhysicsManager PhysicsMgr;

	// To support animation
	#define N_ANIMATIONS 1

	AnimBlender AB;
	Animations Anim[N_ANIMATIONS];
	SkeletalAnimation SKA;

	// to provide textual feedback
	TextMaker txt;
	
	// Other application parameters
	float Ar;	// Aspect ratio

	glm::mat4 ViewPrj;
	glm::mat4 World;
	glm::vec3 Pos = glm::vec3(0,0,5);
	glm::vec3 cameraPos;
	float Yaw = glm::radians(0.0f);
	float Pitch = glm::radians(0.0f);
	float Roll = glm::radians(0.0f);

	glm::vec4 debug1 = glm::vec4(0);

    // Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 1000;
		windowHeight = 800;
		windowTitle = "CGProject - Medieval Village Sim";
    	windowResizable = GLFW_TRUE;

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
		
		// updates the textual output
		txt.resizeScreen(w, h);
	}
	
	// Here you load and setup all your Vulkan Models, Texutures and Physics manger.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSLglobal.init(this, {
					// this array contains the binding:
					// first  element : the binding number
					// second element : the type of element (buffer or texture)
					// third  element : the pipeline stage where it will be used
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
				  });

		DSLlocalChar.init(this, {
					// this array contains the binding:
					// first  element : the binding number
					// second element : the type of element (buffer or texture)
					// third  element : the pipeline stage where it will be used
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectChar), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
				  });

		DSLlocalSimp.init(this, {
					// this array contains the binding:
					// first  element : the binding number
					// second element : the type of element (buffer or texture)
					// third  element : the pipeline stage where it will be used
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
					{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1}
				  });

		DSLskyBox.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(skyBoxUniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
		  });
		DSLwaterVert.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(TimeUBO), 1},
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp),1},
		  });
        // TODO: forse basta il UBO con solo mvpMat, invece che anche le altre 2...
        DSLwaterFrag.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(GlobalUniformBufferObject), 1},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
		  });

        DSLgrass.init(this, {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp), 1},
			{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(TimeUBO), 1},
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1}
		  });

        DSLterrain.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp), 1},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
                {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
                {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1},
                {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1},
                {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4, 1},
                {6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5, 1},
                {7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6, 1},
                {8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7, 1},
                {9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 8, 1},
        });

        // This is for the PBR of terrain  -- SET 2
        DSLterrainFactors.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(TerrainFactorsUBO), 1},
        });

		DSLlocalPBR.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObjectSimp), 1},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
					{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
					{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1},
                    {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1}
				  });

        // This is for the PBR with specular glossines  -- SET 2
        DSLsgAoFactors.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SgAoMaterialFactorsUBO), 1},
        });

		VDchar.init(this, {
				  {0, sizeof(VertexChar), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexChar, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexChar, norm),
				         sizeof(glm::vec3), NORMAL},
				  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexChar, UV),
				         sizeof(glm::vec2), UV},
					{0, 3, VK_FORMAT_R32G32B32A32_UINT, offsetof(VertexChar, jointIndices),
				         sizeof(glm::uvec4), JOINTINDEX},
					{0, 4, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexChar, weights),
				         sizeof(glm::vec4), JOINTWEIGHT}
				});

		VDsimp.init(this, {
				  {0, sizeof(VertexSimp), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexSimp, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexSimp, norm),
				         sizeof(glm::vec3), NORMAL},
				  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexSimp, UV),
				         sizeof(glm::vec2), UV}
				});

		VDskyBox.init(this, {
		  {0, sizeof(skyBoxVertex), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
		  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(skyBoxVertex, pos),
				 sizeof(glm::vec3), POSITION}
		});

		VDtan.init(this, {
				  {0, sizeof(VertexTan), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexTan, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexTan, norm),
				         sizeof(glm::vec3), NORMAL},
				  {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexTan, UV),
				         sizeof(glm::vec2), UV},
				  {0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexTan, tan),
				         sizeof(glm::vec4), TANGENT}
				});

		VDRs.resize(4);
		VDRs[0].init("VDchar",   &VDchar);
		VDRs[1].init("VDsimp",   &VDsimp);
		VDRs[2].init("VDskybox", &VDskyBox);
		VDRs[3].init("VDtan",    &VDtan);

		// initializes the render passes
		RP.init(this);
		// sets the blue sky
		RP.properties[0].clearValue = {0.0f,0.9f,1.0f,1.0f};
		

		// Pipelines [Shader couples]
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		Pchar.init(this, &VDchar, "shaders/PosNormUvTanWeights.vert.spv", "shaders/CookTorranceForCharacter.frag.spv", {&DSLglobal, &DSLlocalChar});

		PsimpObj.init(this, &VDsimp, "shaders/SimplePosNormUV.vert.spv", "shaders/CookTorrance.frag.spv", {&DSLglobal, &DSLlocalSimp});

		PskyBox.init(this, &VDskyBox, "shaders/SkyBoxShader.vert.spv", "shaders/SkyBoxShader.frag.spv", {&DSLskyBox});
		PskyBox.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
		PskyBox.setCullMode(VK_CULL_MODE_BACK_BIT);
		PskyBox.setPolygonMode(VK_POLYGON_MODE_FILL);

        PWater.init(this, &VDsimp, "shaders/WaterShader.vert.spv", "shaders/WaterShader.frag.spv", {&DSLwaterVert, &DSLwaterFrag});

        Pgrass.init(this, &VDsimp, "shaders/GrassShader.vert.spv", "shaders/GrassShader.frag.spv", {&DSLglobal, &DSLgrass});
		
        Pterrain.init(this, &VDtan, "shaders/SimplePosNormUvTan.vert.spv", "shaders/TerrainShader.frag.spv", {&DSLglobal, &DSLterrain, &DSLterrainFactors});

		P_PBR_SpecGloss.init(this, &VDtan, "shaders/SimplePosNormUvTan.vert.spv", "shaders/PBR_SpecGloss.frag.spv", {&DSLglobal, &DSLlocalPBR, &DSLsgAoFactors});

        PRs.resize(7);
		PRs[0].init("CookTorranceChar", {
							 {&Pchar, {//Pipeline and DSL for the first pass
								 /*DSLglobal*/{},
								 /*DSLlocalChar*/{
										/*t0*/{true,  0, {}}// index 0 of the "texture" field in the json file
									 }
									}}
							  }, /*TotalNtextures*/1, &VDchar);
		PRs[1].init("CookTorranceNoiseSimp", {
							 {&PsimpObj, {//Pipeline and DSL for the first pass
								 /*DSLglobal*/{},
								 /*DSLlocalSimp*/{
										/*t0*/{true,  0, {}},// index 0 of the "texture" field in the json file
										/*t1*/{true,  1, {}} // index 1 of the "texture" field in the json file
									 }
									}}
							  }, /*TotalNtextures*/2, &VDsimp);
		PRs[2].init("SkyBox", {
							 {&PskyBox, {//Pipeline and DSL for the first pass
								 /*DSLskyBox*/{
										/*t0*/{true,  0, {}}// index 0 of the "texture" field in the json file
									 }
									}}
							  }, /*TotalNtextures*/1, &VDskyBox);
        PRs[3].init("Water", {
                            {&PWater, {//Pipeline and DSL for the first pass
                                    /*DSLwater*/{},
                                                {{true, 0, {}}
                                }
                            }}
                    }, /*TotalNtextures*/1, &VDsimp);
        PRs[4].init("Terrain", {
                {&Pterrain, {//Pipeline and DSL for the first pass
                        /*DSLglobal*/{},
                        /*DSLterrain*/{
                                             {true,  0, {}},
                                             {true,  1, {}},
                                             {true,  2, {}},
                                             {true,  3, {}},
                                             {true,  4, {}},
                                             {true,  5, {}},
                                             {true,  6, {}},
                                             {true,  7, {}},
                                             {true,  8, {}},
                                     },
                                     {}
                }}
        }, /*TotalNtextures*/9, &VDtan);
        PRs[5].init("Grass", {
                 {&Pgrass, {//Pipeline and DSL for the first pass
                     /*DSLgrass*/{
                            {},
                            {
                                    {true, 0, {}},
                                    {true, 1, {}}
                            }
                         }
                        }}
                  }, /*TotalNtextures*/2, &VDsimp);
        PRs[6].init("PBR_sg", {
							 {&P_PBR_SpecGloss, {//Pipeline and DSL for the first pass
								 {},    /*DSLglobal*/
								 {      /*DSLlocalPBR*/
										{true,  0, {}},     // albedo
										{true,  1, {}},     // normal
										{true,  2, {}},     // specular / glossiness
										{true,  3, {}},     // ambient occlusion
									 },
                                 {}   /*DSLsgAoFactors*/
									}}
							  }, 4, &VDtan);

		// sets the size of the Descriptor Set Pool
		DPSZs.uniformBlocksInPool = 1000;
		DPSZs.texturesInPool = 1000;
		DPSZs.setsInPool = 1000;
		
        std::cout << "\nLoading the scene\n\n";
		if(SC.init(this, /*Npasses*/1, VDRs, PRs, "assets/models/scene.json") != 0) {
			std::cout << "ERROR LOADING THE SCENE\n";
			exit(0);
		}

		// initializes animations
		for(int ian = 0; ian < N_ANIMATIONS; ian++) {
			Anim[ian].init(*SC.As[ian]);
		}
		AB.init({{0,32,0.0f,0}});
		SKA.init(Anim, N_ANIMATIONS, "Armature|mixamo.com|Layer0", 0);
		
		// submits the main command buffer
		submitCommandBuffer("main", 0, populateCommandBufferAccess, this);

		// initializes the textual output and Prepares for showing the FPS count
		txt.init(this, windowWidth, windowHeight);
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
		RP.create();
		
		// This creates a new pipeline (with the current surface), using its shaders for the provided render pass
        std::cout << "Creating pipelines\n";
		Pchar.create(&RP);
		PsimpObj.create(&RP);
		PskyBox.create(&RP);
        PWater.create(&RP);
        Pgrass.create(&RP);
		Pterrain.create(&RP);
        P_PBR_SpecGloss.create(&RP);

        std::cout << "Creating descriptor sets\n";
		SC.pipelinesAndDescriptorSetsInit();
		txt.pipelinesAndDescriptorSetsInit();

        std::cout << "pipelinesAndDescriptorSetsInit done\n";
    }

	// Here you destroy your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsCleanup() {
		Pchar.cleanup();
		PsimpObj.cleanup();
		PskyBox.cleanup();
        PWater.cleanup();
        Pgrass.cleanup();
		Pterrain.cleanup();
        P_PBR_SpecGloss.cleanup();
		RP.cleanup();

		SC.pipelinesAndDescriptorSetsCleanup();
		txt.pipelinesAndDescriptorSetsCleanup();
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// You also have to destroy the pipelines
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
		
		Pchar.destroy();	
		PsimpObj.destroy();
		PskyBox.destroy();		
        PWater.destroy();
        Pgrass.destroy();
        P_PBR_SpecGloss.destroy();
		Pterrain.destroy();

		RP.destroy();

		SC.localCleanup();	
		txt.localCleanup();

		for(auto & ian : Anim) {
			ian.cleanup();
		}
	}
	
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params) {
		// Simple trick to avoid having always 'T->'
		// in che code that populates the command buffer!
    std::cout << "Populating command buffer for " << currentImage << "\n";
		CGProject *T = (CGProject *)Params;
		T->populateCommandBuffer(commandBuffer, currentImage);
	}
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
	// This is the real place where the Command Buffer is written
		// begin standard pass
		RP.begin(commandBuffer, currentImage);
		SC.populateCommandBuffer(commandBuffer, 0, currentImage);
        RP.end(commandBuffer);
	}

// Here is where you update the uniforms, where logic of application is.
	void updateUniformBuffer(uint32_t currentImage) {
		static bool debounce = false;
		static int curDebounce = 0;
		
		// handle the ESC key to exit the app
		if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		if(glfwGetKey(window, GLFW_KEY_1)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_1;

				debug1.x = 1.0 - debug1.x;
			}
		} else {
			if((curDebounce == GLFW_KEY_1) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		if(glfwGetKey(window, GLFW_KEY_2)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_2;

				debug1.y = 1.0 - debug1.y;
			}
		} else {
			if((curDebounce == GLFW_KEY_2) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		if(glfwGetKey(window, GLFW_KEY_P)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_P;

				debug1.z = (float)(((int)debug1.z + 1) % 65);
            std::cout << "Showing bone index: " << debug1.z << "\n";
			}
		} else {
			if((curDebounce == GLFW_KEY_P) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		if(glfwGetKey(window, GLFW_KEY_O)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_O;

				debug1.z = (float)(((int)debug1.z + 64) % 65);
        std::cout << "Showing bone index: " << debug1.z << "\n";
			}
		} else {
			if((curDebounce == GLFW_KEY_O) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		static int curAnim = 0;
		if(glfwGetKey(window, GLFW_KEY_SPACE)) {
            PhysicsMgr.jumpPlayer();
        }

		// moves the view
		float deltaT = GameLogic();

		// updated the animation
		const float SpeedUpAnimFact = 0.85f;
		AB.Advance(deltaT * SpeedUpAnimFact);
		
		// defines the global parameters for the uniform
		const glm::mat4 lightView = glm::rotate(glm::mat4(1), glm::radians(-30.0f), glm::vec3(0.0f,1.0f,0.0f)) * glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(1.0f,0.0f,0.0f));
		const glm::vec3 lightDir = glm::vec3(lightView * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
	
		GlobalUniformBufferObject gubo{};

		gubo.lightDir = lightDir;
		gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo.eyePos = cameraPos;

		// defines the local parameters for the uniforms
		UniformBufferObjectChar uboc{};	
		uboc.debug1 = debug1;

		SKA.Sample(AB);
		std::vector<glm::mat4> *TMsp = SKA.getTransformMatrices();
		
		glm::mat4 AdaptMat =
			glm::scale(glm::mat4(1.0f), glm::vec3(0.01f)) * 
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f,0.0f,0.0f));

		int instanceId;
        int techniqueId = 0;
		// character
		for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
			for(int im = 0; im < TMsp->size(); im++) {
				uboc.mMat[im]   = AdaptMat * (*TMsp)[im];
				uboc.mvpMat[im] = ViewPrj * uboc.mMat[im];
				uboc.nMat[im] = glm::inverse(glm::transpose(uboc.mMat[im]));
			}

			SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
			SC.TI[techniqueId].I[instanceId].DS[0][1]->map(currentImage, &uboc, 0);  // Set 1
		}

		UniformBufferObjectSimp ubos{};
		// normal objects
        techniqueId++;
		for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
			ubos.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
			ubos.mvpMat = ViewPrj * ubos.mMat;
			ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));

			SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
			SC.TI[techniqueId].I[instanceId].DS[0][1]->map(currentImage, &ubos, 0);  // Set 1
		}
		
		// skybox pipeline
		skyBoxUniformBufferObject sbubo{};
        techniqueId++;
		sbubo.mvpMat = ViewPrj * glm::translate(glm::mat4(1), cameraPos) * glm::scale(glm::mat4(1), glm::vec3(100.0f));
		SC.TI[techniqueId].I[0].DS[0][0]->map(currentImage, &sbubo, 0);

        // Water objects
        techniqueId++;
        TimeUBO timeUbo{};
        timeUbo.time = glfwGetTime();
        ubos.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
        ubos.mvpMat = ViewPrj * ubos.mMat;
        ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));
        SC.TI[techniqueId].I[0].DS[0][0]->map(currentImage, &timeUbo, 0); // Set 0
        SC.TI[techniqueId].I[0].DS[0][0]->map(currentImage, &ubos, 1); // Set 0
        SC.TI[techniqueId].I[0].DS[0][1]->map(currentImage, &gubo, 0); // Set 0

		// Terrain objects
        techniqueId++;
        TerrainFactorsUBO terrainFactorsUbo{};
		for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
			ubos.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
			ubos.mvpMat = ViewPrj * ubos.mMat;
			ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));

            terrainFactorsUbo.maskBlendFactor = SC.TI[techniqueId].I[instanceId].factor1;
            terrainFactorsUbo.tilingFactor = SC.TI[techniqueId].I[instanceId].factor2;

			SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
			SC.TI[techniqueId].I[instanceId].DS[0][1]->map(currentImage, &ubos, 0);  // Set 1
			SC.TI[techniqueId].I[instanceId].DS[0][2]->map(currentImage, &terrainFactorsUbo, 0);  // Set 2
		}

        // Vegetation/Grass objects
        techniqueId++;
        for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
            ubos.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
            ubos.mvpMat = ViewPrj * ubos.mMat;
            ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));

            timeUbo.time = glfwGetTime();

            SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
            SC.TI[techniqueId].I[instanceId].DS[0][1]->map(currentImage, &ubos, 0);  // Set 1
            SC.TI[techniqueId].I[instanceId].DS[0][1]->map(currentImage, &timeUbo, 1);  // Set 1
        }

        // PBR_SpecGloss objects
        SgAoMaterialFactorsUBO sgAoUbo{};
        techniqueId++;
		for(instanceId = 0; instanceId < SC.TI[techniqueId].InstanceCount; instanceId++) {
			ubos.mMat   = SC.TI[techniqueId].I[instanceId].Wm;
			ubos.mvpMat = ViewPrj * ubos.mMat;
			ubos.nMat   = glm::inverse(glm::transpose(ubos.mMat));
            
            sgAoUbo.diffuseFactor = SC.TI[techniqueId].I[instanceId].diffuseFactor;
            sgAoUbo.specularFactor = SC.TI[techniqueId].I[instanceId].specularFactor;
            sgAoUbo.glossinessFactor = SC.TI[techniqueId].I[instanceId].factor1;
            sgAoUbo.aoFactor = SC.TI[techniqueId].I[instanceId].factor2;

			SC.TI[techniqueId].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // Set 0
			SC.TI[techniqueId].I[instanceId].DS[0][1]->map(currentImage, &ubos, 0); // Set 1
			SC.TI[techniqueId].I[instanceId].DS[0][2]->map(currentImage, &sgAoUbo, 0); // Set 2
		}

		// updates the FPS
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
		// Retrieve configs from PhysicsManger.hpp
		PlayerConfig physicsConfig;

		// Parameters
		// Camera FOV-y, Near Plane and Far Plane
		const float FOVy = glm::radians(45.0f);
		const float nearPlane = 0.1f;
		const float farPlane = 500.f;
		// Player starting point
		const glm::vec3 StartingPosition = physicsConfig.startPosition;
		// Camera target height and distance
		static float camHeight = 1.5;
		static float camDist = 5;
		// Camera Pitch limits
		const float minPitch = glm::radians(-8.75f);
		const float maxPitch = glm::radians(60.0f);
		// Rotation and motion speed
		const float ROT_SPEED = glm::radians(120.0f);
		const float MOVE_SPEED_BASE = physicsConfig.moveSpeed;
		const float MOVE_SPEED_RUN = physicsConfig.runSpeed;
		const float JUMP_FORCE = physicsConfig.jumpForce;
		const float MAX_CAM_DIST = 7.5;
		const float MIN_CAM_DIST = 1.5;

		// Integration with the timers and the controllers
		// TODO: Detect running by pressing SHIFT key (currently hardcoded as walking all the time)
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

		// Camera rotation controls
		static float Yaw = glm::radians(0.0f);
		static float Pitch = glm::radians(0.0f);
		static float relDir = glm::radians(0.0f);
		static float dampedRelDir = glm::radians(0.0f);
		static glm::vec3 dampedCamPos = StartingPosition;

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
		glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
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