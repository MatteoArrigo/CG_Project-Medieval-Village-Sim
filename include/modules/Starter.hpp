// This has been adapted from the Vulkan tutorial

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <array>
#include <cmath>
#include <math.h>
#include <chrono>
#include <unordered_map>
#include <map>

#ifdef STARTER_IMPLEMENTATION
// to allow splitting header and implementation
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define SINFL_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#endif

// GLM to support matrix operations
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform2.hpp>


// to load OBJ files
#include <tiny_obj_loader.h>

// to load images
#include <stb_image.h>

// to load GLTF
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#include <tiny_gltf.h>

// AES encription, to load MGCG files
#include <plusaes.hpp>

// Unzip library, to load MGCG files
#include <sinfl.h>

// use GLFW to support windowing
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


// For compile compatibility issues
#define M_E			2.7182818284590452354	/* e */
#define M_LOG2E		1.4426950408889634074	/* log_2 e */
#define M_LOG10E	0.43429448190325182765	/* log_10 e */
#define M_LN2		0.69314718055994530942	/* log_e 2 */
#define M_LN10		2.30258509299404568402	/* log_e 10 */
#define M_PI		3.14159265358979323846	/* pi */
#define M_PI_2		1.57079632679489661923	/* pi/2 */
#define M_PI_4		0.78539816339744830962	/* pi/4 */
#define M_1_PI		0.31830988618379067154	/* 1/pi */
#define M_2_PI		0.63661977236758134308	/* 2/pi */
#define M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */


const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete();
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pDebugMessenger);
			
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks* pAllocator);
			
void PrintVkError( VkResult result );

std::vector<char> readFile(const std::string& filename);

class BaseProject;

struct VertexBindingDescriptorElement {
	uint32_t binding;
	uint32_t stride;
	VkVertexInputRate inputRate;
};

enum VertexDescriptorElementUsage {POSITION, NORMAL, UV, COLOR, TANGENT, POS2D, JOINTWEIGHT, JOINTINDEX, OTHER};

struct VertexDescriptorElement {
	uint32_t binding;
	uint32_t location;
	VkFormat format;
	uint32_t offset;
	uint32_t size;
	VertexDescriptorElementUsage usage;
};

struct VertexComponent {
	bool hasIt;
	uint32_t offset;
};

struct VertexDescriptor {
	BaseProject *BP;
	
	VertexComponent Position;
	VertexComponent Pos2D;
	VertexComponent Normal;
	VertexComponent UV;
	VertexComponent Color;
	VertexComponent Tangent;
	VertexComponent JointWeight;
	VertexComponent JointIndex;

	std::vector<VertexBindingDescriptorElement> Bindings;
	std::vector<VertexDescriptorElement> Layout;
 	
 	void init(BaseProject *bp, std::vector<VertexBindingDescriptorElement> B, std::vector<VertexDescriptorElement> E);
	void cleanup();

	std::vector<VkVertexInputBindingDescription> getBindingDescription();
	std::vector<VkVertexInputAttributeDescription>
						getAttributeDescriptions();
};

enum ModelType {OBJ, GLTF, MGCG};

class AssetFile;

class Model {
	BaseProject *BP;
	
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VertexDescriptor *VD;

	public:
	glm::mat4 Wm;
	std::vector<unsigned char> vertices{};
	std::vector<uint32_t> indices{};
	void loadModelOBJ(std::string file);
	void makeOBJMesh(const tinyobj::shape_t *M, const tinyobj::attrib_t *A);
	static void getGLTFnodeTransforms(const tinygltf::Node *N, glm::vec3 &T, glm::vec3 &S, glm::quat &Q);
	void makeGLTFwm(const tinygltf::Node *N);
	void makeGLTFMesh(tinygltf::Model *M, const tinygltf::Primitive *Prm);
	void loadModelGLTF(std::string file, bool encoded);
	void createIndexBuffer();
	void createVertexBuffer();

	void init(BaseProject *bp, VertexDescriptor *VD, std::string file, ModelType MT);
	void initFromAsset(BaseProject *bp, VertexDescriptor *VD, AssetFile *AF, std::string AN, int Mid = 0, std::string NN = "");
	void initMesh(BaseProject *bp, VertexDescriptor *VD, bool printDebug = true);
	void cleanup();
  	void bind(VkCommandBuffer commandBuffer);
};

class AssetFile {
	friend Model;
	
	tinygltf::Model model;
	std::unordered_map<std::string, std::vector<const tinygltf::Primitive *>> GLTFmeshes;
	std::unordered_map<std::string, const tinygltf::Node *> GLTFnodes;

	// OBJ asset stuff
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::unordered_map<std::string, const tinyobj::shape_t *> OBJmeshes;
	
	ModelType type;
	
	public:
	void initGLTF(std::string file);
	void initOBJ(std::string file);
	void init(std::string file, ModelType MT);
	ModelType getType() {return type;}
	tinygltf::Model *getGLTFmodel() {return &model;}
	void cleanup();
};

struct Texture {
	BaseProject *BP;
	uint32_t mipLevels;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	int imgs;
	static const int maxImgs = 6;
	
	void createTextureImage(std::vector<std::string>files, VkFormat Fmt = VK_FORMAT_R8G8B8A8_SRGB);
	void createTextureImageView(VkFormat Fmt = VK_FORMAT_R8G8B8A8_SRGB);
	void createTextureSampler(VkFilter magFilter = VK_FILTER_LINEAR,
							 VkFilter minFilter = VK_FILTER_LINEAR,
							 VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
							 VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
							 VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
							 VkBool32 anisotropyEnable = VK_TRUE,
							 float maxAnisotropy = 16,
							 float maxLod = -1
							);

	void init(BaseProject *bp, std::string file, VkFormat Fmt = VK_FORMAT_R8G8B8A8_SRGB, bool initSampler = true);
	void initCubic(BaseProject *bp, std::vector<std::string>, VkFormat Fmt = VK_FORMAT_R8G8B8A8_SRGB);
	VkDescriptorImageInfo getViewAndSampler();
	void cleanup();
};

struct DescriptorSetLayoutBinding {
	uint32_t binding;
	VkDescriptorType type;
	VkShaderStageFlags flags;
	int linkSize;
	int count;
};


struct DescriptorSetLayout {
	BaseProject *BP;
 	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<DescriptorSetLayoutBinding> Bindings;
	int imgInfoSize;
 	
 	void init(BaseProject *bp, std::vector<DescriptorSetLayoutBinding> B);
	void cleanup();
};

enum AttchmentType {COLOR_AT, DEPTH_AT, RESOLVE_AT};

struct AttachmentProperties {
	AttchmentType type;
	VkFormat format;
	int usage;
	int aspect;
	bool doDepthTransition;
	bool swapChain;
	
	VkClearValue clearValue;

    VkSampleCountFlagBits samples;
    VkAttachmentLoadOp    loadOp;
    VkAttachmentStoreOp   storeOp;
    VkAttachmentLoadOp    stencilLoadOp;
    VkAttachmentStoreOp   stencilStoreOp;
    VkImageLayout         initialLayout;
    VkImageLayout         finalLayout;
    VkImageLayout         refLayout;
};

struct RenderPass;

struct FrameBufferAttachment {
	RenderPass *RP;
	
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
	AttachmentProperties *properties;
	
	VkAttachmentDescription descr;
	VkAttachmentReference ref;
	
	VkSampler sampler;
	bool freeSampler;
	
  	void init(RenderPass *rp, AttachmentProperties *p, bool initSampler);
	void cleanup();
	void destroy();
	void createResources();
	void createDescriptionAndReference(int num);
	VkImageView getView(int currentImage);
	VkDescriptorImageInfo getViewAndSampler();	
	void createTextureSampler(
							 VkFilter magFilter = VK_FILTER_LINEAR,
							 VkFilter minFilter = VK_FILTER_LINEAR,
							 VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
							 VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
							 VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
							 VkBool32 anisotropyEnable = VK_TRUE,
							 float maxAnisotropy = 16,
							 float maxLod = -1
							);
};

enum StockAttchmentsConfiguration {AT_SURFACE_AA_DEPTH, AT_ONE_COLOR_AND_DEPTH, AT_DEPTH_ONLY, AT_SURFACE_NOAA_DEPTH, AT_NO_ATTCHMENTS};

enum StockAttchmentsDependencies {ATDEP_SIMPLE, ATDEP_SURFACE_ONLY, ATDEP_DEPTH_TRANS, ATDEP_NO_DEP};

struct RenderPass {
	BaseProject *BP;
	
	int width;
	int height;
	int count;
	
	int colorAttchementsCount;
	int firstColorAttIdx;
	int depthAttIdx;
	int resolveAttIdx;
	
	std::vector <AttachmentProperties> properties;
	std::vector <FrameBufferAttachment> attachments;
	
	std::vector<VkFramebuffer> frameBuffers;
	std::vector<VkSubpassDependency> dependencies;

	std::vector<VkClearValue> clearValues;

	VkRenderPass renderPass;

  	void init(BaseProject *bp, int w = -1, int h = -1, int _count = -1, std::vector <AttachmentProperties> *p = nullptr, std::vector<VkSubpassDependency> *d = nullptr, bool initSampler = false);
	void create();
	void begin(VkCommandBuffer commandBuffer, int currentImage);
	void end(VkCommandBuffer commandBuffer);
	void cleanup();
	void destroy();
	static std::vector <AttachmentProperties> *getStandardAttchmentsProperties(StockAttchmentsConfiguration cfg, BaseProject *BP);
	static std::vector<VkSubpassDependency> *getStandardDependencies(StockAttchmentsDependencies cfg);
	
	private:
	void createRenderPass();
	void createFramebuffers();
};

struct Pipeline {
	BaseProject *BP;
	VkPipeline graphicsPipeline;
  	VkPipelineLayout pipelineLayout;
 
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	std::vector<DescriptorSetLayout *> D;
	std::vector<VkPushConstantRange> PK;	
	
	VkCompareOp compareOp;
	VkPolygonMode polyModel;
 	VkCullModeFlagBits CM;
 	bool transp;
	VkPrimitiveTopology topology;
	
	VertexDescriptor *VD;
  	
  	void init(BaseProject *bp, VertexDescriptor *vd,
			  const std::string& VertShader, const std::string& FragShader,
  			  std::vector<DescriptorSetLayout *> d,
			  std::vector<VkPushConstantRange> pk = {});
  	void create(RenderPass *RP);
  	void destroy();
  	void bind(VkCommandBuffer commandBuffer);
	void setCompareOp(VkCompareOp _compareOp);
	void setPolygonMode(VkPolygonMode _polyModel);
	void setCullMode(VkCullModeFlagBits _CM);
	void setTransparency(bool _transp);
	void setTopology(VkPrimitiveTopology _topology);
  	
  	VkShaderModule createShaderModule(const std::vector<char>& code);
	void cleanup();
};

struct DescriptorSet {
	BaseProject *BP;

	std::vector<std::vector<VkBuffer>> uniformBuffers;
	std::vector<std::vector<VkDeviceMemory>> uniformBuffersMemory;
	std::vector<VkDescriptorSet> descriptorSets;
	DescriptorSetLayout *Layout;
	
	std::vector<bool> toFree;

	void init(BaseProject *bp, DescriptorSetLayout *L,
						 std::vector<VkDescriptorImageInfo>VaSs);
	void cleanup();
  	void bind(VkCommandBuffer commandBuffer, Pipeline &P, int setId, int currentImage);
  	void map(int currentImage, void *src, int slot);
};


struct PoolSizes {
	int uniformBlocksInPool = 0;
	int texturesInPool = 0;
	int setsInPool = 0;
};

typedef void (* pNCBfunc)(VkCommandBuffer commandBuffer, int i, void *params);
typedef void (* pNCBfree)(void *params);

enum NamedCommandBuffersStates {NCBS_SUBMITTED, NCBS_IN_CREATION, NCBS_IN_USE, NCBS_TO_DELETE, NCBS_DELETING, NCBS_DETACHED, NCBS_DEAD};

struct NamedCommandBuffer {
	std::string name;
	int order;
	std::vector<VkCommandBuffer *> cb;
	pNCBfunc filler;
	pNCBfree cleaner;
	void *params;

	NamedCommandBuffersStates state;
	std::vector<bool> inQueue;
};

struct NamedCommandBufferVersions {
	NamedCommandBuffer *current;
	std::vector<NamedCommandBuffer *>old;
};

// MAIN ! 
class BaseProject {
	friend class VertexDescriptor;
	friend class Model;
	friend class Texture;
	friend class FrameBufferAttachment;
	friend class RenderPass;
	friend class Pipeline;
	friend class DescriptorSetLayout;
	friend class DescriptorSet;

public:
	virtual void setWindowParameters() = 0;
    void run(); 

	PoolSizes DPSZs;

protected:
	uint32_t windowWidth;
	uint32_t windowHeight;
	bool windowResizable;
	std::string windowTitle;
	VkClearColorValue initialBackgroundColor;

    GLFWwindow* window;
    VkInstance instance;

	VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
	VkCommandPool commandPool;
	
	std::unordered_map<std::string, NamedCommandBufferVersions> namedCommandBuffers = {};
	
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
		
 	VkDescriptorPool descriptorPool;

	VkDebugUtilsMessengerEXT debugMessenger;

	size_t currentFrame = 0;
	bool framebufferResized = false;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	
    void initWindow();

	virtual void onWindowResize(int w, int h) = 0;
	
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);	

	virtual void localInit() = 0;
	virtual void pipelinesAndDescriptorSetsInit() = 0;

    void initVulkan();
    void createInstance();
	std::vector<const char*> getRequiredExtensions();
	bool checkIfItHasExtension(const char *ext);
	bool checkIfItHasDeviceExtension(VkPhysicalDevice device, const char *ext);
	bool checkValidationLayerSupport();
	void populateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
	void setupDebugMessenger();
	void createSurface();
	
	class deviceReport {
		public:
			bool swapChainAdequate;
			bool swapChainFormatSupport;
			bool swapChainPresentModeSupport;
			bool completeQueueFamily;
			bool anisotropySupport;
			bool extensionsSupported;
			std::set<std::string> requiredExtensions;
			void print();
	};
	
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device, deviceReport &devRep);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device, deviceReport &devRep);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSampleCountFlagBits getMaxUsableSampleCount();
	void createLogicalDevice();
	void createSwapChain();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(
			const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void createImageViews();
	VkImageView createImageView(VkImage image, VkFormat format,
							VkImageAspectFlags aspectFlags,
							uint32_t mipLevels, VkImageViewType type, int layerCount
							);
	void createCommandPool();
	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat> candidates,
					VkImageTiling tiling, VkFormatFeatureFlags features);
	bool hasStencilComponent(VkFormat format);
	void createImage(uint32_t width, uint32_t height,
				 uint32_t mipLevels, int imgCount,
				 VkSampleCountFlagBits numSamples, 
				 VkFormat format,
				 VkImageTiling tiling, VkImageUsageFlags usage,
				 VkImageCreateFlags cflags,
				 VkMemoryPropertyFlags properties, VkImage& image,
				 VkDeviceMemory& imageMemory);	
	void generateMipmaps(VkImage image, VkFormat imageFormat,
					 int32_t texWidth, int32_t texHeight,
					 uint32_t mipLevels, int layerCount);
	void transitionImageLayout(VkImage image, VkFormat format,
				VkImageLayout oldLayout, VkImageLayout newLayout,
				uint32_t mipLevels, int layersCount);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t
					   width, uint32_t height, int layerCount);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
				  VkMemoryPropertyFlags properties,
				  VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	uint32_t findMemoryType(uint32_t typeFilter,
						VkMemoryPropertyFlags properties);
	void createDescriptorPool();
						
	public:
	void submitCommandBuffer(std::string name, int order, pNCBfunc populateNewCommandBuffer, void *params, pNCBfree onErase = nullptr);

	protected:
	void removeBuffer(std::string name);
	void clearNamedCommandBufferForImage(NamedCommandBuffer *ncb, int img);
	void clearNamedCommandBuffer(NamedCommandBuffer *ncb);
	void clearCommandBuffers();
	void resetCommandBuffers();
	void createSyncObjects();
	void mainLoop();
	void createCommandBuffer(NamedCommandBuffer *ncb, int imageIndex);
	void updateCommandBuffers(std::vector<VkCommandBuffer> &buffers, int imageIndex);
	void drawFrame();
	
	virtual void updateUniformBuffer(uint32_t currentImage) = 0;
	virtual void pipelinesAndDescriptorSetsCleanup() = 0;
	virtual void localCleanup() = 0;

	void recreateSwapChain();
	void cleanupSwapChain();
	void cleanup();
	void RebuildPipeline();
	
	// Control Wrapper
	void handleGamePad(int id,  glm::vec3 &m, glm::vec3 &r, bool &fire);
	void getSixAxis(float &deltaT,
				glm::vec3 &m,
				glm::vec3 &r,
				bool &fire);
	
	// Public part of the base class
	public:
	// Debug commands
	void printFloat(const char *Name, float v);
	void printVec2(const char *Name, glm::vec2 v);
	void printVec3(const char *Name, glm::vec3 v);
	void printVec4(const char *Name, glm::vec4 v);
	void printMat3(const char *Name, glm::mat3 v);
	void printMat4(const char *Name, glm::mat4 v);
	void printQuat(const char *Name, glm::quat q);
	
	// to support screenshot
	// Taken from the Sasha Willem sample by copy&paste
	// This could be better integrated in the code, but for the moment,
	// i cannot afford to do it, so it stays like this, even if it is awful!	
	private:
	inline VkCommandBufferBeginInfo vks_initializers_commandBufferBeginInfo();
	inline VkCommandBufferAllocateInfo vks_initializers_commandBufferAllocateInfo(
		VkCommandPool commandPool, 
		VkCommandBufferLevel level, 
		uint32_t bufferCount);
	VkCommandBuffer vulkanDevice_createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin);
	inline VkFenceCreateInfo vks_initializers_fenceCreateInfo(VkFenceCreateFlags flags = 0);
	inline VkSubmitInfo vks_initializers_submitInfo();
	void vulkanDevice_flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool freeCmd);	
	inline VkMemoryAllocateInfo vks_initializers_memoryAllocateInfo();
	inline VkImageCreateInfo vks_initializers_imageCreateInfo();
	inline VkImageMemoryBarrier vks_initializers_imageMemoryBarrier();
	void vks_tools_insertImageMemoryBarrier(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkImageSubresourceRange subresourceRange);
	
	
	// Custom define for better code readability
	#define VK_FLAGS_NONE 0
	// Default fence timeout in nanoseconds
	#define DEFAULT_FENCE_TIMEOUT 100000000000		

	public:
	bool screenshotSaved = false;
	void saveScreenshot(const char *filename, int currentBuffer);
	
};




/**** Implementations starts here ****/

#ifdef STARTER_IMPLEMENTATION
// external functions and constants

std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool QueueFamilyIndices::isComplete() {
	return graphicsFamily.has_value() &&
		   presentFamily.has_value();
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
				vkGetInstanceProcAddr(instance,
					"vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
				vkGetInstanceProcAddr(instance,
					"vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

struct errorcode {
	VkResult resultCode;
	std::string meaning;
}

ErrorCodes[ ] = {
	{ VK_NOT_READY, "Not Ready" },
	{ VK_TIMEOUT, "Timeout" },
	{ VK_EVENT_SET, "Event Set" },
	{ VK_EVENT_RESET, "Event Reset" },
	{ VK_INCOMPLETE, "Incomplete" },
	{ VK_ERROR_OUT_OF_HOST_MEMORY, "Out of Host Memory" },
	{ VK_ERROR_OUT_OF_DEVICE_MEMORY, "Out of Device Memory" },
	{ VK_ERROR_INITIALIZATION_FAILED, "Initialization Failed" },
	{ VK_ERROR_DEVICE_LOST, "Device Lost" },
	{ VK_ERROR_MEMORY_MAP_FAILED, "Memory Map Failed" },
	{ VK_ERROR_LAYER_NOT_PRESENT, "Layer Not Present" },
	{ VK_ERROR_EXTENSION_NOT_PRESENT, "Extension Not Present" },
	{ VK_ERROR_FEATURE_NOT_PRESENT, "Feature Not Present" },
	{ VK_ERROR_INCOMPATIBLE_DRIVER, "Incompatible Driver" },
	{ VK_ERROR_TOO_MANY_OBJECTS, "Too Many Objects" },
	{ VK_ERROR_FORMAT_NOT_SUPPORTED, "Format Not Supported" },
	{ VK_ERROR_FRAGMENTED_POOL, "Fragmented Pool" },
	{ VK_ERROR_SURFACE_LOST_KHR, "Surface Lost" },
	{ VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "Native Window in Use" },
	{ VK_SUBOPTIMAL_KHR, "Suboptimal" },
	{ VK_ERROR_OUT_OF_DATE_KHR, "Error Out of Date" },
	{ VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "Incompatible Display" },
	{ VK_ERROR_VALIDATION_FAILED_EXT, "Valuidation Failed" },
	{ VK_ERROR_INVALID_SHADER_NV, "Invalid Shader" },
	{ VK_ERROR_OUT_OF_POOL_MEMORY_KHR, "Out of Pool Memory" },
	{ VK_ERROR_INVALID_EXTERNAL_HANDLE, "Invalid External Handle" },

};

void PrintVkError( VkResult result ) {
	const int numErrorCodes = sizeof( ErrorCodes ) / sizeof( struct errorcode );
	std::string meaning = "";
	for( int i = 0; i < numErrorCodes; i++ ) {
		if( result == ErrorCodes[i].resultCode ) {
			meaning = ErrorCodes[i].meaning;
			break;
		}
	}
	std::cout << "Error: " << result << ", " << meaning << "\n";
}

std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		std::cout << "Failed to open: " << filename << "\n";
		throw std::runtime_error("failed to open file!");
	}
	
	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);
//std::cout << filename << " -> " << fileSize << " B\n";	 
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	 
	file.close();
	 
	return buffer;
}

// BaseProject class members

void BaseProject::run() {
	windowResizable = GLFW_FALSE;

	setWindowParameters();
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void BaseProject::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, windowResizable);

	window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

}

void BaseProject::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<BaseProject*>
					(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
	app->onWindowResize(width, height);
}

void BaseProject::initVulkan() {
	createInstance();				
	setupDebugMessenger();			
	createSurface();				
	pickPhysicalDevice();			
	createLogicalDevice();			
	createSwapChain();				
	createImageViews();				

	createCommandPool();			
	localInit();

	createDescriptorPool();			
	pipelinesAndDescriptorSetsInit();

//		createCommandBuffers();			
	createSyncObjects();			 
}

void BaseProject::createInstance() {
std::cout << "Starting createInstance()\n"  << std::flush;
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = windowTitle.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
			
//		createInfo.enabledExtensionCount = glfwExtensionCount;
//		createInfo.ppEnabledExtensionNames = glfwExtensions;

	createInfo.enabledLayerCount = 0;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount =
		static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();		

	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	
	if (!checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		createInfo.enabledLayerCount =
			static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)
								&debugCreateInfo;
	
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	
	if(result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create instance!");
	}
}

std::vector<const char*> BaseProject::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions =
		glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions,
		glfwExtensions + glfwExtensionCount);
		
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);		
	
	if(checkIfItHasExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

	}
	if(checkIfItHasExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}
	
	return extensions;
}

bool BaseProject::checkIfItHasExtension(const char *ext) {
	uint32_t extCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);

	std::vector<VkExtensionProperties> availableExt(extCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount,
				availableExt.data());
				
	bool found = false;
	for (const auto& extProp : availableExt) {
		if (strcmp(ext, extProp.extensionName) == 0) {
			found = true;
			break;
		}
	}
	return found;
}

bool BaseProject::checkIfItHasDeviceExtension(VkPhysicalDevice device, const char *ext) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr,
				&extensionCount, nullptr);
				
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr,
				&extensionCount, availableExtensions.data());
				
	bool found = false;
	for (const auto& extProp : availableExtensions) {
		if (strcmp(ext, extProp.extensionName) == 0) {
			found = true;
			break;
		}
	}
	return found;
}

bool BaseProject::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount,
				availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
	
		if (!layerFound) {
			return false;
		}
	}
	
	return true;    
}

void BaseProject::populateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType =
		VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL BaseProject::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;		
	return VK_FALSE;
}

void BaseProject::setupDebugMessenger() {

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(createInfo);
	
	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
			&debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void BaseProject::createSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface)
			!= VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void BaseProject::deviceReport::print() {
	std::cout << "swapChainAdequate: " << swapChainAdequate <<"\n";
	std::cout << "swapChainFormatSupport: " << swapChainFormatSupport <<"\n";
	std::cout << "swapChainPresentModeSupport: " << swapChainPresentModeSupport <<"\n";
	std::cout << "completeQueueFamily: " << completeQueueFamily <<"\n";
	std::cout << "anisotropySupport: " << anisotropySupport <<"\n";
	std::cout << "extensionsSupported: " << extensionsSupported <<"\n";
	
	for (const auto& ext : requiredExtensions) {
		std::cout << "Extension <" << ext <<"> unsupported. \n";
	}
}

void BaseProject::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	deviceReport devRep;
	 
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	
	std::cout << "Physical devices found: " << deviceCount << "\n";
	
	for (const auto& device : devices) {
		if(checkIfItHasDeviceExtension(device, "VK_KHR_portability_subset")) {
			deviceExtensions.push_back("VK_KHR_portability_subset");
		}
					
		bool suitable = isDeviceSuitable(device, devRep);
		if (suitable) {
			physicalDevice = device;
			msaaSamples = getMaxUsableSampleCount();
			std::cout << "\n\nMaximum samples for anti-aliasing: " << msaaSamples << "\n\n\n";
			break;
		} else {
			std::cout << "Device " << device << " is not suitable\n";
			devRep.print();
		}
	}
	
	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

bool BaseProject::isDeviceSuitable(VkPhysicalDevice device, deviceReport &devRep) {
	QueueFamilyIndices indices = findQueueFamilies(device);

	devRep.extensionsSupported = checkDeviceExtensionSupport(device, devRep);

	devRep.swapChainAdequate = false;
	if (devRep.extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		devRep.swapChainFormatSupport = swapChainSupport.formats.empty();
		devRep.swapChainPresentModeSupport = swapChainSupport.presentModes.empty();
		devRep.swapChainAdequate = !devRep.swapChainPresentModeSupport &&
							!devRep.swapChainPresentModeSupport;
	}
	
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
	
	devRep.completeQueueFamily = indices.isComplete();
	devRep.anisotropySupport = supportedFeatures.samplerAnisotropy;
	
	return devRep.completeQueueFamily && devRep.extensionsSupported && devRep.swapChainAdequate &&
					devRep.anisotropySupport;
}

QueueFamilyIndices BaseProject::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;
	
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
					nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
							queueFamilies.data());
							
	int i=0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
			
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}			
		i++;
	}

	return indices;
}

bool BaseProject::checkDeviceExtensionSupport(VkPhysicalDevice device, deviceReport &devRep) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr,
				&extensionCount, nullptr);
				
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr,
				&extensionCount, availableExtensions.data());
				
	std::set<std::string> requiredExtensions(deviceExtensions.begin(),
				deviceExtensions.end());
	devRep.requiredExtensions = requiredExtensions;
				
	for (const auto& extension : availableExtensions){
		devRep.requiredExtensions.erase(extension.extensionName);
	}

	return devRep.requiredExtensions.empty();
}

SwapChainSupportDetails BaseProject::querySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;
	
	 vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
			&details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
			nullptr);
			
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface,
				&formatCount, details.formats.data());
	}
	
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
		&presentModeCount, nullptr);
	
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
				&presentModeCount, details.presentModes.data());
	}
	 
	return details;
}

VkSampleCountFlagBits BaseProject::getMaxUsableSampleCount() {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	
	VkSampleCountFlags counts =
			physicalDeviceProperties.limits.framebufferColorSampleCounts &
			physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}	

void BaseProject::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies =
			{indices.graphicsFamily.value(), indices.presentFamily.value()};
	
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;
	deviceFeatures.fillModeNonSolid  = VK_TRUE;
	
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = 
		static_cast<uint32_t>(queueCreateInfos.size());
	
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount =
			static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		createInfo.enabledLayerCount = 
				static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	
	VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
	
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create logical device!");
	}
	
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void BaseProject::createSwapChain() {
	SwapChainSupportDetails swapChainSupport =
			querySwapChainSupport(physicalDevice);
	VkSurfaceFormatKHR surfaceFormat =
			chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode =
			chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
	
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
							VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
									 indices.presentFamily.value()};
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}
	
	 createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	 createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	 createInfo.presentMode = presentMode;
	 createInfo.clipped = VK_TRUE;
	 createInfo.oldSwapchain = VK_NULL_HANDLE;
	 
	 VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
	 if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create swap chain!");
	}
	
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
			swapChainImages.data());
			
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

VkSurfaceFormatKHR BaseProject::chooseSwapSurfaceFormat(
			const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	
	return availableFormats[0];
}

VkPresentModeKHR BaseProject::chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D BaseProject::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		
		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		actualExtent.width = std::max(capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actualExtent.height));
		return actualExtent;
	}
}

void BaseProject::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat,VK_IMAGE_ASPECT_COLOR_BIT, 1, VK_IMAGE_VIEW_TYPE_2D, 1);
	}
}

VkImageView BaseProject::createImageView(VkImage image, VkFormat format,
							VkImageAspectFlags aspectFlags,
							uint32_t mipLevels, VkImageViewType type, int layerCount
							) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = type;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layerCount;
	VkImageView imageView;

	VkResult result = vkCreateImageView(device, &viewInfo, nullptr,
			&imageView);
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create image view!");
	}
	return imageView;
}



void BaseProject::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = 
			findQueueFamilies(physicalDevice);
			
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0; // Optional
	
	VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create command pool!");
	}
}


VkFormat BaseProject::findDepthFormat() {
	return findSupportedFormat({VK_FORMAT_D32_SFLOAT,
								VK_FORMAT_D32_SFLOAT_S8_UINT,
								VK_FORMAT_D24_UNORM_S8_UINT},
								VK_IMAGE_TILING_OPTIMAL, 
							VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

VkFormat BaseProject::findSupportedFormat(const std::vector<VkFormat> candidates,
					VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR &&
					(props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
					(props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	
	throw std::runtime_error("failed to find supported format!");
}

bool BaseProject::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		   format == VK_FORMAT_D24_UNORM_S8_UINT;
}
	
void BaseProject::createImage(uint32_t width, uint32_t height,
				 uint32_t mipLevels, int imgCount,
				 VkSampleCountFlagBits numSamples, 
				 VkFormat format,
				 VkImageTiling tiling, VkImageUsageFlags usage,
				 VkImageCreateFlags cflags,
				 VkMemoryPropertyFlags properties, VkImage& image,
				 VkDeviceMemory& imageMemory) {		
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = imgCount;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = numSamples;
	imageInfo.flags = cflags; 
	
	VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create image!");
	}
	
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
										properties);
	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) !=
							VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

void BaseProject::generateMipmaps(VkImage image, VkFormat imageFormat,
					 int32_t texWidth, int32_t texHeight,
					 uint32_t mipLevels, int layerCount) {
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat,
						&formatProperties);

	if (!(formatProperties.optimalTilingFeatures &
				VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layerCount;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) { 
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		
		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							 0, nullptr, 0, nullptr,
							 1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = layerCount;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
							   mipHeight > 1 ? mipHeight/2:1, 1};
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = layerCount;
		
		vkCmdBlitImage(commandBuffer, image,
					   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
					   &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
							 0, nullptr, 0, nullptr,
							 1, &barrier);
		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer,
						 VK_PIPELINE_STAGE_TRANSFER_BIT,
						 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						 0, nullptr, 0, nullptr,
						 1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

void BaseProject::transitionImageLayout(VkImage image, VkFormat format,
				VkImageLayout oldLayout, VkImageLayout newLayout,
				uint32_t mipLevels, int layersCount) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layersCount;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
				newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
			   newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
								VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}
	vkCmdPipelineBarrier(commandBuffer,
							sourceStage, destinationStage, 0,
							0, nullptr, 0, nullptr, 1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

void BaseProject::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t
					   width, uint32_t height, int layerCount) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};
	
	vkCmdCopyBufferToImage(commandBuffer, buffer, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer BaseProject::beginSingleTimeCommands() { 
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
	
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	
	return commandBuffer;
}

void BaseProject::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);
	
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);
	
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void BaseProject::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
				  VkMemoryPropertyFlags properties,
				  VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	VkResult result =
			vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create vertex buffer!");
	}
	
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
	
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
			findMemoryType(memRequirements.memoryTypeBits, properties);

	result = vkAllocateMemory(device, &allocInfo, nullptr,
			&bufferMemory);
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}
	
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

uint32_t BaseProject::findMemoryType(uint32_t typeFilter,
						VkMemoryPropertyFlags properties) {
	 VkPhysicalDeviceMemoryProperties memProperties;
	 vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	 
	 for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && 
			(memProperties.memoryTypes[i].propertyFlags & properties) ==
					properties) {
			return i;
		}
	}
	
	throw std::runtime_error("failed to find suitable memory type!");
}

void BaseProject::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(DPSZs.uniformBlocksInPool * swapChainImages.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(DPSZs.texturesInPool * swapChainImages.size());
														 
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());;
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(DPSZs.setsInPool * swapChainImages.size());
	
	VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr,
								&descriptorPool);
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void BaseProject::submitCommandBuffer(std::string name, int order, pNCBfunc populateNewCommandBuffer, void *params, pNCBfree onErase) {
	int sz = swapChainImageViews.size();

	NamedCommandBuffer *nncb = new NamedCommandBuffer{name, order, {}, populateNewCommandBuffer, onErase, params, NCBS_SUBMITTED, {}};
	nncb->cb.resize(sz);
	nncb->inQueue.resize(sz);
	for(int i = 0; i < sz; i++) {
		nncb->inQueue[i] = false;
	}

	auto found = namedCommandBuffers.find(name);
	if(found != namedCommandBuffers.end()) {
		// If this named command buffer was already pending,
		// remove the previous instance
		found->second.current->state = NCBS_TO_DELETE;
		found->second.old.push_back(found->second.current);
		found->second.current = nncb;
//std::cout << "Existing command buffer '" << name << "', old size: " << found->second.old.size() << "\n";
	} else {
		// otherwise, add a new named command buffer
		namedCommandBuffers[name] = {nncb, {}};
//std::cout << "New command buffer '" << name << "'\n";
	}
}

void BaseProject::removeBuffer(std::string name) {
	auto found = namedCommandBuffers.find(name);
	if(found != namedCommandBuffers.end()) {
		// A command buffer must be found to be removed
		found->second.current->state = NCBS_TO_DELETE;
		found->second.old.push_back(found->second.current);
		found->second.current = nullptr;
	} else {
		// just print a warning message
		std::cout << "Try to delete a non-submitted command buffer: " << name << "\n";
	}
}

void BaseProject::clearNamedCommandBufferForImage(NamedCommandBuffer *ncb, int img) {
	if(ncb->inQueue[img]) {
		vkFreeCommandBuffers(device, commandPool, 1,
						 ncb->cb[img]);
		free(ncb->cb[img]);
		ncb->inQueue[img] = false;
	}
}

void BaseProject::clearNamedCommandBuffer(NamedCommandBuffer *ncb) {
	int sz = swapChainImageViews.size();

	for(int i = 0; i < sz; i++) {
		clearNamedCommandBufferForImage(ncb, i);
	}
	if(ncb->cleaner != nullptr) {
//std::cout << "Calling cleaner function\n";
		ncb->cleaner(ncb->params);
	} else {
//std::cout << "Calling cleaner function is null, so no call for c.b. '" << ncb->name <<"'\n";
	}
	delete ncb;
}

void BaseProject::clearCommandBuffers() {
	for(auto &v : namedCommandBuffers) {
		// check it was allocated
		if(v.second.current != nullptr) {
			clearNamedCommandBuffer(v.second.current);
		}
		for(auto c : v.second.old) {
			clearNamedCommandBuffer(c);
		}
	}
	namedCommandBuffers.clear();
}

void BaseProject::resetCommandBuffers() {
	int sz = swapChainImageViews.size();

	for(auto &v : namedCommandBuffers) {
		// check it was allocated
		if(v.second.current != nullptr) {
			for(int i = 0; i < sz; i++) {
				if(v.second.current->inQueue[i]) {
					vkFreeCommandBuffers(device, commandPool, 1,
									 v.second.current->cb[i]);
					free(v.second.current->cb[i]);
					v.second.current->inQueue[i] = false;
				}
			}
			v.second.current->state = NCBS_SUBMITTED;
		}
	}
}

void BaseProject::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
			
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkResult result1 = vkCreateSemaphore(device, &semaphoreInfo, nullptr,
							&imageAvailableSemaphores[i]);
		VkResult result2 = vkCreateSemaphore(device, &semaphoreInfo, nullptr,
							&renderFinishedSemaphores[i]);
		VkResult result3 = vkCreateFence(device, &fenceInfo, nullptr,
							&inFlightFences[i]);
		if (result1 != VK_SUCCESS ||
			result2 != VK_SUCCESS ||
			result3 != VK_SUCCESS) {
			PrintVkError(result1);
			PrintVkError(result2);
			PrintVkError(result3);
			throw std::runtime_error("failed to create synchronization objects for a frame!!");
		}
	}
}

void BaseProject::mainLoop() {
	while (!glfwWindowShouldClose(window)){
		glfwPollEvents();
		drawFrame();
	}
	
	vkDeviceWaitIdle(device);
}

void BaseProject::createCommandBuffer(NamedCommandBuffer *ncb, int imageIndex) {
//std::cout << "Buffer: '" << ncb->name << "', id: " << imageIndex << "\n";

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	
	VkCommandBuffer *cb = (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer));
//std::cout << "Allocating \n";		
	VkResult result = vkAllocateCommandBuffers(device, &allocInfo, cb);
//std::cout << "Checking \n";		
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to allocate command buffer!");
	}
	ncb->cb[imageIndex] = cb;
	ncb->inQueue[imageIndex] = true;

//std::cout << "Beginning\n";
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(*cb, &beginInfo) !=
				VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}
	
//std::cout << "Filling\n";
	ncb->filler(*cb, imageIndex, ncb->params);
	
//std::cout << "Finishing\n";
	if (vkEndCommandBuffer(*cb) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
	
//std::cout << "Closing: " << *cb << "\n";		

	// check if all buffers are now updated
	int updCnt = 0;
	for(int i = 0; i < ncb->inQueue.size(); i++) {
		updCnt += (ncb->inQueue[i] ? 1 : 0);
	}
	if(updCnt == ncb->inQueue.size()) {
		ncb->state = NCBS_IN_USE;
	} else {
		ncb->state = NCBS_IN_CREATION;
	}
}

void BaseProject::updateCommandBuffers(std::vector<VkCommandBuffer> &buffers, int imageIndex) {
	// Creation of newly submitted command buffers
	std::map<int, VkCommandBuffer>sortedBuffer = {};
	
	for(auto &v : namedCommandBuffers) {
//std::cout << "Considering buffer: " << v.first << "\n";
		NamedCommandBuffer *ncb = v.second.current;
		if(ncb->state == NCBS_IN_USE) {
			sortedBuffer[ncb->order] = *ncb->cb[imageIndex];
//			buffers.push_back(*ncb->cb[imageIndex]);
		} else if((ncb->state == NCBS_SUBMITTED) || (ncb->state == NCBS_IN_CREATION)) {
			if(!ncb->inQueue[imageIndex]) {
				// this command buffer needs to be created
				createCommandBuffer(ncb, imageIndex);
			}
			sortedBuffer[ncb->order] = *ncb->cb[imageIndex];
//			buffers.push_back(*ncb->cb[imageIndex]);
		} else {
			std::cout << "Error! state " << ncb->state << " not permitted here!\n";
		}
		
		std::vector<int> toDelete = {};
		for(int j = 0; j < v.second.old.size(); j++) {
			NamedCommandBuffer *ocb = v.second.old[j];
//std::cout << "Found old version for c.b. '" << ocb->name << "'\n";
			if((ocb->state == NCBS_TO_DELETE) || (ocb->state == NCBS_DELETING)) {
				if(ocb->inQueue[imageIndex]) {
					// this command buffer needs to be deleted
					clearNamedCommandBufferForImage(ocb, imageIndex);
				}
			}

			// check if entry can be deleted
			int onCnt = 0;
			for(int i = 0; i < ocb->inQueue.size(); i++) {
				onCnt += (ocb->inQueue[i] ? 1 : 0);
			}
			if(onCnt == 0) {
				ocb->state = NCBS_DETACHED;
				toDelete.push_back(j);
			} else {
				ocb->state = NCBS_DELETING;
			}
		}
//std::cout << toDelete.size() << " old c.b. of '" << v.first << "' can be erased\n";
		for(int j = 0; j < toDelete.size(); j++) {
			clearNamedCommandBuffer(v.second.old[toDelete[j]]);
			v.second.old.erase(v.second.old.begin() + toDelete[j]);
		}
	}
	
	for(auto &m : sortedBuffer) {
		buffers.push_back(m.second);
	}
}

void BaseProject::drawFrame() {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame],
					VK_TRUE, UINT64_MAX);
	
	uint32_t imageIndex;
	
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
			imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex],
						VK_TRUE, UINT64_MAX);
	}
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];
	
	updateUniformBuffer(imageIndex);
	
	std::vector<VkCommandBuffer> buffers = {};
	updateCommandBuffers(buffers, imageIndex);
	
	VkSubmitInfo submitInfo{};
	
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] =
		{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = buffers.size();
	submitInfo.pCommandBuffers = buffers.data();
	VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	
	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
			inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
	
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	
	VkSwapchainKHR swapChains[] = {swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional
	
	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
		framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
	
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void BaseProject::recreateSwapChain() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);
	
	cleanupSwapChain();

	createSwapChain();
	createImageViews();

	createDescriptorPool();			
	pipelinesAndDescriptorSetsInit();

	resetCommandBuffers();
}

void BaseProject::cleanupSwapChain() {
//		clearCommandBuffers();
			
	pipelinesAndDescriptorSetsCleanup();

	for (size_t i = 0; i < swapChainImageViews.size(); i++){
		vkDestroyImageView(device, swapChainImageViews[i], nullptr);
	}
	
	vkDestroySwapchainKHR(device, swapChain, nullptr);

	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}
	
void BaseProject::cleanup() {
	cleanupSwapChain();
		
	localCleanup();
	
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	
	vkDestroyCommandPool(device, commandPool, nullptr);
	
	vkDestroyDevice(device, nullptr);
	
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();
}

void BaseProject::RebuildPipeline() {
	framebufferResized = true;
}

void BaseProject::handleGamePad(int id,  glm::vec3 &m, glm::vec3 &r, bool &fire) {
	const float deadZone = 0.1f;
	
	if(glfwJoystickIsGamepad(id)) {
		GLFWgamepadstate state;
		if (glfwGetGamepadState(id, &state)) {
			if(fabs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]) > deadZone) {
				m.x += state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
			}
			if(fabs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]) > deadZone) {
				m.z += state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
			}
			if(fabs(state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]) > deadZone) {
				m.y -= state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
			}
			if(fabs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]) > deadZone) {
				m.y += state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
			}

			if(fabs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]) > deadZone) {
				r.y += state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
			}
			if(fabs(state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]) > deadZone) {
				r.x += state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
			}
			r.z += state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] ? 1.0f : 0.0f;
			r.z -= state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] ? 1.0f : 0.0f;
			fire = fire | (bool)state.buttons[GLFW_GAMEPAD_BUTTON_A] | (bool)state.buttons[GLFW_GAMEPAD_BUTTON_B];
		}
	}
}
	
void BaseProject::getSixAxis(float &deltaT,
				glm::vec3 &m,
				glm::vec3 &r,
				bool &fire) {
					
	static auto startTime = std::chrono::high_resolution_clock::now();
	static float lastTime = 0.0f;
	
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>
				(currentTime - startTime).count();
	deltaT = time - lastTime;
	lastTime = time;

	static double old_xpos = 0, old_ypos = 0;
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	double m_dx = xpos - old_xpos;
	double m_dy = ypos - old_ypos;
	old_xpos = xpos; old_ypos = ypos;

	const float MOUSE_RES = 10.0f;				
	glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
	if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		r.y = -m_dx / MOUSE_RES;
		r.x = -m_dy / MOUSE_RES;
	}

	if(glfwGetKey(window, GLFW_KEY_LEFT)) {
		r.y = -1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_RIGHT)) {
		r.y = 1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_UP)) {
		r.x = -1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_DOWN)) {
		r.x = 1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_Q)) {
		r.z = 1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_E)) {
		r.z = -1.0f;
	}

	if(glfwGetKey(window, GLFW_KEY_A)) {
		m.x = -1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_D)) {
		m.x = 1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_S)) {
		m.z = 1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_W)) {
		m.z = -1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_R)) {
		m.y = 1.0f;
	}
	if(glfwGetKey(window, GLFW_KEY_F)) {
		m.y = -1.0f;
	}
	
	fire = glfwGetKey(window, GLFW_KEY_SPACE) | (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
	handleGamePad(GLFW_JOYSTICK_1,m,r,fire);
	handleGamePad(GLFW_JOYSTICK_2,m,r,fire);
	handleGamePad(GLFW_JOYSTICK_3,m,r,fire);
	handleGamePad(GLFW_JOYSTICK_4,m,r,fire);
}

void BaseProject::printFloat(const char *Name, float v) {
	std::cout << "float " << Name << " = " << v << ";\n";
}

void BaseProject::printVec2(const char *Name, glm::vec2 v) {
	std::cout << "glm::vec3 " << Name << " = glm::vec3(" << v[0] << ", " << v[1] << ");\n";
}

void BaseProject::printVec3(const char *Name, glm::vec3 v) {
	std::cout << "glm::vec3 " << Name << " = glm::vec3(" << v[0] << ", " << v[1] << ", " << v[2] << ");\n";
}

void BaseProject::printVec4(const char *Name, glm::vec4 v) {
	std::cout << "glm::vec4 " << Name << " = glm::vec4(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ");\n";
}

void BaseProject::printMat3(const char *Name, glm::mat3 v) {
		std::cout << "glm::mat3 " << Name << " = glm::mat3(";
		for(int i = 0; i<9; i++) {
			std::cout << v[i/3][i%3] << ((i<8) ? ", " : ");\n");
		}
}

void BaseProject::printMat4(const char *Name, glm::mat4 v) {
		std::cout << "glm::mat4 " << Name << " = glm::mat4(";
		for(int i = 0; i<16; i++) {
			std::cout << v[i/4][i%4] << ((i<15) ? ", " : ");\n");
		}
}

void BaseProject::printQuat(const char *Name, glm::quat q) {
	std::cout << "glm::vec3 " << Name << " = glm::vec3(" << q[0] << ", " << q[1] << ", " << q[2] << ", " << q[3] << ");\n";
}

inline VkCommandBufferBeginInfo BaseProject::vks_initializers_commandBufferBeginInfo()
{
	VkCommandBufferBeginInfo cmdBufferBeginInfo {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	return cmdBufferBeginInfo;
}

inline VkCommandBufferAllocateInfo BaseProject::vks_initializers_commandBufferAllocateInfo(
	VkCommandPool commandPool, 
	VkCommandBufferLevel level, 
	uint32_t bufferCount)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = level;
	commandBufferAllocateInfo.commandBufferCount = bufferCount;
	return commandBufferAllocateInfo;
}
	
VkCommandBuffer BaseProject::vulkanDevice_createCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
{
	VkResult result;

	VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks_initializers_commandBufferAllocateInfo(pool, level, 1);
	VkCommandBuffer cmdBuffer;
	result = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cmdBuffer);
	if(result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create screenshot!");
	}
	// If requested, also start recording for the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks_initializers_commandBufferBeginInfo();
		result = vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
		if(result != VK_SUCCESS) {
			PrintVkError(result);
			throw std::runtime_error("failed to create screenshot!");
		}
	}
	return cmdBuffer;
}

inline VkFenceCreateInfo BaseProject::vks_initializers_fenceCreateInfo(VkFenceCreateFlags flags)
{
	VkFenceCreateInfo fenceCreateInfo {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = flags;
	return fenceCreateInfo;
}

inline VkSubmitInfo BaseProject::vks_initializers_submitInfo()
{
	VkSubmitInfo submitInfo {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	return submitInfo;
}

void BaseProject::vulkanDevice_flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool freeCmd)
{
	VkResult result;

	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	result = vkEndCommandBuffer(commandBuffer);
	if(result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create screenshot!");
	}

	VkSubmitInfo submitInfo = vks_initializers_submitInfo();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceInfo = vks_initializers_fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	result = vkCreateFence(device, &fenceInfo, nullptr, &fence);
	if(result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create screenshot!");
	}
	// Submit to the queue
	result = vkQueueSubmit(queue, 1, &submitInfo, fence);
	if(result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create screenshot!");
	}
	// Wait for the fence to signal that command buffer has finished executing
	result = vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
	if(result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create screenshot!");
	}
	vkDestroyFence(device, fence, nullptr);
	if (freeCmd)
	{
		clearCommandBuffers();
	}
}	

inline VkMemoryAllocateInfo BaseProject::vks_initializers_memoryAllocateInfo()
{
	VkMemoryAllocateInfo memAllocInfo {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	return memAllocInfo;
}

inline VkImageCreateInfo BaseProject::vks_initializers_imageCreateInfo()
{
	VkImageCreateInfo imageCreateInfo {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	return imageCreateInfo;
}

inline VkImageMemoryBarrier BaseProject::vks_initializers_imageMemoryBarrier()
{
	VkImageMemoryBarrier imageMemoryBarrier {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	return imageMemoryBarrier;
}
	
void BaseProject::vks_tools_insertImageMemoryBarrier(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	VkImageSubresourceRange subresourceRange)
{
	VkImageMemoryBarrier imageMemoryBarrier = vks_initializers_imageMemoryBarrier();
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);
}

void BaseProject::saveScreenshot(const char *filename, int currentBuffer) {
	VkResult result;
	uint32_t width = swapChainExtent.width;
	uint32_t height = swapChainExtent.height;
	
	screenshotSaved = false;
	bool supportsBlit = true;


	// Check blit support for source and destination
	VkFormatProperties formatProps;

	// Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
	vkGetPhysicalDeviceFormatProperties(physicalDevice, swapChainImageFormat, &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	// Check if the device supports blitting to linear images
	vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	// Source for the copy is the last rendered swapchain image
	VkImage srcImage = swapChainImages[currentBuffer];

	// Create the linear tiled destination image to copy to and to read the memory from
	VkImageCreateInfo imageCreateCI(vks_initializers_imageCreateInfo());
	imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
	// Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
	imageCreateCI.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateCI.extent.width = width;
	imageCreateCI.extent.height = height;
	imageCreateCI.extent.depth = 1;
	imageCreateCI.arrayLayers = 1;
	imageCreateCI.mipLevels = 1;
	imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	// Create the image
	VkImage dstImage;
	result = vkCreateImage(device, &imageCreateCI, nullptr, &dstImage);
	if(result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create screenshot!");
	}
	// Create memory to back up the image
	VkMemoryRequirements memRequirements;
	VkMemoryAllocateInfo memAllocInfo(vks_initializers_memoryAllocateInfo());
	VkDeviceMemory dstImageMemory;
	vkGetImageMemoryRequirements(device, dstImage, &memRequirements);
	memAllocInfo.allocationSize = memRequirements.size;
	// Memory must be host visible to copy from
	memAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	result = vkAllocateMemory(device, &memAllocInfo, nullptr, &dstImageMemory);
	if(result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create screenshot!!");
	}
	result = vkBindImageMemory(device, dstImage, dstImageMemory, 0);
	if(result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create screenshot!!");
	}

	// Do the actual blit from the swapchain image to our host visible destination image
	VkCommandBuffer copyCmd = vulkanDevice_createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandPool, true);

	// Transition destination image to transfer destination layout
	vks_tools_insertImageMemoryBarrier(
		copyCmd,
		dstImage,
		0,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	// Transition swapchain image from present to transfer source layout
	vks_tools_insertImageMemoryBarrier(
		copyCmd,
		srcImage,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	// If source and destination support blit we'll blit as this also does automatic format conversion (e.g. from BGR to RGB)
	if (supportsBlit)
	{
		// Define the region to blit (we will blit the whole swapchain image)
		VkOffset3D blitSize;
		blitSize.x = width;
		blitSize.y = height;
		blitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		// Issue the blit command
		vkCmdBlitImage(
			copyCmd,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else
	{
		// Otherwise use image copy (requires us to manually flip components)
		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = width;
		imageCopyRegion.extent.height = height;
		imageCopyRegion.extent.depth = 1;

		// Issue the copy command
		vkCmdCopyImage(
			copyCmd,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion);
	}

	// Transition destination image to general layout, which is the required layout for mapping the image memory later on
	vks_tools_insertImageMemoryBarrier(
		copyCmd,
		dstImage,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	// Transition back the swap chain image after the blit is done
	vks_tools_insertImageMemoryBarrier(
		copyCmd,
		srcImage,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	vulkanDevice_flushCommandBuffer(copyCmd, graphicsQueue, commandPool, true);

	// Get layout of the image (including row pitch)
	VkImageSubresource subResource { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(device, dstImage, &subResource, &subResourceLayout);

	// Map image memory so we can start copying from it
	const char* data;
	vkMapMemory(device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
	data += subResourceLayout.offset;

	char *pixelArray;
	pixelArray = (char *)malloc(width * height * 3);
	// If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
	bool colorSwizzle = false;
	// Check if source is BGR
	// Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
	if (!supportsBlit)
	{
		std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
		colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), swapChainImageFormat) != formatsBGR.end());
	}

	int j = 0;
	for (uint32_t y = 0; y < height; y++)
	{
		unsigned int *row = (unsigned int*)data;
		for (uint32_t x = 0; x < width; x++)
		{
			if (colorSwizzle)
			{
				pixelArray[j++] = ((char*)row)[2];
				pixelArray[j++] = ((char*)row)[1];
				pixelArray[j++] = ((char*)row)[0];
			}
			else
			{
				pixelArray[j++] = ((char*)row)[0];
				pixelArray[j++] = ((char*)row)[1];
				pixelArray[j++] = ((char*)row)[2];
			}
			row++;
		}
		data += subResourceLayout.rowPitch;
	}
	stbi_write_png(filename, width, height, 3, pixelArray, width*3);
	free(pixelArray);

	std::cout << "Screenshot saved to disk" << std::endl;

	// Clean up resources
	vkUnmapMemory(device, dstImageMemory);
	vkFreeMemory(device, dstImageMemory, nullptr);
	vkDestroyImage(device, dstImage, nullptr);

	screenshotSaved = true;
}	


// Helper classes

void VertexDescriptor::init(BaseProject *bp, std::vector<VertexBindingDescriptorElement> B, std::vector<VertexDescriptorElement> E) {
	BP = bp;
	Bindings = B;
	Layout = E;
	
	Position.hasIt = false; Position.offset = 0;
	Pos2D.hasIt = false; Position.offset = 0;
	Normal.hasIt = false; Normal.offset = 0;
	UV.hasIt = false; UV.offset = 0;
	Color.hasIt = false; Color.offset = 0;
	Tangent.hasIt = false; Tangent.offset = 0;
	JointWeight.hasIt = false; JointWeight.offset = 0;
	JointIndex.hasIt = false; JointIndex.offset = 0;
	
	if(B.size() <= 1) {	// for now, read models only with every vertex information in a single binding
		for(int i = 0; i < E.size(); i++) {
			switch(E[i].usage) {
			  case VertexDescriptorElementUsage::POSITION:
			    if(E[i].format == VK_FORMAT_R32G32B32_SFLOAT) {
				  if(E[i].size == sizeof(glm::vec3)) {
					Position.hasIt = true;
					Position.offset = E[i].offset;
				  } else {
					std::cout << "Vertex Position - wrong size\n";
				  }
				} else {
				  std::cout << "Vertex Position - wrong format\n";
				}
			    break;
			  case VertexDescriptorElementUsage::POS2D:
			    if(E[i].format == VK_FORMAT_R32G32_SFLOAT) {
				  if(E[i].size == sizeof(glm::vec2)) {
					Pos2D.hasIt = true;
					Pos2D.offset = E[i].offset;
				  } else {
					std::cout << "Vertex Position 2D - wrong size\n";
				  }
				} else {
				  std::cout << "Vertex Position 2D - wrong format\n";
				}
			    break;
			  case VertexDescriptorElementUsage::NORMAL:
			    if(E[i].format == VK_FORMAT_R32G32B32_SFLOAT) {
				  if(E[i].size == sizeof(glm::vec3)) {
					Normal.hasIt = true;
					Normal.offset = E[i].offset;
				  } else {
					std::cout << "Vertex Normal - wrong size\n";
				  }
				} else {
				  std::cout << "Vertex Normal - wrong format\n";
				}
			    break;
			  case VertexDescriptorElementUsage::UV:
			    if(E[i].format == VK_FORMAT_R32G32_SFLOAT) {
				  if(E[i].size == sizeof(glm::vec2)) {
					UV.hasIt = true;
					UV.offset = E[i].offset;
				  } else {
					std::cout << "Vertex UV - wrong size\n";
				  }
				} else {
				  std::cout << "Vertex UV - wrong format\n";
				}
			    break;
			  case VertexDescriptorElementUsage::COLOR:
			    if(E[i].format == VK_FORMAT_R32G32B32_SFLOAT) {
				  if(E[i].size == sizeof(glm::vec3)) {
					Color.hasIt = true;
					Color.offset = E[i].offset;
				  } else {
					std::cout << "Vertex Color - wrong size\n";
				  }
				} else {
				  std::cout << "Vertex Color - wrong format\n";
				}
			    break;
			  case VertexDescriptorElementUsage::TANGENT:
			    if(E[i].format == VK_FORMAT_R32G32B32A32_SFLOAT) {
				  if(E[i].size == sizeof(glm::vec4)) {
					Tangent.hasIt = true;
					Tangent.offset = E[i].offset;
				  } else {
					std::cout << "Vertex Tangent - wrong size\n";
				  }
				} else {
				  std::cout << "Vertex Tangent - wrong format\n";
				}
			    break;
				case VertexDescriptorElementUsage::JOINTWEIGHT:
					if(E[i].format == VK_FORMAT_R32G32B32A32_SFLOAT) {
						if(E[i].size == sizeof(glm::vec4)) {
							JointWeight.hasIt = true;
							JointWeight.offset = E[i].offset;
						} else {
							std::cout << "Vertex Joint Weight - wrong size\n";
						}
					} else {
						std::cout << "Vertex Joint Weight - wrong format\n";
					}
				break;
				case VertexDescriptorElementUsage::JOINTINDEX:
					if(E[i].format == VK_FORMAT_R32G32B32A32_UINT) {
						if(E[i].size == sizeof(glm::uvec4)) {
							JointIndex.hasIt = true;
							JointIndex.offset = E[i].offset;
						} else {
							std::cout << "Vertex Joint Index - wrong size\n";
						}
					} else {
						std::cout << "Vertex Joint Index - wrong format\n";
					}
				break;
			  default:
			    break;
			}
		}
	} else {
		throw std::runtime_error("Vertex format with more than one binding is not supported yet\n");
	}
}

void VertexDescriptor::cleanup() {
}

std::vector<VkVertexInputBindingDescription> VertexDescriptor::getBindingDescription() {
	std::vector<VkVertexInputBindingDescription>bindingDescription{};
	bindingDescription.resize(Bindings.size());
//std::cout << "Binding Size: " << Bindings.size() << "\n";
	for(int i = 0; i < Bindings.size(); i++) {
		bindingDescription[i].binding = Bindings[i].binding;
		bindingDescription[i].stride = Bindings[i].stride;
		bindingDescription[i].inputRate = Bindings[i].inputRate;
	}
	return bindingDescription;
}
	
std::vector<VkVertexInputAttributeDescription> VertexDescriptor::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};	
	attributeDescriptions.resize(Layout.size());
	for(int i = 0; i < Layout.size(); i++) {
		attributeDescriptions[i].binding = Layout[i].binding;
		attributeDescriptions[i].location = Layout[i].location;
		attributeDescriptions[i].format = Layout[i].format;
		attributeDescriptions[i].offset = Layout[i].offset;
	}
					
	return attributeDescriptions;
}










void AssetFile::init(std::string file, ModelType MT) {
	type = MT;
	
	if(type == OBJ) {
		initOBJ(file);
	}
	if(type == GLTF) {
		initGLTF(file);
	}
}


void AssetFile::initOBJ(std::string file) {
	std::string warn, err;
	char *matpath;

/*	int lastBackSlash = -1;
	for(int i = 0; i < file.length(); i++) {
		if((file[i] == '/') || (file[i] == '\\')) {lastBackSlash = i;}
	}
	if(lastBackSlash > 0) {
		matpath = (char *)malloc(lastBackSlash+1);
		memcpy(matpath, file.c_str(), lastBackSlash);
		matpath[lastBackSlash] = '\0';
	} else {*/
		matpath = nullptr;
//	}
	
	std::cout << "Loading Asset File: " << file << "[OBJ] - mat. path: " << (matpath == nullptr ? "<<NOPATH>>" : matpath) << "\n";	
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
						  file.c_str(), matpath)) {
		throw std::runtime_error(warn + err);
	}
/*	std::cout << "Asset has: " << materials.size() << " materials\n";*/
	for (const auto& shape : shapes) {
		std::cout << " Name:" << shape.name << "\n";
		OBJmeshes[shape.name] = &shape;
/*		for (const auto& id : shape.mesh.material_ids) {
			std::cout << id << "\t";
		}
		std::cout << "\n";*/
	}
	
	if(matpath != nullptr) {
		free(matpath);
	}
}

void AssetFile::initGLTF(std::string file) {
	// GLTF assets stuff
	tinygltf::TinyGLTF loader;
	std::string warn, err;

	std::cout << "Loading Asset File: " << file << "[GLTF]\n";	
	if (!loader.LoadASCIIFromFile(&model, &warn, &err, 
					file.c_str())) {
		throw std::runtime_error(warn + err);
	}


	for (const auto& mesh :  model.meshes) {
		std::cout << " Name:" << mesh.name << " Primitives: " << mesh.primitives.size() << "\n";
		int PrimCount = 0;
		for (const auto& primitive :  mesh.primitives) {
			if (primitive.indices < 0) {
				continue;
			} else {
				std::cout << "Primitive: " << PrimCount << ", Material: " <<
					primitive.material << " -> " << model.materials[primitive.material].name <<"\n";
			}
			GLTFmeshes[mesh.name].push_back(&primitive);
			PrimCount++;
		}
	}

	int cnt = 0;
	for (const auto& node :  model.nodes) {
		std::cout << "Node: " << cnt ++ << " Mesh: " << node.mesh << " Name:" << node.name << "\n";
		GLTFnodes[node.name] = &node;
	}
}

void AssetFile::cleanup() {
}
	




void Model::makeOBJMesh(const tinyobj::shape_t *M, const tinyobj::attrib_t *A) {
	int mainStride = VD->Bindings[0].stride;
	int newId = 0;
	for (const auto& index : M->mesh.indices) {
		std::vector<unsigned char> vertex(mainStride, 0);
		glm::vec3 pos = {
			A->vertices[3 * index.vertex_index + 0],
			A->vertices[3 * index.vertex_index + 1],
			A->vertices[3 * index.vertex_index + 2]
		};
		if(VD->Position.hasIt) {
			glm::vec3 *o = (glm::vec3 *)((char*)(&vertex[0]) + VD->Position.offset);
			*o = pos;
		}
		
		glm::vec3 color = {
			A->colors[3 * index.vertex_index + 0],
			A->colors[3 * index.vertex_index + 1],
			A->colors[3 * index.vertex_index + 2]
		};
		if(VD->Color.hasIt) {
			glm::vec3 *o = (glm::vec3 *)((char*)(&vertex[0]) + VD->Color.offset);
			*o = color;
		}
		
		glm::vec2 texCoord = {
			A->texcoords[2 * index.texcoord_index + 0],
			1 - A->texcoords[2 * index.texcoord_index + 1] 
		};
		if(VD->UV.hasIt) {
			glm::vec2 *o = (glm::vec2 *)((char*)(&vertex[0]) + VD->UV.offset);
			*o = texCoord;
		}

		glm::vec3 norm = {
			A->normals[3 * index.normal_index + 0],
			A->normals[3 * index.normal_index + 1],
			A->normals[3 * index.normal_index + 2]
		};
		if(VD->Normal.hasIt) {
			glm::vec3 *o = (glm::vec3 *)((char*)(&vertex[0]) + VD->Normal.offset);
			*o = norm;
		}
		
		vertices.insert(vertices.end(), vertex.begin(), vertex.end());
//			indices.push_back((vertices.size()/mainStride)-1);
		indices.push_back(newId++);
	}
}

void Model::loadModelOBJ(std::string file) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	
	std::cout << "Loading : " << file << "[OBJ]\n";	
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
						  file.c_str())) {
		throw std::runtime_error(warn + err);
	}
	
//	std::cout << "Building\n";	
//	std::cout << "Position " << VD->Position.hasIt << "," << VD->Position.offset << "\n";	
//	std::cout << "UV " << VD->UV.hasIt << "," << VD->UV.offset << "\n";	
//	std::cout << "Normal " << VD->Normal.hasIt << "," << VD->Normal.offset << "\n";
	for (const auto& shape : shapes) {
		makeOBJMesh(&shape, &attrib);
	}
	std::cout << "[OBJ] Vertices: "<< (vertices.size()/VD->Bindings[0].stride);
	std::cout << " Indices: "<< indices.size() << "\n";
	
}

void Model::makeGLTFMesh(tinygltf::Model *M, const tinygltf::Primitive *Prm) {
	int mainStride = VD->Bindings[0].stride;

	const float *bufferPos = nullptr;
	const float *bufferNormals = nullptr;
	const float *bufferTangents = nullptr;
	const float *bufferTexCoords = nullptr;
	const glm::u8 *bufferJointIndex = nullptr;
	const float *bufferJointWeight = nullptr;
	
	bool meshHasPos = false;
	bool meshHasNorm = false;
	bool meshHasTan = false;
	bool meshHasUV = false;
	bool meshHasJointIndex = false;
	bool meshHasJointWeight = false;
	
	int cntPos = 0;
	int cntNorm = 0;
	int cntTan = 0;
	int cntUV = 0;
	int cntJointIndex = 0;
	int cntJointWeight = 0;
	int cntTot = 0;

	auto pIt = Prm->attributes.find("POSITION");
	if(pIt != Prm->attributes.end()) {
		const tinygltf::Accessor &posAccessor = M->accessors[pIt->second];
		const tinygltf::BufferView &posView = M->bufferViews[posAccessor.bufferView];
		bufferPos = reinterpret_cast<const float *>(&(M->buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
		meshHasPos = true;
		cntPos = posAccessor.count;
		if(cntPos > cntTot) cntTot = cntPos;
	} else {
		if(VD->Position.hasIt) {
			std::cout << "Warning: vertex layout has position, but file hasn't\n";
		}
	}
	
	auto nIt = Prm->attributes.find("NORMAL");
	if(nIt != Prm->attributes.end()) {
		const tinygltf::Accessor &normAccessor = M->accessors[nIt->second];
		const tinygltf::BufferView &normView = M->bufferViews[normAccessor.bufferView];
		bufferNormals = reinterpret_cast<const float *>(&(M->buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
		meshHasNorm = true;
		cntNorm = normAccessor.count;
		if(cntNorm > cntTot) cntTot = cntNorm;
	} else {
		if(VD->Normal.hasIt) {
			std::cout << "Warning: vertex layout has normal, but file hasn't\n";
		}
	}

	auto tIt = Prm->attributes.find("TANGENT");
	if(tIt != Prm->attributes.end()) {
		const tinygltf::Accessor &tanAccessor = M->accessors[tIt->second];
		const tinygltf::BufferView &tanView = M->bufferViews[tanAccessor.bufferView];
		bufferTangents = reinterpret_cast<const float *>(&(M->buffers[tanView.buffer].data[tanAccessor.byteOffset + tanView.byteOffset]));
		meshHasTan = true;
		cntTan = tanAccessor.count;
		if(cntTan > cntTot) cntTot = cntTan;
	} else {
		if(VD->Tangent.hasIt) {
			std::cout << "Warning: vertex layout has tangent, but file hasn't\n";
		}
	}

	auto uIt = Prm->attributes.find("TEXCOORD_0");
	if(uIt != Prm->attributes.end()) {
		const tinygltf::Accessor &uvAccessor = M->accessors[uIt->second];
		const tinygltf::BufferView &uvView = M->bufferViews[uvAccessor.bufferView];
		bufferTexCoords = reinterpret_cast<const float *>(&(M->buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
		meshHasUV = true;
		cntUV = uvAccessor.count;
		if(cntUV > cntTot) cntTot = cntUV;
	} else {
		if(VD->UV.hasIt) {
			std::cout << "Warning: vertex layout has UV, but file hasn't\n";
		}
	}

	auto iIt = Prm->attributes.find("JOINTS_0");
	if(iIt != Prm->attributes.end()) {
		const tinygltf::Accessor &jointAccessor = M->accessors[iIt->second];
		const tinygltf::BufferView &jointView = M->bufferViews[jointAccessor.bufferView];
		bufferJointIndex = reinterpret_cast<const glm::u8 *>(&(M->buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
		meshHasJointIndex = true;
		cntJointIndex = jointAccessor.count;
		if(cntJointIndex > cntTot) cntTot = cntJointIndex;
	} else {
		if(VD->JointIndex.hasIt) {
			std::cout << "Warning: vertex layout has Joint, but file hasn't\n";
		}
	}
	auto wIt = Prm->attributes.find("WEIGHTS_0");
	if(wIt != Prm->attributes.end()) {
		const tinygltf::Accessor &weightsAccessor = M->accessors[wIt->second];
		const tinygltf::BufferView &weightsView = M->bufferViews[weightsAccessor.bufferView];
		bufferJointWeight = reinterpret_cast<const float *>(&(M->buffers[weightsView.buffer].data[weightsAccessor.byteOffset + weightsView.byteOffset]));
		meshHasJointWeight = true;
		cntJointWeight = weightsAccessor.count;
		if(cntJointWeight > cntTot) cntTot = cntJointWeight;
	} else {
		if(VD->JointWeight.hasIt) {
			std::cout << "Warning: vertex layout has Weights, but file hasn't\n";
		}
	}

//std::cout << "making vertex array. Stride:" << mainStride << "\n";
//	std::unordered_map<int, bool> usedIndices;

	for(int i = 0; i < cntTot; i++) {
		std::vector<unsigned char> vertex(mainStride, 0);
//std::cout << vertices.size() << "," << vertex.size() << "," << &vertex << " " << &vertex[0] << " ";
//std::cout << i << "\n";
		
		if((i < cntPos) && meshHasPos && VD->Position.hasIt) {
			glm::vec3 pos = {
				bufferPos[3 * i + 0],
				bufferPos[3 * i + 1],
				bufferPos[3 * i + 2]
			};
//std::cout << "Pos: " <<	VD->Position.offset << "\n";
			glm::vec3 *o = (glm::vec3 *)((char*)(&vertex[0]) + VD->Position.offset);
//std::cout << "at: " << o << "\n";
			*o = pos;
//std::cout << "Copied: " << o->x << "\n";
		}
		if((i < cntNorm) && meshHasNorm && VD->Normal.hasIt) {
			glm::vec3 normal = {
				bufferNormals[3 * i + 0],
				bufferNormals[3 * i + 1],
				bufferNormals[3 * i + 2]
			};
//std::cout << "Nor: " <<	VD->Normal.offset << "\n";
			glm::vec3 *o = (glm::vec3 *)((char*)(&vertex[0]) + VD->Normal.offset);
			*o = normal;
		}

		if((i < cntTan) && meshHasTan && VD->Tangent.hasIt) {
			glm::vec4 tangent = {
				bufferTangents[4 * i + 0],
				bufferTangents[4 * i + 1],
				bufferTangents[4 * i + 2],
				bufferTangents[4 * i + 3]
			};
//std::cout << "Tan: " <<	VD->Tangent.offset << "\n";
			glm::vec4 *o = (glm::vec4 *)((char*)(&vertex[0]) + VD->Tangent.offset);
			*o = tangent;
		}
		
		if((i < cntUV) && meshHasUV && VD->UV.hasIt) {
			glm::vec2 texCoord = {
				bufferTexCoords[2 * i + 0],
				bufferTexCoords[2 * i + 1] 
			};
//std::cout << "UV : " <<	VD->UV.offset << "\n";
			glm::vec2 *o = (glm::vec2 *)((char*)(&vertex[0]) + VD->UV.offset);
			*o = texCoord;
		}


		if((i < cntJointIndex) && meshHasJointIndex && VD->JointIndex.hasIt) {
			glm::uvec4 jointIndex = {
				(glm::uint)bufferJointIndex[4 * i + 0],
				(glm::uint)bufferJointIndex[4 * i + 1],
				(glm::uint)bufferJointIndex[4 * i + 2],
				(glm::uint)bufferJointIndex[4 * i + 3]
			};
//std::cout << jointIndex.x << " " << jointIndex.y << " " << jointIndex.z << " " << jointIndex.w << "\t\t";

//usedIndices[jointIndex.x] = true;
//usedIndices[jointIndex.y] = true;
//usedIndices[jointIndex.z] = true;
//usedIndices[jointIndex.w] = true;

			glm::uvec4 *o = (glm::uvec4 *)((char*)(&vertex[0]) + VD->JointIndex.offset);
			*o = jointIndex;
		}

		if((i < cntJointWeight) && meshHasJointWeight && VD->JointWeight.hasIt) {
			glm::vec4 jointWeight = {
				bufferJointWeight[4 * i + 0],
				bufferJointWeight[4 * i + 1],
				bufferJointWeight[4 * i + 2],
				bufferJointWeight[4 * i + 3]
			};
//std::cout << bufferJointWeight[4 * i + 0] << " " << bufferJointWeight[4 * i + 1] << " " << bufferJointWeight[4 * i + 2] << " " << 
//				bufferJointWeight[4 * i + 3] << "\n";

			glm::vec4 *o = (glm::vec4 *)((char*)(&vertex[0]) + VD->JointWeight.offset);
			*o = jointWeight;
		}

//std::cout << vertices.size() << "," << vertex.size() << " Inserting\n";
		vertices.insert(vertices.end(), vertex.begin(), vertex.end());
//std::cout << vertices.size() << " Inserted\n";
	} 

//std::cout << "Used indices: " << usedIndices.size() << "\n";
//for(auto kkk: usedIndices) {
//	std::cout << kkk.first << ", ";
//}
//std::cout << "\n";

	const tinygltf::Accessor &accessor = M->accessors[Prm->indices];
	const tinygltf::BufferView &bufferView = M->bufferViews[accessor.bufferView];
	const tinygltf::Buffer &buffer = M->buffers[bufferView.buffer];
	
	switch(accessor.componentType) {
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
			{
				const uint16_t *bufferIndex = reinterpret_cast<const uint16_t *>(&(buffer.data[accessor.byteOffset + bufferView.byteOffset]));
				for(int i = 0; i < accessor.count; i++) {
					indices.push_back(bufferIndex[i]);
				}
			}
			break;
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
			{
				const uint32_t *bufferIndex = reinterpret_cast<const uint32_t *>(&(buffer.data[accessor.byteOffset + bufferView.byteOffset]));
				for(int i = 0; i < accessor.count; i++) {
					indices.push_back(bufferIndex[i]);
				}
			}
			break;
		default:
			std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
			throw std::runtime_error("Error loading GLTF component");
	}			
}


void Model::getGLTFnodeTransforms(const tinygltf::Node *N, 	glm::vec3 &T, glm::vec3 &S, glm::quat &Q) {
	if(N->translation.size() > 0) {
//std::cout << "node " << i << " has T\n";
		T = glm::vec3(N->translation[0],
					  N->translation[1],
					  N->translation[2]);
	} else {
		T = glm::vec3(0);
	}
	if(N->rotation.size() > 0) {
//std::cout << "node " << i << " has Q\n";
		Q = glm::quat(N->rotation[3],
					  N->rotation[0],
					  N->rotation[1],
					  N->rotation[2]);
	} else {
		Q = glm::quat(1.0f,0.0f,0.0f,0.0f);
	}
	if(N->scale.size() > 0) {
//std::cout << "node " << i << " has S\n";
		S = glm::vec3(N->scale[0],
					  N->scale[1],
					  N->scale[2]);
	} else {
		S = glm::vec3(1);
	}
}


void Model::makeGLTFwm(const tinygltf::Node *N) {
	glm::vec3 T;
	glm::vec3 S;
	glm::quat Q;
	
	getGLTFnodeTransforms(N, T, S, Q);
//printVec3("T",T);
//printQuat("Q",Q);
//printVec3("S",S);
	Wm = glm::translate(glm::mat4(1), T) *
			 glm::mat4(Q) *
			 glm::scale(glm::mat4(1), S);
}

void Model::loadModelGLTF(std::string file, bool encoded) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string warn, err;
	
	std::cout << "Loading : " << file << (encoded ? "[MGCG]" : "[GLTF]") << "\n";	
	if(encoded) {
		auto modelString = readFile(file);
		
		const std::vector<unsigned char> key = plusaes::key_from_string(&"CG2023SkelKey128"); // 16-char = 128-bit
		const unsigned char iv[16] = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		};

		// decrypt
		unsigned long padded_size = 0;
		std::vector<unsigned char> decrypted(modelString.size());

		plusaes::decrypt_cbc((unsigned char*)modelString.data(), modelString.size(), &key[0], key.size(), &iv, &decrypted[0], decrypted.size(), &padded_size);

		int size = 0;
		void *decomp;
		
		sscanf(reinterpret_cast<char *const>(&decrypted[0]), "%d", &size);
//std::cout << decrypted.size() << ", decomp: " << size << "\n";
//for(int i=0;i<32;i++) {
//	std::cout << (int)decrypted[i] << "\n";
//}

		decomp = calloc(size, 1);
		int n = sinflate(decomp, (int)size, &decrypted[16], decrypted.size()-16);
		
		if (!loader.LoadASCIIFromString(&model, &warn, &err, 
						reinterpret_cast<const char *>(decomp), size, "/")) {
			throw std::runtime_error(warn + err);
		}
	} else {
		if (!loader.LoadASCIIFromFile(&model, &warn, &err, 
						file.c_str())) {
			throw std::runtime_error(warn + err);
		}
	}

	for (const auto& mesh :  model.meshes) {
		std::cout << "Primitives: " << mesh.primitives.size() << "\n";
		for (const auto& primitive :  mesh.primitives) {
			if (primitive.indices < 0) {
				continue;
			}

			makeGLTFMesh(&model, &primitive);
		}
	}

	std::cout << (encoded ? "[MGCG]" : "[GLTF]") << " Vertices: " << (vertices.size()/VD->Bindings[0].stride)
			  << " Indices: " << indices.size() << "\n";
/*
std::cout << model.nodes[0].translation.size() << "\n";
std::cout << model.nodes[0].rotation.size() << "\n";
std::cout << model.nodes[0].scale.size() << "\n";
*/
	makeGLTFwm(&model.nodes[0]);
}

void Model::createVertexBuffer() {
//	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	VkDeviceSize bufferSize = vertices.size();

	BP->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						vertexBuffer, vertexBufferMemory);

	void* data;
	vkMapMemory(BP->device, vertexBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t) bufferSize);
	vkUnmapMemory(BP->device, vertexBufferMemory);			
}

void Model::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	BP->createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
							 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							 indexBuffer, indexBufferMemory);

	void* data;
	vkMapMemory(BP->device, indexBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t) bufferSize);
	vkUnmapMemory(BP->device, indexBufferMemory);
}

void Model::initMesh(BaseProject *bp, VertexDescriptor *vd, bool printDebug) {
	BP = bp;
	VD = vd;
	int mainStride = VD->Bindings[0].stride;
	if(printDebug) {
		std::cout << "[Manual] Vertices: " << (vertices.size()/mainStride)
				  << " Indices: " << indices.size() << "\n";
	}
	createVertexBuffer();
	createIndexBuffer();
	Wm = glm::mat4(1);
}

void Model::init(BaseProject *bp, VertexDescriptor *vd, std::string file, ModelType MT) {
	BP = bp;
	VD = vd;
	Wm = glm::mat4(1);

	if(MT == OBJ) {
		loadModelOBJ(file);
	} else if(MT == GLTF) {
		loadModelGLTF(file, false);
	} else if(MT == MGCG) {
		loadModelGLTF(file, true);
	}
	
	createVertexBuffer();
	createIndexBuffer();
}

void Model::initFromAsset(BaseProject *bp, VertexDescriptor *vd, AssetFile *AF, std::string AN, int Mid, std::string NN) {
	BP = bp;
	VD = vd;
	Wm = glm::mat4(1);

	switch(AF->type) {
	  case GLTF:
   	    {
   	      const tinygltf::Primitive *Prm;
   		  auto el = AF->GLTFmeshes.find(AN);
   		  if(el != AF->GLTFmeshes.end()) {
   		  	std::vector<const tinygltf::Primitive *> P = el->second;
   		  	if((Mid >= 0) && (Mid < P.size())) {
   		  		makeGLTFMesh(&AF->model, P[Mid]);
   		  	} else {
   		  		std::cout << "Asset >" << AN << "< does not have component: " << Mid << "\n";
   		  	}
   		  } else {
   		  	std::cout << "Asset does not contain Mesh: " << AN << "\n";
   		  }
		  if(NN != "") {
			  auto nel = AF->GLTFnodes.find(NN);
			  if(nel != AF->GLTFnodes.end()) {
				  makeGLTFwm(nel->second);
			  } else {
				std::cout << "Asset does not contain Node: " << NN << "\n";
			  }
		  }
   	    }
		break;
	  case OBJ:
   	    {
   	      const tinyobj::shape_t *Prm;
   		  auto el = AF->OBJmeshes.find(AN);
   		  if(el != AF->OBJmeshes.end()) {
   		  	Prm = el->second;
   		  	if(Mid != 0) {
   		  		std::cout << "OBJ assets can only be single material\n";
   		  	} else {
   		  		makeOBJMesh(Prm, &AF->attrib);
   		  	}
   		  } else {
   		  	std::cout << "Asset does not contain Mesh: " << AN << "\n";
   		  }
   	    }
		break;
	  default:
	    std::cout << "Unknown asset file type: " << AF->type << "\n";
	    break;
	}

	createVertexBuffer();
	createIndexBuffer();
}

void Model::cleanup() {
   	vkDestroyBuffer(BP->device, indexBuffer, nullptr);
   	vkFreeMemory(BP->device, indexBufferMemory, nullptr);
	vkDestroyBuffer(BP->device, vertexBuffer, nullptr);
   	vkFreeMemory(BP->device, vertexBufferMemory, nullptr);
}

void Model::bind(VkCommandBuffer commandBuffer) {
	VkBuffer vertexBuffers[] = {vertexBuffer};
	// property .vertexBuffer of models, contains the VkBuffer handle to its vertex buffer
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	// property .indexBuffer of models, contains the VkBuffer handle to its index buffer
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0,
							VK_INDEX_TYPE_UINT32);
}






void Texture::createTextureImage(std::vector<std::string>files, VkFormat Fmt) {
	int texWidth, texHeight, texChannels;
	int curWidth = -1, curHeight = -1, curChannels = -1;
	stbi_uc* pixels[maxImgs];
	
	for(int i = 0; i < imgs; i++) {
	 	pixels[i] = stbi_load(files[i].c_str(), &texWidth, &texHeight,
						&texChannels, STBI_rgb_alpha);
		if (!pixels[i]) {
			std::cout << "Not found: " << files[i] << "\n";
			throw std::runtime_error("failed to load texture image!");
		}
		std::cout << "[" << i << "]" << files[i] << " -> size: " << texWidth
				  << "x" << texHeight << ", ch: " << texChannels <<"\n";
				  
		if(i == 0) {
			curWidth = texWidth;
			curHeight = texHeight;
			curChannels = texChannels;
		} else {
			if((curWidth != texWidth) ||
			   (curHeight != texHeight) ||
			   (curChannels != texChannels)) {
				throw std::runtime_error("multi texture images must be all of the same size!");
			}
		}
	}
	
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	VkDeviceSize totalImageSize = texWidth * texHeight * 4 * imgs;
	mipLevels = static_cast<uint32_t>(std::floor(
					std::log2(std::max(texWidth, texHeight)))) + 1;
	
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	 
	BP->createBuffer(totalImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	  						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	  						VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	  						stagingBuffer, stagingBufferMemory);
	void* data;
	vkMapMemory(BP->device, stagingBufferMemory, 0, totalImageSize, 0, &data);
	for(int i = 0; i < imgs; i++) {
		memcpy(static_cast<char *>(data) + imageSize * i, pixels[i], static_cast<size_t>(imageSize));
		stbi_image_free(pixels[i]);
	}
	vkUnmapMemory(BP->device, stagingBufferMemory);
	
	
	BP->createImage(texWidth, texHeight, mipLevels, imgs, VK_SAMPLE_COUNT_1_BIT, Fmt,
				VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				imgs == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage,
				textureImageMemory);
				
	BP->transitionImageLayout(textureImage, Fmt,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, imgs);
	BP->copyBufferToImage(stagingBuffer, textureImage,
			static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), imgs);

	BP->generateMipmaps(textureImage, Fmt,
					texWidth, texHeight, mipLevels, imgs);

	vkDestroyBuffer(BP->device, stagingBuffer, nullptr);
	vkFreeMemory(BP->device, stagingBufferMemory, nullptr);
}

void Texture::createTextureImageView(VkFormat Fmt) {
	textureImageView = BP->createImageView(textureImage,
									   Fmt,
									   VK_IMAGE_ASPECT_COLOR_BIT,
									   mipLevels,
									   imgs == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
									   imgs);
}
	
void Texture::createTextureSampler(VkFilter magFilter,
							 VkFilter minFilter,
							 VkSamplerAddressMode addressModeU,
							 VkSamplerAddressMode addressModeV,
							 VkSamplerMipmapMode mipmapMode,
							 VkBool32 anisotropyEnable,
							 float maxAnisotropy,
							 float maxLod) {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = magFilter;
	samplerInfo.minFilter = minFilter;
	samplerInfo.addressModeU = addressModeU;
	samplerInfo.addressModeV = addressModeV;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = anisotropyEnable;
	samplerInfo.maxAnisotropy = maxAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = mipmapMode;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = ((maxLod == -1) ? static_cast<float>(mipLevels) : maxLod);
	
	VkResult result = vkCreateSampler(BP->device, &samplerInfo, nullptr,
									  &textureSampler);
	if (result != VK_SUCCESS) {
	 	PrintVkError(result);
	 	throw std::runtime_error("failed to create texture sampler!");
	}
}
	


void Texture::init(BaseProject *bp, std::string file, VkFormat Fmt, bool initSampler) {
	BP = bp;
	imgs = 1;
	createTextureImage({file}, Fmt);
	createTextureImageView(Fmt);
	if(initSampler) {
		createTextureSampler();
	}
}


void Texture::initCubic(BaseProject *bp, std::vector<std::string>files, VkFormat Fmt) {
	if(files.size() != 6) {
		std::cout << "\nError! Cube map without 6 files - " << files.size() << "\n";
		exit(0);
	}
	BP = bp;
	imgs = 6;
	createTextureImage(files, Fmt);
	createTextureImageView(Fmt);
	createTextureSampler();
}

VkDescriptorImageInfo Texture::getViewAndSampler() {
	return {textureSampler, textureImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
}

void Texture::cleanup() {
   	vkDestroySampler(BP->device, textureSampler, nullptr);
   	vkDestroyImageView(BP->device, textureImageView, nullptr);
	vkDestroyImage(BP->device, textureImage, nullptr);
	vkFreeMemory(BP->device, textureImageMemory, nullptr);
}





void FrameBufferAttachment::createTextureSampler(
VkFilter magFilter,
							 VkFilter minFilter,
							 VkSamplerAddressMode addressModeU,
							 VkSamplerAddressMode addressModeV,
							 VkSamplerMipmapMode mipmapMode,
							 VkBool32 anisotropyEnable,
							 float maxAnisotropy,
							 float maxLod
							) {
	BaseProject *BP = RP->BP;

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = magFilter;
	samplerInfo.minFilter = minFilter;
	samplerInfo.addressModeU = addressModeU;
	samplerInfo.addressModeV = addressModeV;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = anisotropyEnable;
	samplerInfo.maxAnisotropy = maxAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = mipmapMode;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	
	VkResult result = vkCreateSampler(BP->device, &samplerInfo, nullptr,
									  &sampler);
	if (result != VK_SUCCESS) {
	 	PrintVkError(result);
	 	throw std::runtime_error("failed to create texture sampler!");
	}
}

void FrameBufferAttachment::init(RenderPass *rp, AttachmentProperties *p, bool initSampler) {
	RP = rp;
	properties = p;
	if(initSampler) {
		createTextureSampler();
		freeSampler = true;
	} else {
		freeSampler = false;
	}
}

void FrameBufferAttachment::createResources() {
	BaseProject *BP = RP->BP;

	VkFormat format = properties->format;
	int usage = properties->usage;
	int aspect = properties->aspect;
	VkSampleCountFlagBits samples = properties->samples;
	bool doDepthTransition = properties->doDepthTransition;
	
	BP->createImage(RP->width, RP->height, 1, 1,
				samples, format, VK_IMAGE_TILING_OPTIMAL,
				usage, 0, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image, mem);
	view = BP->createImageView(image, format,
								aspect, 1,
								VK_IMAGE_VIEW_TYPE_2D, 1);
								
	if(doDepthTransition) {
		BP->transitionImageLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);
	}
}

void FrameBufferAttachment::createDescriptionAndReference(int num) {
	descr.format         = properties->format;
	descr.samples        = properties->samples;    
	descr.loadOp         = properties->loadOp;
	descr.storeOp        = properties->storeOp;
	descr.stencilLoadOp  = properties->stencilLoadOp;
	descr.stencilStoreOp = properties->stencilStoreOp;
	descr.initialLayout  = properties->initialLayout;
	descr.finalLayout    = properties->finalLayout;

	ref.attachment = num;
	ref.layout = properties->refLayout;
}

VkImageView FrameBufferAttachment::getView(int currentImage) {
	BaseProject *BP = RP->BP;
	
	if(properties->swapChain) {
		return BP->swapChainImageViews[currentImage];
	} else {
		return view;
	}
}

VkDescriptorImageInfo FrameBufferAttachment::getViewAndSampler() {
	return {sampler, view, properties->finalLayout};
}



void FrameBufferAttachment::cleanup() {
	BaseProject *BP = RP->BP;

//std::cout << "Cleaning up render pass attchment " << properties->swapChain << " " << properties->type << " " << properties->usage << "\n";

	if(!properties->swapChain) {
		vkDestroyImageView(BP->device, view, nullptr);
		vkDestroyImage(BP->device, image, nullptr);
		vkFreeMemory(BP->device, mem, nullptr);
	}
}

void FrameBufferAttachment::destroy() {
	BaseProject *BP = RP->BP;
	if(freeSampler) {
		vkDestroySampler(BP->device, sampler, nullptr);
	}
}





void RenderPass::init(BaseProject *bp, int w, int h, int _count, std::vector <AttachmentProperties> *p, std::vector<VkSubpassDependency> *d, bool initSampler) {
	BP = bp;
	width = (w > 0 ? w : BP->swapChainExtent.width);
	height = (h > 0 ? h : BP->swapChainExtent.height);
	count = (_count > 0 ? _count : BP->swapChainImageViews.size());

	if(p == nullptr) {
		properties = *getStandardAttchmentsProperties(AT_SURFACE_AA_DEPTH, BP);
	} else {
		properties = *p;
	}

	if(d == nullptr) {
		dependencies = *getStandardDependencies(ATDEP_SURFACE_ONLY);
	} else {
		dependencies = *d;
	}

	attachments.resize(properties.size());
	for(int i = 0; i < attachments.size(); i++) {
		attachments[i].init(this, &properties[i], initSampler);
	}
}

void RenderPass::createRenderPass() {
	colorAttchementsCount = 0;
	firstColorAttIdx = -1;
	depthAttIdx = -1;
	resolveAttIdx = -1;
	
	for(int j = 0; j < attachments.size(); j++) {
		attachments[j].createDescriptionAndReference(j);
		switch(attachments[j].properties->type) {
		  case COLOR_AT:
		    colorAttchementsCount++;
			if(firstColorAttIdx < 0) firstColorAttIdx = j;
			break;
		  case DEPTH_AT:
			if(depthAttIdx < 0) depthAttIdx = j;
			break;
		  case RESOLVE_AT:
			if(resolveAttIdx < 0) resolveAttIdx = j;
			break;
		}
	}
	
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = colorAttchementsCount;
	subpass.pColorAttachments = &attachments[firstColorAttIdx].ref;
	if(depthAttIdx >= 0) {
		subpass.pDepthStencilAttachment = &attachments[depthAttIdx].ref;
	}
	if(resolveAttIdx >= 0) {
		subpass.pResolveAttachments = &attachments[resolveAttIdx].ref;
	}
	
	std::vector<VkAttachmentDescription> att;
	att.resize(attachments.size());
	for(int j = 0; j < attachments.size(); j++) {
		att[j] = attachments[j].descr;
	}


	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(att.size());;
	renderPassInfo.pAttachments = att.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	VkResult result = vkCreateRenderPass(BP->device, &renderPassInfo, nullptr,
				&renderPass);
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create render pass!");
	}		
}

void RenderPass::createFramebuffers() {
	frameBuffers.resize(count);
	for (size_t i = 0; i < frameBuffers.size(); i++) {

		std::vector<VkImageView> att;
		att.resize(attachments.size());
		for(int j = 0; j < attachments.size(); j++) {
			att[j] = attachments[j].getView(i);
		}

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType =
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount =
						static_cast<uint32_t>(att.size());;
		framebufferInfo.pAttachments = att.data();
		framebufferInfo.width = width; 
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;
		
		VkResult result = vkCreateFramebuffer(BP->device, &framebufferInfo, nullptr,
					&frameBuffers[i]);
		if (result != VK_SUCCESS) {
			PrintVkError(result);
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void RenderPass::create() {
	createRenderPass();

	for(int i = 0; i < attachments.size(); i++) {
//		if(properties[i].type != RESOLVE_AT) {
		if(!properties[i].swapChain) {
			attachments[i].createResources();
		}
	}	

	createFramebuffers();
}

void RenderPass::begin(VkCommandBuffer commandBuffer, int currentImage) {
	clearValues.resize(properties.size());
	for(int i = 0; i < properties.size(); i++) {
		clearValues[i] = properties[i].clearValue;
	}
	
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass; 
	renderPassInfo.framebuffer = frameBuffers[currentImage];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = {(uint32_t)width, (uint32_t)height};

	renderPassInfo.clearValueCount =
					static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
			VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass::end(VkCommandBuffer commandBuffer) {
	vkCmdEndRenderPass(commandBuffer);
}

void RenderPass::cleanup() {
	for (size_t i = 0; i < frameBuffers.size(); i++) {
		vkDestroyFramebuffer(BP->device, frameBuffers[i], nullptr);
	}
		
	for(int i = 0; i < attachments.size(); i++) {
		attachments[i].cleanup();
	}
	
	vkDestroyRenderPass(BP->device, renderPass, nullptr);
}

void RenderPass::destroy() {
	for(int i = 0; i < attachments.size(); i++) {
		attachments[i].destroy();
	}	
}

std::vector <AttachmentProperties> *RenderPass::getStandardAttchmentsProperties(StockAttchmentsConfiguration cfg, BaseProject *BP) {
	static std::vector <AttachmentProperties> OneColorAndDepth = {
		{COLOR_AT, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, false, false,
			{.color = {.float32 = {0.0f,0.0f,0.0f,1.0f}}},
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{DEPTH_AT, BP->findDepthFormat(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
			VK_IMAGE_ASPECT_DEPTH_BIT, false, false,
			{.depthStencil = {1.0f,0}},
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}	
	};

	static std::vector <AttachmentProperties> DepthOnly = {
		{DEPTH_AT, VK_FORMAT_D32_SFLOAT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT, false, false,
			{.depthStencil = {1.0f,0}},
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
	};

	static std::vector <AttachmentProperties> SurfaceAADepth = {
		{COLOR_AT, BP->swapChainImageFormat, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, false, false,
			{.color = {.float32 = {0.0f,0.0f,0.0f,1.0f}}},
			BP->msaaSamples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
		{DEPTH_AT, BP->findDepthFormat(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
			VK_IMAGE_ASPECT_DEPTH_BIT, true, false,
			{.depthStencil = {1.0f,0}},
			BP->msaaSamples,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
		{RESOLVE_AT, BP->swapChainImageFormat,
			0, 0, false, true,
			{.color = {.float32 = {0.0f,0.0f,0.0f,1.0f}}},
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}	
	};

	static std::vector <AttachmentProperties> SurfaceNoAADepth = {
		{COLOR_AT, BP->swapChainImageFormat, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, false, true,
			{.color = {.float32 = {0.0f,0.0f,0.0f,1.0f}}},
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
		{DEPTH_AT, BP->findDepthFormat(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
			VK_IMAGE_ASPECT_DEPTH_BIT, true, false,
			{.depthStencil = {1.0f,0}},
			VK_SAMPLE_COUNT_1_BIT,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}
	};

	static std::vector <AttachmentProperties> NoAttachments = {
	};
	
	switch(cfg) {
	  case AT_ONE_COLOR_AND_DEPTH:
	    return &OneColorAndDepth;
		break;
	  case AT_SURFACE_AA_DEPTH:
	    return &SurfaceAADepth;
		break;
	  case AT_SURFACE_NOAA_DEPTH:
	    return &SurfaceNoAADepth;
		break;
	  case AT_DEPTH_ONLY:
	    return &DepthOnly;
		break;
	  default:
		return &NoAttachments;
	}
}

std::vector<VkSubpassDependency> *RenderPass::getStandardDependencies(StockAttchmentsDependencies cfg) {
	static std::vector<VkSubpassDependency> SimpleDependency = {
	  {
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_ACCESS_NONE_KHR,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT
	  } , {
		0,
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT	  }
	};

	static std::vector<VkSubpassDependency> DepthTransition = {
	  {
		VK_SUBPASS_EXTERNAL,
		0,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_DEPENDENCY_BY_REGION_BIT
	  } , {
		0,
		VK_SUBPASS_EXTERNAL,
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_DEPENDENCY_BY_REGION_BIT	  }
	};

	static std::vector<VkSubpassDependency> SurfaceOnly = {
			{
				VK_SUBPASS_EXTERNAL,
				0,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				0
			}	
	};

	static std::vector <VkSubpassDependency> NoDep = {
	};

	switch(cfg) {
	  case ATDEP_SIMPLE:
	    return &SimpleDependency;
		break;
	  case ATDEP_SURFACE_ONLY:
	    return &SurfaceOnly;
		break;
	  case ATDEP_DEPTH_TRANS:
	    return &DepthTransition;
		break;
	  default:
		return &NoDep;
	}
}

void Pipeline::init(BaseProject *bp, VertexDescriptor *vd,
					const std::string& VertShader, const std::string& FragShader,
					std::vector<DescriptorSetLayout *> d,
					std::vector<VkPushConstantRange> pk) {
	BP = bp;
	VD = vd;
	
	auto vertShaderCode = readFile(VertShader);
	auto fragShaderCode = readFile(FragShader);
	std::cout << "Vertex shader <" << VertShader << "> len: " << 
				vertShaderCode.size() << "\n";
	std::cout << "Fragment shader <" << FragShader << "> len: " <<
				fragShaderCode.size() << "\n";
	
	vertShaderModule =
			createShaderModule(vertShaderCode);
	fragShaderModule =
			createShaderModule(fragShaderCode);

 	compareOp = VK_COMPARE_OP_LESS;
 	polyModel = VK_POLYGON_MODE_FILL;
 	CM = VK_CULL_MODE_BACK_BIT;
 	transp = false;
	topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	D = d;
	PK = pk;
}

void Pipeline::setCompareOp(VkCompareOp _compareOp) {
 	compareOp = _compareOp;
}

void Pipeline::setPolygonMode(VkPolygonMode _polyModel) {
 	polyModel = _polyModel;
}

void Pipeline::setCullMode(VkCullModeFlagBits _CM) {
 	CM = _CM;
}

void Pipeline::setTransparency(bool _transp) {
 	transp = _transp;
}

void Pipeline::setTopology(VkPrimitiveTopology _topology) {
 	topology = _topology;
}


void Pipeline::create(RenderPass *RP) {	
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
    		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
    		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] =
    		{vertShaderStageInfo, fragShaderStageInfo};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = VD->getBindingDescription();
	auto attributeDescriptions = VD->getAttributeDescriptions();
			
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
	vertexInputInfo.vertexAttributeDescriptionCount =
			static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();
	vertexInputInfo.pVertexAttributeDescriptions =
			attributeDescriptions.data();		

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType =
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = topology;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) RP->width;
	viewport.height = (float) RP->height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	
	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = {(uint32_t)RP->width, (uint32_t)RP->height};
	
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType =
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;
	
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType =
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = polyModel;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = CM;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
	
	int colorAttId = RP->firstColorAttIdx;
	VkSampleCountFlagBits samples;
	if(colorAttId >= 0) {
		samples = RP->attachments[colorAttId].properties->samples;
	} else {
		samples = VK_SAMPLE_COUNT_1_BIT;
	}
	
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType =
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = samples;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional
	
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = transp ? VK_TRUE : VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor =
			transp ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor =
			transp ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp =
			VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor =
			VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor =
			VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp =
			VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType =
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional
	
	std::vector<VkDescriptorSetLayout> DSL(D.size());
	for(int i = 0; i < D.size(); i++) {
		DSL[i] = D[i]->descriptorSetLayout;
	}
	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = DSL.size();
	pipelineLayoutInfo.pSetLayouts = DSL.data();
	pipelineLayoutInfo.pushConstantRangeCount = PK.size();
	pipelineLayoutInfo.pPushConstantRanges = PK.data();
//std::cout << "Push constant ranges: " << PK.size() << "\n";
	
	VkResult result = vkCreatePipelineLayout(BP->device, &pipelineLayoutInfo, nullptr,
				&pipelineLayout);
	if (result != VK_SUCCESS) {
	 	PrintVkError(result);
		throw std::runtime_error("failed to create pipeline layout!");
	}
	
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = 
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = compareOp;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType =
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = RP->renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	
	result = vkCreateGraphicsPipelines(BP->device, VK_NULL_HANDLE, 1,
			&pipelineInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS) {
	 	PrintVkError(result);
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	
}

void Pipeline::destroy() {
	vkDestroyShaderModule(BP->device, fragShaderModule, nullptr);
	vkDestroyShaderModule(BP->device, vertShaderModule, nullptr);
}	

void Pipeline::bind(VkCommandBuffer commandBuffer) {
	vkCmdBindPipeline(commandBuffer,
					  VK_PIPELINE_BIND_POINT_GRAPHICS,
					  graphicsPipeline);

}

VkShaderModule Pipeline::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	
	VkShaderModule shaderModule;

	VkResult result = vkCreateShaderModule(BP->device, &createInfo, nullptr,
					&shaderModule);
	if (result != VK_SUCCESS) {
	 	PrintVkError(result);
		throw std::runtime_error("failed to create shader module!");
	}
	
	return shaderModule;
}

void Pipeline::cleanup() {
		vkDestroyPipeline(BP->device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(BP->device, pipelineLayout, nullptr);
}

void DescriptorSetLayout::init(BaseProject *bp, std::vector<DescriptorSetLayoutBinding> B) {
	BP = bp;
	Bindings = B;
	imgInfoSize = 0;
	
	std::vector<VkDescriptorSetLayoutBinding> binds;
	binds.resize(B.size());
	for(int i = 0; i < B.size(); i++) {
		binds[i].binding = B[i].binding;
		binds[i].descriptorType = B[i].type;
		binds[i].descriptorCount = B[i].count;
		binds[i].stageFlags = B[i].flags;
		binds[i].pImmutableSamplers = nullptr;
		if((B[i].type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) && (B[i].linkSize + B[i].count > imgInfoSize)) {
			imgInfoSize = B[i].linkSize + B[i].count;
		}
	}
	
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(binds.size());;
	layoutInfo.pBindings = binds.data();
	
	VkResult result = vkCreateDescriptorSetLayout(BP->device, &layoutInfo,
								nullptr, &descriptorSetLayout);
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void DescriptorSetLayout::cleanup() {
    	vkDestroyDescriptorSetLayout(BP->device, descriptorSetLayout, nullptr);	
}

void DescriptorSet::init(BaseProject *bp, DescriptorSetLayout *DSL,
						 std::vector<VkDescriptorImageInfo>VaSs) {
	BP = bp;
	Layout = DSL;
	
	int size = DSL->Bindings.size();
	int imgInfoSize = DSL->imgInfoSize;
//std::cout << "imgInfoSize: " << imgInfoSize << "(" << size << ")\n";
	
	uniformBuffers.resize(size);
	uniformBuffersMemory.resize(size);
	toFree.resize(size);

	for (int j = 0; j < size; j++) {
		uniformBuffers[j].resize(BP->swapChainImages.size());
		uniformBuffersMemory[j].resize(BP->swapChainImages.size());
//std::cout << j << " " << (DSL->Bindings[j].type) << "\n";
		if(DSL->Bindings[j].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
//std::cout << "Uniform size: " << DSL->Bindings[j].linkSize << "\n";
			for (size_t i = 0; i < BP->swapChainImages.size(); i++) {
				VkDeviceSize bufferSize = DSL->Bindings[j].linkSize;
				BP->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
									 	 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
									 	 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
									 	 uniformBuffers[j][i], uniformBuffersMemory[j][i]);
			}
			toFree[j] = true;
		} else {
			toFree[j] = false;
		}
	}
	
	std::vector<VkDescriptorSetLayout> layouts(BP->swapChainImages.size(),
											   DSL->descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = BP->descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(BP->swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();
//std::cout << "Allocating\n";	
	descriptorSets.resize(BP->swapChainImages.size());
	
	VkResult result = vkAllocateDescriptorSets(BP->device, &allocInfo,
										descriptorSets.data());
	if (result != VK_SUCCESS) {
		PrintVkError(result);
		throw std::runtime_error("failed to allocate descriptor sets!");
	}
	
	for (size_t i = 0; i < BP->swapChainImages.size(); i++) {
//std::cout << "Consdering swap chain image " << i << "\n";	

		std::vector<VkWriteDescriptorSet> descriptorWrites(size);
		std::vector<VkDescriptorBufferInfo> bufferInfo(size);
		std::vector<VkDescriptorImageInfo> imageInfo(imgInfoSize);
		for (int j = 0; j < size; j++) {
//std::cout << "Consdering binding " << j << "\n";	
			if(DSL->Bindings[j].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
//std::cout << "Writing uniform buffer " << j <<"\n";			
				bufferInfo[j].buffer = uniformBuffers[j][i];
				bufferInfo[j].offset = 0;
				bufferInfo[j].range = DSL->Bindings[j].linkSize;
				
				descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[j].dstSet = descriptorSets[i];
				descriptorWrites[j].dstBinding = DSL->Bindings[j].binding;
				descriptorWrites[j].dstArrayElement = 0;
				descriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[j].descriptorCount = DSL->Bindings[j].count;
				descriptorWrites[j].pBufferInfo = &bufferInfo[j];
			} else if(DSL->Bindings[j].type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
//std::cout << "Writing combined image sampler " << j << ", count " << DSL->Bindings[j].count << ", link " << DSL->Bindings[j].linkSize << "\n";
				for(int k = 0; k < DSL->Bindings[j].count; k++) {
					int h = DSL->Bindings[j].linkSize + k;
//std::cout << k << " " << h << " " << (&VaSs[h]) << "\n";
					VkDescriptorImageInfo VaS = VaSs[h];
//std::cout << VaS.imageView << " " << VaS.sampler << "\n";
					imageInfo[h] = VaS;
/*					imageInfo[h].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo[h].imageView = VaS.textureImageView;
					imageInfo[h].sampler = VaS.textureSampler;
*/				}
//std::cout << "Writing descriptor sets\n";			
				descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[j].dstSet = descriptorSets[i];
				descriptorWrites[j].dstBinding = DSL->Bindings[j].binding;
				descriptorWrites[j].dstArrayElement = 0;
				descriptorWrites[j].descriptorType =
											VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[j].descriptorCount = DSL->Bindings[j].count;
				descriptorWrites[j].pImageInfo = &imageInfo[DSL->Bindings[j].linkSize];
			}
		}		
//std::cout << "Updating descriptor sets\n";	
		vkUpdateDescriptorSets(BP->device,
						static_cast<uint32_t>(descriptorWrites.size()),
						descriptorWrites.data(), 0, nullptr);
	}
}

void DescriptorSet::cleanup() {
	for(int j = 0; j < uniformBuffers.size(); j++) {
		if(toFree[j]) {
			for (size_t i = 0; i < BP->swapChainImages.size(); i++) {
				vkDestroyBuffer(BP->device, uniformBuffers[j][i], nullptr);
				vkFreeMemory(BP->device, uniformBuffersMemory[j][i], nullptr);
			}
		}
	}
}

void DescriptorSet::bind(VkCommandBuffer commandBuffer, Pipeline &P, int setId,
						 int currentImage) {
//std::cout << "DS[ci]: " << &descriptorSets[currentImage] << "\n";
	vkCmdBindDescriptorSets(commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					P.pipelineLayout, setId, 1, &descriptorSets[currentImage],
					0, nullptr);
}

void DescriptorSet::map(int currentImage, void *src, int slot) {
	void* data;

	int size = Layout->Bindings[slot].linkSize;

	vkMapMemory(BP->device, uniformBuffersMemory[slot][currentImage], 0,
						size, 0, &data);
	memcpy(data, src, size);
	vkUnmapMemory(BP->device, uniformBuffersMemory[slot][currentImage]);	
}

#endif