#include "imgui_vulkan.h"

#define APP_USE_UNLIMITED_FRAME_RATE

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

	void ImGuiVulkan::Resize(const int width, const int height) {}

	void ImGuiVulkan::SetupImGui(const int width, const int height)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// Check for WSI support
		VkBool32 res;
		vkGetPhysicalDeviceSurfaceSupportKHR(
			renderer->GetPhysicalDevice(),
			renderer->GetQueueFamilyIndex(),
			renderer->GetSurface(),
			&res);

		if (res != VK_TRUE)
			throw std::runtime_error("Error no WSI support on physical device 0\n");

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = renderer->GetInstance();
		init_info.PhysicalDevice = renderer->GetPhysicalDevice();
		init_info.Device = renderer->GetDevice();
		init_info.QueueFamily = renderer->GetQueueFamilyIndex();
		init_info.Queue = renderer->GetPresentQueue();
		init_info.DescriptorPool = renderer->GetDescriptorPool();
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.CheckVkResultFn = VulkanRenderer::CheckVkResult;

		ImGui_ImplVulkan_Init(&init_info);
	}

	void ImGuiVulkan::DrawUi()
	{
		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow(&show_demo_window);

		ImGui::Begin("Demo window");
		ImGui::Text("Demo text");
		ImGui::End();

		// Rendering
		ImGui::Render();
	}
}
