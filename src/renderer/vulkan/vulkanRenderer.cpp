#include "vulkanRenderer.h"

#include <set>
#include <string>
#include <algorithm>
#include <backends/imgui_impl_vulkan.h>

#include "vulkan_utils.h"
#include "renderer/mesh.h"
#include "util/Core.h"
#include "util/filesystem.h"

namespace Raytracing
{
#ifdef NDEBUG
	constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
	constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif

	VulkanRenderer::~VulkanRenderer()
	{
		CleanupSwapChain();

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);
		vkDestroyBuffer(device, vertexBuffer, nullptr);

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

	void VulkanRenderer::Init(const int width, const int height)
	{
		viewportWidth = width;
		viewportHeight = height;
		InitVulkan();
	}

	void VulkanRenderer::InitVulkan()
	{
		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		std::vector<const char*> glfw_extension_list;
		std::vector<const char*> validation_layer_list;

		for (size_t i = 0; i < glfw_extension_count; i++)
			glfw_extension_list.push_back(glfw_extensions[i]);

		glfw_extension_list.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		glfw_extension_list.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		if (ENABLE_VALIDATION_LAYERS)
		{
			glfw_extension_list.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		instance = CreateVkInstance(glfw_extension_list, validation_layer_list);
		surface = CreateSurface();
		physicalDevice = PickPhysicalDevice();
		device = CreateLogicalDevice();

		CreateSwapChain();
		CreateImageViews();

		renderPass = CreateRenderPass(device);
		graphicsPipeline = CreateGraphicsPipeline(device);

		CreateFramebuffers();
		CreateCommandPool();
		CreateVertexBuffer(device, mesh_vertices);
		CreateIndexBuffer(device, mesh_indices);
		CreateCommandBuffers();
		CreateSyncObjects();
		CreateDescriptorPool();
	}

	void VulkanRenderer::DrawFrame()
	{
		//Timer timer("Render");

		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
		                                        &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		vkResetFences(device, 1, &inFlightFences[currentFrame]);
		vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		const VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
			throw std::runtime_error("failed to submit draw command buffer!");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = {swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	uint32_t VulkanRenderer::GetQueueFamilyIndex() const
	{
		return VulkanUtils::FindQueueFamilies(physicalDevice, surface).graphicsFamily.value();
	}

	void VulkanRenderer::IdleWait()
	{
		vkDeviceWaitIdle(device);
	}

	void VulkanRenderer::Resize(const int width, const int height)
	{
		Renderer::Resize(width, height);
		framebufferResized = true;
	}

	VkPhysicalDevice VulkanRenderer::PickPhysicalDevice() const
	{
		VkPhysicalDevice physical_device = VK_NULL_HANDLE;
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

		if (device_count == 0) throw std::runtime_error("Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

		for (const auto& device : devices)
		{
			if (VulkanUtils::IsDeviceSuitable(device, surface))
			{
				physical_device = device;
				break;
			}
		}

		if (physical_device == VK_NULL_HANDLE) throw std::runtime_error("Failed to find a suitable GPU!");

		return physical_device;
	}

	VkDevice VulkanRenderer::CreateLogicalDevice()
	{
		VkDevice device;

		const QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);

		constexpr float queue_priority = 1.0f;

		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = indices.graphicsFamily.value();
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		VkPhysicalDeviceFeatures deviceFeatures{};
		VkDeviceCreateInfo createInfo{};
		createInfo.pQueueCreateInfos = &queue_create_info;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pEnabledFeatures = &deviceFeatures;

		const std::vector device_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		createInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
		createInfo.ppEnabledExtensionNames = device_extensions.data();

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
			throw std::runtime_error("failed to create logical device!");

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

		return device;
	}

	VkQueue VulkanRenderer::GetDeviceQueue() const
	{
		VkQueue graphics_queue;
		const QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphics_queue);

		return graphics_queue;
	}

	VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& code) const
	{
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shader_module;
		VulkanUtils::CheckVkResult(
			vkCreateShaderModule(device, &create_info, nullptr, &shader_module),
			"Failed to create shader module."
		);

		return shader_module;
	}

	VkSurfaceKHR VulkanRenderer::CreateSurface() const
	{
		VkSurfaceKHR surface;
		VulkanUtils::CheckVkResult(
			glfwCreateWindowSurface(instance, glfwWindow, nullptr, &surface),
			"Failed to create window surface."
		);

		return surface;
	}

	SwapChainSupportDetails VulkanRenderer::QuerySwapChainSupport() const
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &format_count, nullptr);

		if (format_count != 0)
		{
			details.formats.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &format_count, details.formats.data());
		}

		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &present_mode_count, nullptr);

		if (present_mode_count != 0)
		{
			details.presentModes.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &present_mode_count,
			                                          details.presentModes.data());
		}

		return details;
	}

	VkQueue VulkanRenderer::GetDevicePresentQueue() const
	{
		VkQueue presentQueue;
		const QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

		return presentQueue;
	}

	VkInstance VulkanRenderer::CreateVkInstance(
		const std::vector<const char*>& deviceExtensions,
		const std::vector<const char*>& validationLayers)
	{
		VkInstanceCreateInfo createInfo;

		VkApplicationInfo* vkApplicationInfo = new VkApplicationInfo();
		vkApplicationInfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		vkApplicationInfo->pApplicationName = APPLICATION_NAME.c_str();
		vkApplicationInfo->pEngineName = "No Engine";

		vkApplicationInfo->applicationVersion = VK_MAKE_VERSION(
			APPLICATION_VERSION.major,
			APPLICATION_VERSION.minor,
			APPLICATION_VERSION.patch);

		vkApplicationInfo->engineVersion = VK_MAKE_VERSION(
			APPLICATION_VERSION.major,
			APPLICATION_VERSION.minor,
			APPLICATION_VERSION.patch);

		vkApplicationInfo->apiVersion = VK_API_VERSION_1_0;

		createInfo.pApplicationInfo = vkApplicationInfo;

		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		if (!deviceExtensions.empty())
		{
			createInfo.enabledExtensionCount = deviceExtensions.size();
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}
		else
		{
			createInfo.enabledExtensionCount = 0;
		}

		if (ENABLE_VALIDATION_LAYERS && !validationLayers.empty())
		{
			std::clog << "Validation layer enabled" << std::endl;
			for (auto& layer : validationLayers)
				std::clog << layer << std::endl;


			auto debugCreateInfo = VulkanUtils::CreateDebugMessengerCreateInfo();

			createInfo.enabledLayerCount = validationLayers.size();
			createInfo.ppEnabledLayerNames = validationLayers.data();

			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VkInstance instance;
		const VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
		if (result != VK_SUCCESS)
		{
			std::string message = "ERROR: Instance creation failed with result '";
			message += VulkanUtils::GetVkResultString(result);
			message += "'.";
			throw std::runtime_error(message);
		}

		return instance;
	}

	void VulkanRenderer::CreateSwapChain()
	{
		const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport();
		const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
		const VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

		uint32_t image_count = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && image_count > swapChainSupport.capabilities.
			maxImageCount)
		{
			image_count = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;

		createInfo.minImageCount = image_count;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		const QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);
		const uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		VkResult err = vkGetSwapchainImagesKHR(device, swapChain, &image_count, nullptr);
		VulkanUtils::CheckVkResult(err);

		swapChainImages.resize(image_count);

		err = vkGetSwapchainImagesKHR(device, swapChain, &image_count, swapChainImages.data());
		VulkanUtils::CheckVkResult(err);

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void VulkanRenderer::CreateImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++)
			swapChainImageViews[i] = CreateImageView(swapChainImages[i]);
	}

	VkImageView VulkanRenderer::CreateImageView(const VkImage image) const
	{
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = image;

		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = swapChainImageFormat;

		create_info.components.r = VK_COMPONENT_SWIZZLE_R;
		create_info.components.g = VK_COMPONENT_SWIZZLE_G;
		create_info.components.b = VK_COMPONENT_SWIZZLE_B;
		create_info.components.a = VK_COMPONENT_SWIZZLE_A;

		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		VkImageView image_view;
		if (vkCreateImageView(device, &create_info, nullptr, &image_view) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views!");
		}

		return image_view;
	}

	VkPipeline VulkanRenderer::CreateGraphicsPipeline(VkDevice device)
	{
		auto vert_shader_code = FileSystem::ReadFile("shader/spv/vert.spv");
		auto frag_shader_code = FileSystem::ReadFile("shader/spv/frag.spv");

		VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
		VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

		VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_shader_module;
		vert_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_shader_module;
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

		auto binding_description = Vertex::GetBindingDescription();
		auto attribute_descriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;

		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
		vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

		VkPipelineInputAssemblyStateCreateInfo input_assembly{};
		input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewport_state{};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo color_blending{};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &color_blend_attachment;
		color_blending.blendConstants[0] = 0.0f;
		color_blending.blendConstants[1] = 0.0f;
		color_blending.blendConstants[2] = 0.0f;
		color_blending.blendConstants[3] = 0.0f;

		std::vector dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;


		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipeline_info{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		pipeline_info.pDepthStencilState = nullptr; // Optional
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = &dynamicState;
		pipeline_info.layout = pipelineLayout;
		pipeline_info.renderPass = renderPass;
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipeline_info.basePipelineIndex = -1; // Optional

		VkPipeline pipeline;
		VulkanUtils::CheckVkResult(
			vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline),
			"Failed to create graphics pipeline."
		);

		vkDestroyShaderModule(device, frag_shader_module, nullptr);
		vkDestroyShaderModule(device, vert_shader_module, nullptr);

		return pipeline;
	}

	VkRenderPass VulkanRenderer::CreateRenderPass(const VkDevice device) const
	{
		VkAttachmentDescription color_attachment{};
		color_attachment.format = swapChainImageFormat;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref;
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments = &color_attachment;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		VkRenderPass renderPass;
		VulkanUtils::CheckVkResult(
			vkCreateRenderPass(device, &render_pass_info, nullptr, &renderPass),
			"Failed to create render pass."
		);

		return renderPass;
	}

	void VulkanRenderer::CreateFramebuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			swapChainFramebuffers[i] = CreateFramebuffer(
				device,
				swapChainImageViews[i],
				renderPass,
				swapChainExtent.width, swapChainExtent.height);
		}
	}

	VkFramebuffer VulkanRenderer::CreateFramebuffer(
		const VkDevice device, const VkImageView imageView, const VkRenderPass renderPass, const uint32_t width, const uint32_t height)
	{
		const VkImageView attachments[] = {imageView};

		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = renderPass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = width;
		framebuffer_info.height = height;
		framebuffer_info.layers = 1;

		VkFramebuffer framebuffer;
		VulkanUtils::CheckVkResult(
			vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffer),
			"failed to create framebuffer!"
		);

		return framebuffer;
	}

	void VulkanRenderer::CreateCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void VulkanRenderer::CreateSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

	void VulkanRenderer::CreateVertexBuffer(const VkDevice device, const std::vector<Vertex>& vertexData)
	{
		VkDeviceSize bufferSize = sizeof(vertexData[0]) * vertexData.size();
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertexData.data(), bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBuffer,
			vertexBufferMemory);

		CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void VulkanRenderer::CreateIndexBuffer(const VkDevice device, const std::vector<uint16_t> mesh_indices)
	{
		const VkDeviceSize buffer_size = sizeof(mesh_indices[0]) * mesh_indices.size();

		VkBuffer staging_buffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_buffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, buffer_size, 0, &data);
		memcpy(data, mesh_indices.data(), buffer_size);
		vkUnmapMemory(device, stagingBufferMemory);

		CreateBuffer(
			buffer_size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indexBuffer,
			indexBufferMemory);

		CopyBuffer(staging_buffer, indexBuffer, buffer_size);

		vkDestroyBuffer(device, staging_buffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void VulkanRenderer::CreateDescriptorPool()
	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE
			},
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 0;

		for (VkDescriptorPoolSize& pool_size : pool_sizes)
			pool_info.maxSets += pool_size.descriptorCount;

		pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
		pool_info.pPoolSizes = pool_sizes;

		const VkResult err = vkCreateDescriptorPool(device, &pool_info, g_Allocator, &g_DescriptorPool);
		VulkanUtils::CheckVkResult(err);
	}

	void VulkanRenderer::CreateBuffer(
		const VkDeviceSize size,
		const VkBufferUsageFlags usage,
		const VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory) const
	{
		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements mem_requirements;
		vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

		VkMemoryAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_requirements.size;
		alloc_info.memoryTypeIndex = VulkanUtils::FindMemoryType(physicalDevice, mem_requirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &alloc_info, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void VulkanRenderer::RecreateSwapChain()
	{
		vkDeviceWaitIdle(device);

		CleanupSwapChain();

		CreateSwapChain();
		CreateImageViews();
		CreateFramebuffers();
	}

	void VulkanRenderer::CleanupSwapChain() const
	{
		for (const auto framebuffer : swapChainFramebuffers)
			vkDestroyFramebuffer(device, framebuffer, nullptr);

		for (const auto image_view : swapChainImageViews)
			vkDestroyImageView(device, image_view, nullptr);

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	void VulkanRenderer::CreateCommandBuffers()
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh_indices.size()), 1, 0, 0, 0);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void VulkanRenderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
	{
		VkCommandBufferAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = commandPool;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer, &begin_info);

		VkBufferCopy copy_region;
		copy_region.srcOffset = 0; // Optional
		copy_region.dstOffset = 0; // Optional
		copy_region.size = size;
		vkCmdCopyBuffer(command_buffer, srcBuffer, dstBuffer, 1, &copy_region);

		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		vkQueueSubmit(graphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, commandPool, 1, &command_buffer);
	}

	VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& available_format : availableFormats)
		{
			// Switch from VK_FORMAT_B8G8R8A8_SRGB because of ImGui
			if (available_format.format == VK_FORMAT_R8G8B8A8_UNORM
				&& available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return available_format;
			}
		}
		return availableFormats[0];
	}

	VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;

		VkExtent2D actual_extent = {
			static_cast<uint32_t>(viewportWidth),
			static_cast<uint32_t>(viewportHeight)
		};

		actual_extent.width = std::clamp(
			actual_extent.width,
			capabilities.minImageExtent.width,
			capabilities.maxImageExtent.width);

		actual_extent.height = std::clamp(
			actual_extent.height,
			capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height);

		return actual_extent;
	}
}
