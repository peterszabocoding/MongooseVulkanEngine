#include "renderer/vulkan/imgui_vulkan.h"

#include "imgui.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include "imgui_internal.h"
#include "renderer/vulkan/vulkan_utils.h"

#define APP_USE_UNLIMITED_FRAME_RATE

namespace MongooseVK
{
    namespace ImGuiUtils
    {
        void DrawFloatControl(const std::string& label, float& values, float min, float max, float steps, float columnWidth)
        {
            ImGuiIO& io = ImGui::GetIO();

            ImGui::PushID(label.c_str());

            auto labelSize = ImGui::CalcTextSize(label.c_str());

            ImGui::Text(label.c_str());
            ImGui::SameLine();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, labelSize.x + 15.0);
            ImGui::NextColumn();

            ImGui::SetColumnWidth(1, columnWidth);
            ImGui::DragFloat("", &values, steps, min, max, "%.4f");

            ImGui::Columns(1);

            ImGui::PopID();
        }

        void DrawIntControl(const std::string& label, int& values, int min, int max, float columnWidth)
        {
            ImGuiIO& io = ImGui::GetIO();

            ImGui::PushID(label.c_str());

            auto labelSize = ImGui::CalcTextSize(label.c_str());

            ImGui::Text(label.c_str());
            ImGui::SameLine();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, labelSize.x + 15.0);
            ImGui::NextColumn();

            ImGui::SetColumnWidth(1, columnWidth);
            ImGui::DragInt("", &values, 1, min, max);

            ImGui::Columns(1);

            ImGui::PopID();
        }

        void DrawVec3Control(const std::string& label, glm::vec3& values, bool normalizeVector, float resetValue, float columnWidth)
        {
            ImGuiIO& io = ImGui::GetIO();
            auto boldFont = io.Fonts->Fonts[0];

            ImGui::PushID(label.c_str());

            ImGui::Text(label.c_str());
            ImGui::SameLine();

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, columnWidth);
            ImGui::NextColumn();

            ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.3f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("X", buttonSize))
                values.x = resetValue;
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.3f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("Y", buttonSize))
                values.y = resetValue;
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.3f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("Z", buttonSize))
                values.z = resetValue;
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
            ImGui::PopItemWidth();

            ImGui::PopStyleVar();

            ImGui::Columns(1);

            if (normalizeVector)
                values = glm::normalize(values);

            ImGui::PopID();
        }

        void DrawRGBColorPicker(const std::string& label, glm::vec3& values, float columnWidth)
        {
            ImGuiIO& io = ImGui::GetIO();
            auto boldFont = io.Fonts->Fonts[0];

            ImGui::PushID(label.c_str());

            ImGui::Dummy({ columnWidth , 1.0f });

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, columnWidth / 3);
            ImGui::Text(label.c_str());
            ImGui::NextColumn();

            ImGui::SetColumnWidth(1, columnWidth);
            ImGui::ColorEdit3("", &values[0], 0.1f);
            ImGui::Dummy({ columnWidth , 1.0f });

            ImGui::Columns(1);

            ImGui::PopID();
        }
    }

    ImGuiVulkan::ImGuiVulkan() = default;

    ImGuiVulkan::~ImGuiVulkan()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiVulkan::Init(GLFWwindow* window, VulkanRenderer* renderer)
    {
        vulkanDevice = VulkanDevice::Get();
        glfwWindow = window;
        SetupImGui(renderer);
    }

    void ImGuiVulkan::Resize()
    {
        for (auto& uiWindow: uiWindows) uiWindow->Resize();
    }

    void ImGuiVulkan::SetupImGui(VulkanRenderer* renderer) const
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
            vulkanDevice->GetPhysicalDevice(),
            vulkanDevice->GetQueueFamilyIndex(),
            vulkanDevice->GetSurface(),
            &res);

        if (res != VK_TRUE)
            throw std::runtime_error("Error no WSI support on physical device 0\n");

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = vulkanDevice->GetInstance();
        init_info.PhysicalDevice = vulkanDevice->GetPhysicalDevice();
        init_info.Device = vulkanDevice->GetDevice();
        init_info.QueueFamily = vulkanDevice->GetQueueFamilyIndex();
        init_info.Queue = vulkanDevice->GetPresentQueue();
        init_info.DescriptorPool = vulkanDevice->GetGuiDescriptorPool();
        // TODO Pass a render pass here, otherwise ImGui doesn't work on MacOS
        init_info.RenderPass = renderer->renderPasses.presentPass->GetRenderPass()->Get();
        init_info.MinImageCount = 2;
        init_info.ImageCount = 2;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.CheckVkResultFn = [](const VkResult result) { VK_CHECK(result); };

        ImGui_ImplVulkan_Init(&init_info);
    }

    void ImGuiVulkan::DrawUi()
    {
        const auto io = ImGui::GetIO();

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Debug Window");
        for (ImGuiWindow* uiWindow: uiWindows)
        {
            if (ImGui::CollapsingHeader(uiWindow->GetTitle()))
            {
                uiWindow->Draw();
            }

            ImGui::Separator();
        }
        ImGui::End();
        // Rendering
        ImGui::Render();
    }

    void ImGuiVulkan::AddWindow(ImGuiWindow* window)
    {
        uiWindows.push_back(window);
    }
}
