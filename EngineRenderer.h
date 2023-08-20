#pragma once

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class EngineRenderer
{
public:
	EngineRenderer(EngineWindow* window);
	~EngineRenderer();

public:
	bool Init();
	bool Destroy();

	void DrawFrame();

	VkDevice GetVkDevice() { return m_device; }
	
private:
	EngineWindow* m_window;

private:
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;

	const std::vector<const char*> m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	std::vector<VkFramebuffer> m_swapChainFrameBuffers;

	const int MAX_FRAMES_IN_FLIGHT = 2;

private:

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;

	VkFence m_inFlightFence;

	VkDevice m_device;

	VkInstance m_instance;

	VkSurfaceKHR m_surface;

	VkQueue m_presentQueue;
	VkQueue m_graphicsQueue;

	VkRenderPass m_renderPass;

	VkSwapchainKHR m_swapChain;

	VkCommandPool m_commandPool;
	VkCommandBuffer m_commandBuffer;

	VkExtent2D m_swapChainExtent;

	VkPipeline m_graphicsPipeline;

	VkFormat m_swapChainImageFormat;

	VkPipelineLayout m_pipelineLayout;

	VkDebugUtilsMessengerEXT m_debugMessenger;

	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

private:
	void CreateInstance();

	void SetupDebugMessenger();

	void CreateSurface();

	void PickPhysicalDevice();

	void CreateLogicalDevice();

	void CreateSwapChain();

	void CreateImageViews();

	void CreateRenderPass();

	void CreateGraphicsPipeline();

	void CreateFrameBuffers();

	void CreateCommandPool();

	void CreateCommandBuffer();

	void CreateSyncObjects();

private:

	bool CheckValidationLayerSupport();

	bool IsDeviceSuitable(VkPhysicalDevice device);

	bool CheckDeviceExtensionsSupport(VkPhysicalDevice device);

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

private:
	std::vector<const char*> GetRequiredExtensions();

private:
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* userData);
};

