#include "imgui_vulkan.h"

namespace Raytracing
{
	static bool show_demo_window = true;

	ImGuiVulkan::ImGuiVulkan() = default;

	ImGuiVulkan::~ImGuiVulkan()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiVulkan::Init(GLFWwindow* window, VulkanRenderer* vulkanRenderer, const int width, const int height)
	{
		renderer = vulkanRenderer;
		glfwWindow = window;
		SetupImGui(width, height);
	}

	void ImGuiVulkan::Resize(const int width, const int height)
	{
		ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
		ImGui_ImplVulkanH_CreateOrResizeWindow(
			renderer->GetInstance(),
			renderer->GetPhysicalDevice(),
			renderer->GetDevice(),
			&g_MainWindowData,
			renderer->GetQueueFamilyIndex(),
			renderer->GetAllocationCallbackPointer(),
			width, height,
			g_MinImageCount);
		g_MainWindowData.FrameIndex = 0;
	}

	void ImGuiVulkan::SetupImGui(const int width, const int height)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		wd = &g_MainWindowData;
		SetupVulkanWindow(width, height);

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = renderer->GetInstance();
		init_info.PhysicalDevice = renderer->GetPhysicalDevice();
		init_info.Device = renderer->GetDevice();
		init_info.QueueFamily = renderer->GetQueueFamilyIndex();
		init_info.Queue = renderer->GetPresentQueue();
		init_info.PipelineCache = g_PipelineCache;
		init_info.DescriptorPool = renderer->GetDescriptorPool();
		init_info.RenderPass = wd->RenderPass;
		init_info.Subpass = 0;
		init_info.MinImageCount = g_MinImageCount;
		init_info.ImageCount = wd->ImageCount;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = renderer->GetAllocationCallbackPointer();
		init_info.CheckVkResultFn = VulkanRenderer::CheckVkResult;
		ImGui_ImplVulkan_Init(&init_info);
	}

	void ImGuiVulkan::SetupVulkanWindow(int width, int height)
	{
		wd->Surface = renderer->GetSurface();

		// Check for WSI support
		VkBool32 res;
		vkGetPhysicalDeviceSurfaceSupportKHR(
			renderer->GetPhysicalDevice(),
			renderer->GetQueueFamilyIndex(),
			wd->Surface,
			&res);

		if (res != VK_TRUE)
			throw std::runtime_error("Error no WSI support on physical device 0\n");

		// Select Surface Format
		const VkFormat requestSurfaceImageFormat[] = {
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_B8G8R8_UNORM,
			VK_FORMAT_R8G8B8_UNORM
		};

		const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
			renderer->GetPhysicalDevice(),
			wd->Surface, requestSurfaceImageFormat,
			IM_ARRAYSIZE(requestSurfaceImageFormat),
			requestSurfaceColorSpace);

		// Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
		VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
		VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
		wd->PresentMode =
			ImGui_ImplVulkanH_SelectPresentMode(renderer->GetPhysicalDevice(), wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
		//printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

		// Create SwapChain, RenderPass, Framebuffer, etc.
		IM_ASSERT(g_MinImageCount >= 2);
		ImGui_ImplVulkanH_CreateOrResizeWindow(
			renderer->GetInstance(),
			renderer->GetPhysicalDevice(),
			renderer->GetDevice(),
			wd,
			renderer->GetQueueFamilyIndex(),
			renderer->GetAllocationCallbackPointer(),
			width, height,
			g_MinImageCount);
	}

	void ImGuiVulkan::ImGuiFrameRender(ImDrawData* draw_data) const
	{
		VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
		VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
		VkResult err = vkAcquireNextImageKHR(renderer->GetDevice(), wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE,
		                                     &wd->FrameIndex);

		/*
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
			g_SwapChainRebuild = true;
		*/

		if (err == VK_ERROR_OUT_OF_DATE_KHR) return;
		if (err != VK_SUBOPTIMAL_KHR) VulkanRenderer::CheckVkResult(err);

		ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
		{
			err = vkWaitForFences(renderer->GetDevice(), 1, &fd->Fence, VK_TRUE, UINT64_MAX);
			// wait indefinitely instead of periodically checking
			VulkanRenderer::CheckVkResult(err);

			err = vkResetFences(renderer->GetDevice(), 1, &fd->Fence);
			VulkanRenderer::CheckVkResult(err);
		}
		{
			err = vkResetCommandPool(renderer->GetDevice(), fd->CommandPool, 0);
			VulkanRenderer::CheckVkResult(err);
			VkCommandBufferBeginInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
			VulkanRenderer::CheckVkResult(err);
		}
		{
			VkRenderPassBeginInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			info.renderPass = wd->RenderPass;
			info.framebuffer = fd->Framebuffer;
			info.renderArea.extent.width = wd->Width;
			info.renderArea.extent.height = wd->Height;
			info.clearValueCount = 1;
			info.pClearValues = &wd->ClearValue;
			vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
		}

		// Record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

		// Submit command buffer
		vkCmdEndRenderPass(fd->CommandBuffer);
		{
			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			info.waitSemaphoreCount = 1;
			info.pWaitSemaphores = &image_acquired_semaphore;
			info.pWaitDstStageMask = &wait_stage;
			info.commandBufferCount = 1;
			info.pCommandBuffers = &fd->CommandBuffer;
			info.signalSemaphoreCount = 1;
			info.pSignalSemaphores = &render_complete_semaphore;

			err = vkEndCommandBuffer(fd->CommandBuffer);
			VulkanRenderer::CheckVkResult(err);
			err = vkQueueSubmit(renderer->GetPresentQueue(), 1, &info, fd->Fence);
			VulkanRenderer::CheckVkResult(err);
		}
	}

	void ImGuiVulkan::ImGuiFramePresent() const
	{
		/*
		if (g_SwapChainRebuild)
			return;
		*/

		VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
		VkPresentInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &render_complete_semaphore;
		info.swapchainCount = 1;
		info.pSwapchains = &wd->Swapchain;
		info.pImageIndices = &wd->FrameIndex;
		VkResult err = vkQueuePresentKHR(renderer->GetPresentQueue(), &info);

		/*
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
			g_SwapChainRebuild = true;
		*/

		if (err == VK_ERROR_OUT_OF_DATE_KHR)
			return;
		if (err != VK_SUBOPTIMAL_KHR)
			VulkanRenderer::CheckVkResult(err);
		wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
	}

	void ImGuiVulkan::DrawUi() const
	{
		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// Rendering
		ImGui::Render();

		ImDrawData* draw_data = ImGui::GetDrawData();
		ImGuiFrameRender(draw_data);
		ImGuiFramePresent();
	}
}
