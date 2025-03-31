#include "vulkan_device.h"

#include <algorithm>
#include <chrono>
#include <iostream>

#include "util/Core.h"
#include "vulkan_utils.h"
#include "vulkan_pipeline.h"
#include "vulkan_vertex_buffer.h"
#include "vulkan_index_buffer.h"
#include "vulkan_image.h";
#include "vulkan_depth_image.h"

#include <backends/imgui_impl_vulkan.h>
#include "vulkan_texture_image.h"
#include "GLFW/glfw3.h"

namespace Raytracing
{
#ifdef NDEBUG
	constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
	constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif

	VulkanDevice::~VulkanDevice()
	{
		CleanupSwapChain();

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		delete graphicsPipeline;
		delete vulkanImage;
		delete vulkanDepthImage;
		delete vertexBuffer;
		delete indexBuffer;

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

	void VulkanDevice::Init(const int width, const int height, GLFWwindow* glfwWindow)
	{
		viewportWidth = width;
		viewportHeight = height;

		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		std::vector<const char*> device_extensions;
		std::vector<const char*> validation_layer_list;

		for (size_t i = 0; i < glfw_extension_count; i++)
			device_extensions.push_back(glfw_extensions[i]);

		device_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		device_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		if (ENABLE_VALIDATION_LAYERS)
		{
			device_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		instance = CreateVkInstance(device_extensions, validation_layer_list);
		surface = CreateSurface(glfwWindow);
		physicalDevice = PickPhysicalDevice();
		device = CreateLogicalDevice();

		CreateSwapChain();
		CreateImageViews();

		renderPass = CreateRenderPass(device);
		graphicsPipeline = new VulkanPipeline(this);

		vulkanDepthImage = new VulkanDepthImage(this, swapChainExtent.width, swapChainExtent.height);
		CreateFramebuffers();
		CreateCommandPool();

		vulkanImage = new VulkanTextureImage(this, "textures/texture.jpg");
		vertexBuffer = new VulkanVertexBuffer(this, mesh_vertices);
		indexBuffer = new VulkanIndexBuffer(this, mesh_indices);

		CreateDescriptorPool();
		CreateGUIDescriptorPool();

		CreateCommandBuffers();
		CreateSyncObjects();

		graphicsPipeline->Load("shader/spv/vert.spv", "shader/spv/frag.spv");
		graphicsPipeline->GetShader()->SetImage(vulkanImage);
	}

	VkResult VulkanDevice::SubmitDrawCommands(VkSemaphore* signalSemaphores) const
	{
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		const VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		return vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
	}

	VkResult VulkanDevice::PresentFrame(const uint32_t imageIndex, const VkSemaphore* signalSemaphores) const
	{
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		const VkSwapchainKHR swap_chains[] = {swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swap_chains;

		presentInfo.pImageIndices = &imageIndex;

		return vkQueuePresentKHR(presentQueue, &presentInfo);
	}

	void VulkanDevice::Draw()
	{
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		VkResult result;
		uint32_t imageIndex;

		result = vkAcquireNextImageKHR(
			device,
			swapChain,
			UINT64_MAX,
			imageAvailableSemaphores[currentFrame],
			VK_NULL_HANDLE,
			&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			throw std::runtime_error("Failed to acquire swap chain image.");

		vkResetFences(device, 1, &inFlightFences[currentFrame]);
		vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

		RecordCommandBuffer(commandBuffers[currentFrame], imageIndex);
		UpdateUniformBuffer();

		VkSemaphore* signalSemaphores = {(&renderFinishedSemaphores[currentFrame])};

		// Submit commands
		VulkanUtils::CheckVkResult(
			SubmitDrawCommands(signalSemaphores),
			"Failed to submit draw command buffer.");

		// Present frame
		result = PresentFrame(imageIndex, signalSemaphores);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image." + ' | ' + result);
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	VkSurfaceKHR VulkanDevice::CreateSurface(GLFWwindow* glfwWindow) const
	{
		VkSurfaceKHR surface;
		VulkanUtils::CheckVkResult(
			glfwCreateWindowSurface(instance, glfwWindow, nullptr, &surface),
			"Failed to create window surface."
		);

		return surface;
	}

	VkPhysicalDevice VulkanDevice::PickPhysicalDevice() const
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

	VkDevice VulkanDevice::CreateLogicalDevice()
	{
		VkDevice device;

		const VulkanUtils::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);

		constexpr float queue_priority = 1.0f;

		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = indices.graphicsFamily.value();
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

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

	VkQueue VulkanDevice::GetDeviceQueue() const
	{
		VkQueue graphics_queue;
		const VulkanUtils::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphics_queue);

		return graphics_queue;
	}

	SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport() const
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

	VkQueue VulkanDevice::GetDevicePresentQueue() const
	{
		VkQueue presentQueue;
		const VulkanUtils::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

		return presentQueue;
	}

	VkInstance VulkanDevice::CreateVkInstance(
		const std::vector<const char*>& deviceExtensions,
		const std::vector<const char*>& validationLayers)
	{
		VkInstanceCreateInfo createInfo;

		auto* vkApplicationInfo = new VkApplicationInfo();
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

		// ReSharper disable once CppRedundantBooleanExpressionArgument
		if (ENABLE_VALIDATION_LAYERS && !validationLayers.empty())
		{
			std::clog << "Validation layer enabled" << std::endl;
			for (auto& layer : validationLayers)
				std::clog << layer << std::endl;


			auto debugCreateInfo = Raytracing::VulkanUtils::CreateDebugMessengerCreateInfo();

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
			message += Raytracing::VulkanUtils::GetVkResultString(result);
			message += "'.";
			throw std::runtime_error(message);
		}

		return instance;
	}

	void VulkanDevice::CreateSwapChain()
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

		const VulkanUtils::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);
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

	void VulkanDevice::CreateImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++)
			swapChainImageViews[i] = VulkanUtils::CreateImageView(device, swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	VkRenderPass VulkanDevice::CreateRenderPass(const VkDevice device) const
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

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = {color_attachment, depthAttachment};
		VkRenderPassCreateInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
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

	void VulkanDevice::CreateFramebuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			swapChainFramebuffers[i] = CreateFramebuffer(
				device,
				swapChainImageViews[i],
				vulkanDepthImage->GetImageView(),
				renderPass,
				swapChainExtent.width, swapChainExtent.height);
		}
	}

	VkFramebuffer VulkanDevice::CreateFramebuffer(
		const VkDevice device,
		const VkImageView imageView,
		const VkImageView depthImageView,
		const VkRenderPass renderPass, const uint32_t width, const uint32_t height)
	{
		std::array<VkImageView, 2> attachments = {
			imageView,
			depthImageView
		};

		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = renderPass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
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

	void VulkanDevice::CreateCommandPool()
	{
		VulkanUtils::QueueFamilyIndices queueFamilyIndices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void VulkanDevice::CreateSyncObjects()
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

	void VulkanDevice::CreateGUIDescriptorPool()
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

		const VkResult err = vkCreateDescriptorPool(device, &pool_info, nullptr, &gui_descriptionPool);
		VulkanUtils::CheckVkResult(err);
	}

	void VulkanDevice::CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void VulkanDevice::RecreateSwapChain()
	{
		vkDeviceWaitIdle(device);

		CleanupSwapChain();

		CreateSwapChain();
		CreateImageViews();

		vulkanDepthImage->Resize(swapChainExtent.width, swapChainExtent.height);

		CreateFramebuffers();
	}

	void VulkanDevice::CleanupSwapChain() const
	{
		for (const auto framebuffer : swapChainFramebuffers)
			vkDestroyFramebuffer(device, framebuffer, nullptr);

		for (const auto image_view : swapChainImageViews)
			vkDestroyImageView(device, image_view, nullptr);

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	void VulkanDevice::CreateCommandBuffers()
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

	void VulkanDevice::SetViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent)
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)extent.width;
		viewport.height = (float)extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = extent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void VulkanDevice::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		VulkanUtils::CheckVkResult(result, "Failed to begin recording command buffer.");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		SetViewportAndScissor(commandBuffer, swapChainExtent);

		vertexBuffer->Bind(commandBuffer);
		indexBuffer->Bind(commandBuffer);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->GetPipeline());
		vkCmdBindDescriptorSets(commandBuffer,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        graphicsPipeline->GetPipelineLayout(), 0, 1,
		                        &graphicsPipeline->GetShader()->GetDescriptorSet(), 0,
		                        nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh_indices.size()), 1, 0, 0, 0);

		// Render ImGui
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);

		vkCmdEndRenderPass(commandBuffer);

		// End command buffer
		result = vkEndCommandBuffer(commandBuffer);
		VulkanUtils::CheckVkResult(result, "Failed to record command buffer.");
	}

	void VulkanDevice::UpdateUniformBuffer() const
	{
		static auto start_time = std::chrono::high_resolution_clock::now();
		const auto current_time = std::chrono::high_resolution_clock::now();
		const float time = std::chrono::duration<float>(current_time - start_time).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		graphicsPipeline->GetShader()->UpdateUniformBuffer(ubo);
	}

	VkSurfaceFormatKHR VulkanDevice::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

	VkPresentModeKHR VulkanDevice::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR || availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				return availablePresentMode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanDevice::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
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

	uint32_t VulkanDevice::GetQueueFamilyIndex() const
	{
		return VulkanUtils::FindQueueFamilies(physicalDevice, surface).graphicsFamily.value();
	}
}
