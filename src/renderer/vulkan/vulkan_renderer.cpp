#include "vulkan_renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include "vulkan_mesh.h"
#include "vulkan_pipeline.h"
#include "vulkan_swapchain.h"
#include "renderer/mesh.h"


namespace Raytracing {
    VulkanRenderer::~VulkanRenderer() {
        delete vulkanDevice;
        delete graphicsPipeline;
        delete vulkanImage;
    }

    void VulkanRenderer::Init(const int width, const int height) {
        vulkanDevice = new VulkanDevice(width, height, glfwWindow);

        mesh = new VulkanMesh(vulkanDevice, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);


        std::string image_path = "textures/texture.jpg";

        int tex_width, tex_height, tex_channels;
        stbi_uc *pixels = stbi_load(image_path.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
        VkDeviceSize imageSize = tex_width * tex_height * 4;

        if (!pixels)
            throw std::runtime_error("Failed to load texture image.");

        VulkanTextureImageBuilder textureImageBuilder;
        textureImageBuilder.SetData(pixels, imageSize);
        textureImageBuilder.SetResolution(tex_width, tex_height);
        textureImageBuilder.SetFormat(VK_FORMAT_R8G8B8A8_SRGB);
        textureImageBuilder.SetFilter(VK_FILTER_LINEAR, VK_FILTER_LINEAR);
        textureImageBuilder.SetTiling(VK_IMAGE_TILING_OPTIMAL);

        vulkanImage = textureImageBuilder.Build(vulkanDevice);

        stbi_image_free(pixels);

        auto builder = PipelineBuilder();
        builder.SetShaders("shader/spv/vert.spv", "shader/spv/frag.spv");
        builder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
        builder.EnableDepthTest();
        builder.DisableBlending();
        builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        builder.SetMultisampling(VK_SAMPLE_COUNT_1_BIT);

        graphicsPipeline = builder.build(vulkanDevice);
        graphicsPipeline->GetShader()->SetImage(vulkanImage);
    }

    void VulkanRenderer::DrawFrame() {
        static auto start_time = std::chrono::high_resolution_clock::now();
        const auto current_time = std::chrono::high_resolution_clock::now();
        const float time = std::chrono::duration<float>(current_time - start_time).count();

        UniformBufferObject ubo{};
        float aspectRatio = vulkanDevice->GetSwapchain()->GetViewportWidth() / static_cast<float>(vulkanDevice->GetSwapchain()->
                                GetViewportHeight());

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        transform = glm::rotate(transform, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        transform = glm::scale(transform, glm::vec3(1.0f));

        ubo.model = transform;
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;

        const bool result = vulkanDevice->BeginFrame();
        if (!result) return;

        graphicsPipeline->GetShader()->UpdateUniformBuffer(ubo);
        vulkanDevice->DrawMesh(graphicsPipeline, mesh);
        vulkanDevice->DrawImGui();

        vulkanDevice->EndFrame();
    }

    void VulkanRenderer::IdleWait() {
        vkDeviceWaitIdle(vulkanDevice->GetDevice());
    }

    void VulkanRenderer::Resize(const int width, const int height) {
        Renderer::Resize(width, height);
        vulkanDevice->ResizeFramebuffer();
    }
}
