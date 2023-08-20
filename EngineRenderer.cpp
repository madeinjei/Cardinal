#include "cardinal_pch.h"
#include "cardinal.h"
#include "core.h"

EngineRenderer::EngineRenderer(EngineWindow* window) 
{
	this->m_window = window;

	this->m_imageAvailableSemaphore = {};
	this->m_renderFinishedSemaphore = {};
	this->m_swapChainExtent = {};
	this->m_inFlightFence = {};

	this->m_device = VK_NULL_HANDLE;

	this->m_instance = VK_NULL_HANDLE;

	this->m_surface = VK_NULL_HANDLE;

	this->m_presentQueue = VK_NULL_HANDLE;
	this->m_graphicsQueue = VK_NULL_HANDLE;

	this->m_renderPass = VK_NULL_HANDLE;

	this->m_swapChain = VK_NULL_HANDLE;

	this->m_commandPool = VK_NULL_HANDLE;
	this->m_commandBuffer = VK_NULL_HANDLE;

	this->m_graphicsPipeline = VK_NULL_HANDLE;

	this->m_swapChainImageFormat = VK_FORMAT_UNDEFINED;

	this->m_pipelineLayout = VK_NULL_HANDLE;

	this->m_debugMessenger = VK_NULL_HANDLE;

	this->m_physicalDevice = VK_NULL_HANDLE;
}

EngineRenderer::~EngineRenderer() { }

bool EngineRenderer::Init()
{
	if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayerSupport())
	{
		Logger::Error("VALIDATION LAYERS REQUESTED, BUT NOT AVAILABLE");

		return false;
	}

	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFrameBuffers();
	CreateCommandPool();
	CreateCommandBuffer();
	CreateSyncObjects();

	Logger::Info("ENGINE RENDERER INITIALIZED");

	return true;
}

bool EngineRenderer::Destroy()
{
	vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
	vkDestroyFence(m_device, m_inFlightFence, nullptr);

	vkDestroyCommandPool(m_device, m_commandPool, nullptr);

	for (auto framebuffer : m_swapChainFrameBuffers) 
	{
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);
	}

	vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_device, m_renderPass, nullptr);

	for (auto imageView : m_swapChainImageViews) 
	{
		vkDestroyImageView(m_device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
	vkDestroyDevice(m_device, nullptr);

	if (ENABLE_VALIDATION_LAYERS) {
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	return true;
}

void EngineRenderer::DrawFrame()
{
	VkResult result;

	uint32_t imageIndex;

	result = vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);

	if (result != VK_SUCCESS)
	{
		Logger::Error("WAIT FOR FENCES TIMED OUT");
		Logger::Error("%s", string_VkResult(result));
	}
	
	result = vkResetFences(m_device, 1, &m_inFlightFence);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO RESET FENCES");
		Logger::Error("%s", string_VkResult(result));
	}

	result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO ACQUIRE THE NEXT IMAGE");
		Logger::Error("%s", string_VkResult(result));

		return;
	}

	result = vkResetCommandBuffer(m_commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO RESET COMMAND BUFFER");
		Logger::Error("%s", string_VkResult(result));
	}

	RecordCommandBuffer(m_commandBuffer, imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence);

	if (result != VK_SUCCESS) 
	{
		Logger::Error("FAILED TO SUBMIT DRAW COMMAND BUFFER");
		Logger::Error("%s", string_VkResult(result));
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO PRESENT QUEUE");
		Logger::Error("%s", string_VkResult(result));

		return;
	}

	Logger::Trace("DRAWING FRAME");
}

void EngineRenderer::CreateInstance()
{
	VkResult result;

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "CARDINAL";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "CARDINAL SYSTEM";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> extensions = GetRequiredExtensions();

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	if (ENABLE_VALIDATION_LAYERS) {

		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {

		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	result = vkCreateInstance(&createInfo, nullptr, &m_instance);

	if (result != VK_SUCCESS) 
	{
		Logger::Error("FAILED TO CREATE INSTANCE!");
		Logger::Error("%s", string_VkResult(result));
	}
}

void EngineRenderer::CreateSurface()
{
	VkResult result;
#ifdef _WIN32

	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = m_window->GetWindow();
	createInfo.hinstance = GetModuleHandle(nullptr);

	result = vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO CREATE WINDOW SURFACE");
		Logger::Error("%s", string_VkResult(result));
	}
#endif
#ifdef __APPLE__
	//SETUP METAL SURFACE
#endif // __APPLE__

	Logger::Info( "WINDOW SURFACE CREATED SUCCESSFULLY");
}

void EngineRenderer::CreateLogicalDevice()
{
	QueueFamilyIndices indicies = FindQueueFamilies(m_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
	std::set<uint32_t> uniqueQueueFamilies = { indicies.graphicsFamily.value(), indicies.presentFamily.value() };

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

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

	if (ENABLE_VALIDATION_LAYERS) 
	{

		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	} else {

		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {

		Logger::Error( "FAILED TO CREATE LOGICAL DEVICE");

		throw std::runtime_error("FAILED TO CREATE LOGICAL DEVICE");
	}

	vkGetDeviceQueue(m_device, indicies.presentFamily.value(), 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, indicies.graphicsFamily.value(), 0, &m_graphicsQueue);

	Logger::Info( "LOGICAL DEVICES CREATED SUCCESSFULLY");
}

void EngineRenderer::CreateSwapChain()
{
	VkResult result;

	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indicies = FindQueueFamilies(m_physicalDevice);

	uint32_t queueFamilyIndicies[] = { indicies.graphicsFamily.value(), indicies.presentFamily.value() };

	if (indicies.graphicsFamily != indicies.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndicies;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO CREATE SWAP CHAIN");
		Logger::Error("%s", string_VkResult(result));
	}

	result = vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO INITIALIZE SWAP CHAIN IMAGES");
		Logger::Error("%s", string_VkResult(result));
	}

	m_swapChainImages.resize(imageCount);

	result = vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO GET SWAP CHAIN IMAGES");
		Logger::Error("%s", string_VkResult(result));
	}

	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;
}

void EngineRenderer::CreateImageViews()
{
	VkResult result;

	m_swapChainImageViews.resize(m_swapChainImages.size());

	for (size_t i = 0; i < m_swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		result = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]);

		if (result !=  VK_SUCCESS)
		{
			Logger::Error("FAILED TO CREATE IMAGE VIEWS");
			Logger::Error("%s", string_VkResult(result));
		}
	}
}

void EngineRenderer::CreateRenderPass()
{
	VkResult result;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO CREATE RENDER PASS");
		Logger::Error("%s", string_VkResult(result));
	}
}

void EngineRenderer::CreateGraphicsPipeline()
{
	VkResult result;

	std::vector<char> vertexShaderCode = Directory::ReadFile("shaders/vertex_shader.spv");
	std::vector<char> fragmentShaderCode = Directory::ReadFile("shaders/fragment_shader.spv");

	VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageInfo.module = vertexShaderModule;
	vertexShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = fragmentShaderModule;
	fragmentShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

	if (result != VK_SUCCESS)
	{
		Logger::Error( "FAILED TO CREATE PIPELINE LAYOUT");
		Logger::Error("%s", string_VkResult(result));
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);

	if (result != VK_SUCCESS)
	{
		Logger::Error( "FAILED TO CREATE GRAPHICS PIPELINE");
		Logger::Error("%s", string_VkResult(result));
	}

	vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);

	Logger::Info( "GRAPHICS PIPELINE CREATED SUCCESSFULLY");
}

void EngineRenderer::CreateFrameBuffers()
{
	m_swapChainFrameBuffers.resize(m_swapChainImageViews.size());

	for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { m_swapChainImageViews[i] };

		VkFramebufferCreateInfo frameBufferInfo{};
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = m_renderPass;
		frameBufferInfo.attachmentCount = 1;
		frameBufferInfo.pAttachments = attachments;
		frameBufferInfo.width = m_swapChainExtent.width;
		frameBufferInfo.height = m_swapChainExtent.height;
		frameBufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr, &m_swapChainFrameBuffers[i]) != VK_SUCCESS)
		{
			Logger::Error("FAILED TO CREATE FRAME BUFFER");

			throw std::runtime_error("[VK] : FAILED TO CREATE FRAME BUFFER");
		}
	}

	Logger::Info("FRAME BUFFER CREATED SUCCESSFULLY");
}

void EngineRenderer::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
	{
		Logger::Error("FAILED TO CREATE COMMAND POOL");

		throw std::runtime_error("FAILED TO CREATE COMMAND POOL");
	}

	Logger::Info("COMMAND POOL CREATED SUCCESSFULLY");
}

void EngineRenderer::CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer) != VK_SUCCESS)
	{
		Logger::Error("FAILED TO ALLOCATE COMMAND BUFFERS");

		throw std::runtime_error("FAILED TO ALLOCATE COMMAND BUFFERS");
	}

	Logger::Info("COMMAND BUFFERS CREATED SUCCESSFULLY");
}

void EngineRenderer::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result_imageAvailableSemaphore = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore);
	
	if (result_imageAvailableSemaphore != VK_SUCCESS)
	{
		Logger::Error("FAILED TO CREATE IMAGE AVAILABLE SEMAPHORE");
		Logger::Error("%s", string_VkResult(result_imageAvailableSemaphore));
	}

	VkResult result_renderFinishedSemaphore = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore);

	if (result_renderFinishedSemaphore != VK_SUCCESS)
	{
		Logger::Error("FAILED TO CREATE IMAGE AVAILABLE SEMAPHORE");
		Logger::Error("%s", string_VkResult(result_renderFinishedSemaphore));
	}

	VkResult result_inFlightFence = vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFence);

	if (result_inFlightFence != VK_SUCCESS)
	{
		Logger::Error("FAILED TO CREATE IN FLIGHT FENCE");
		Logger::Error("%s", string_VkResult(result_inFlightFence));
	}

	if (result_imageAvailableSemaphore != VK_SUCCESS || result_renderFinishedSemaphore != VK_SUCCESS || result_inFlightFence != VK_SUCCESS)
	{
		Logger::Error("FAILED TO CREATE SYNC OBJECTS");
	}
}

void EngineRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkResult result;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO BEGIN RECORDING COMMAND BUFFER");
		Logger::Error("%s", string_VkResult(result));
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_swapChainFrameBuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapChainExtent;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapChainExtent.width;
	viewport.height = (float)m_swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	result = vkEndCommandBuffer(commandBuffer);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO RECORD COMMAND BUFFER");
		Logger::Error("%s", string_VkResult(result));
	}
}

void EngineRenderer::PickPhysicalDevice()
{
	VkResult result;

	uint32_t deviceCount = 0;

	result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO GET DEVICE COUNT"); 
		Logger::Error("%s", string_VkResult(result));
	}

	if (deviceCount == 0)
	{
		Logger::Error("FAILED TO FIND ANY GPUs WITH VULKAN SUPPORT");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);

	result = vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
	
	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO GET PHYSICAL DEVICES");
		Logger::Error("%s", string_VkResult(result));
	}

	for (const VkPhysicalDevice& device : devices)
	{
		if (IsDeviceSuitable(device)) 
		{
			m_physicalDevice = device;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE) 
	{
		Logger::Error("FAILED TO FIND A SUITABLE GPU");
	}

	VkPhysicalDeviceProperties m_physicalDeviceProperties;

	vkGetPhysicalDeviceProperties(m_physicalDevice, &m_physicalDeviceProperties);
	
	Logger::Info("FOUND A SUITABLE GPU <%s>", m_physicalDeviceProperties.deviceName);
}

void EngineRenderer::SetupDebugMessenger()
{
	VkResult result;

	if (!ENABLE_VALIDATION_LAYERS) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	
	PopulateDebugMessengerCreateInfo(createInfo);

	result = CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO SETUP DEBUG MESSENGER");
		Logger::Error("%s", string_VkResult(result));
	}

	Logger::Info("SETUP DEBUG MESSENGER SUCCESSFULLY");
}

void EngineRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
}

void EngineRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) {

		func(instance, debugMessenger, pAllocator);

		Logger::Info("DEBUG_UTILS_MESSENGER_EXT DESTROYED");
	}
}

VkShaderModule EngineRenderer::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;

	if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule))
	{
		Logger::Error("FAILED TO CREATE SHADER MODULE");
	}

	return shaderModule;
}

bool EngineRenderer::CheckValidationLayerSupport()
{
	VkResult result;

	uint32_t layerCount = 0;
	
	result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO GET LAYER COUNT");
		Logger::Error("%s", string_VkResult(result));
	}

	std::vector<VkLayerProperties> availableLayers(layerCount);

	result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO GET AVAILABLE LAYERS");
		Logger::Error("%s", string_VkResult(result));
	}

	for (const char* layerName : m_validationLayers) 
	{
		bool layerFound = false;

		Logger::Trace("LOCATING LAYER: %s", layerName);

		for (const VkLayerProperties& layer : availableLayers) 
		{
			if (strcmp(layerName, layer.layerName) == 0) 
			{
				layerFound = true;

				Logger::Info("FOUND LAYER: %s", layer.layerName);
				
				break;
			}
		}

		if (!layerFound) {

			return false;
		}
	}

	Logger::Info("VALIDATION LAYER CREATED SUCCESFULLY");

	return true;
}

bool EngineRenderer::IsDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indicies = FindQueueFamilies(device);

	bool extensionsSupported = CheckDeviceExtensionsSupport(device);

	bool swapChainAdequate = false;

	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indicies.isComplete() && extensionsSupported && swapChainAdequate;
}

bool EngineRenderer::CheckDeviceExtensionsSupport(VkPhysicalDevice device)
{
	VkResult result;

	uint32_t extensionCount;

	result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO GET DEVICE EXTENSION COUNT");
		Logger::Error("%s", string_VkResult(result));
	}

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);

	result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	if (result != VK_SUCCESS)
	{
		Logger::Error("FAILED TO GET AVAILABLE DEVICE EXTENSIONS");
		Logger::Error("%s", string_VkResult(result));
	}

	std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

	for (const VkExtensionProperties& extension : availableExtensions) 
	{
		requiredExtensions.erase(extension.extensionName);

		Logger::Info("%s SUPPORTED", extension.extensionName);
	}
	
	return requiredExtensions.empty();																		
}

std::vector<const char*> EngineRenderer::GetRequiredExtensions() {

	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

	if (ENABLE_VALIDATION_LAYERS) {
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

#ifdef _WIN32
	enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif // _WIN32

#ifdef __APPLE__
	enabledExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif // __APPLE__

	return enabledExtensions;
}

QueueFamilyIndices EngineRenderer::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indicies;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;

	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indicies.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;

		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

		if (presentSupport) {

			indicies.presentFamily = i;
		}

		if (indicies.isComplete()) {

			break;
		}

		i++;
	}

	return indicies;
}

SwapChainSupportDetails EngineRenderer::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t formatCount;

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);

		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);

		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR EngineRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{

			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR EngineRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const VkPresentModeKHR availabePresentMode : availablePresentModes)
	{
		if (availabePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{

			return availabePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D EngineRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {

		return capabilities.currentExtent;
	}
	else
	{
		UINT width, height;

		m_window->GetWindowDimensions(&width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

VkResult EngineRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) {

		Logger::Info("DEBUG_UTILS_MESSENGER_EXT CREATED");

		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {

		Logger::Info("DEBUG_UTILS_MESSENGER_EXT NOT CREATED");

		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL EngineRenderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* userData)
{
	std::cerr << "Validation Layer : " << pCallbackData->pMessage << std::endl;

	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)	
	{

	}

	return VK_FALSE;
}
