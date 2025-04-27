#include "vulkan_device.h"

#include <chrono>
#include <iostream>
#include <backends/imgui_impl_vulkan.h>

#include "util/core.h"
#include "vulkan_mesh.h"
#include "vulkan_utils.h"
#include "vulkan_pipeline.h"
#include "vulkan_swapchain.h"
#include "vulkan_renderpass.h"
#include "GLFW/glfw3.h"

#define VMA_IMPLEMENTATION
#include <glm/gtx/transform.hpp>
#include <renderer/transform.h>

#include "vulkan_descriptor_pool.h"
#include "vulkan_descriptor_writer.h"
#include "vulkan_framebuffer.h"
#include "renderer/camera.h"
#include "resource/resource_manager.h"
#include "util/log.h"
#include "vma/vk_mem_alloc.h"

namespace Raytracing
{
#ifdef NDEBUG
    constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
	constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif

    VulkanDevice::VulkanDevice(const int width, const int height, GLFWwindow* glfwWindow)
    {
        Init(width, height, glfwWindow);
    }

    VulkanDevice::~VulkanDevice()
    {
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

    void VulkanDevice::DrawMesh(const Ref<Camera>& camera, const Transform& transform, const Ref<VulkanMesh>& mesh) const
    {
        SimplePushConstantData pushConstantData;
        pushConstantData.normalMatrix = transform.GetNormalMatrix();
        pushConstantData.transform = camera->GetProjection() * camera->GetView() * transform.GetTransform();

        for (auto& meshlet: mesh->GetMeshlets())
        {
            const VulkanMaterial& material = mesh->GetMaterials()[meshlet.GetMaterialIndex()];
            const VkPipeline pipeline = material.pipeline->GetPipeline();
            const VkPipelineLayout pipelineLayout = material.pipeline->GetPipelineLayout();

            vkCmdPushConstants(
                commandBuffers[currentFrame], pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                sizeof(SimplePushConstantData), &pushConstantData);

            DrawMeshlet(meshlet, pipeline, pipelineLayout, material.descriptorSet);
        }
    }

    void VulkanDevice::DrawMeshlet(const VulkanMeshlet& meshlet, const VkPipeline pipeline, const VkPipelineLayout pipelineLayout,
                                   const VkDescriptorSet descriptorSet) const
    {
        vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                                nullptr);

        meshlet.Bind(commandBuffers[currentFrame]);
        vkCmdDrawIndexed(commandBuffers[currentFrame], meshlet.GetIndexCount(), 1, 0, 0, 0);
    }

    void VulkanDevice::DrawImGui() const
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);
    }

    bool VulkanDevice::BeginFrame()
    {
        if (!SetupNextFrame())
            return false;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CHECK_MSG(
            vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo),
            "Failed to begin recording command buffer.");

        gBufferPass->Begin(commandBuffers[currentFrame],
                           gbufferFramebuffers[currentImageIndex],
                           vulkanSwapChain->GetExtent());

        SetViewportAndScissor();
        return true;
    }

    void VulkanDevice::EndFrame()
    {
        // End render pass
        gBufferPass->End(commandBuffers[currentFrame]);

        if (!presentSampler)
            presentSampler = ImageSamplerBuilder(this).Build();

        auto pipeline = ResourceManager::renderToScreenPipeline;

        auto writer = VulkanDescriptorWriter(*pipeline->GetDescriptorSetLayout(),
                                             GetShaderDescriptorPool());

        VkDescriptorImageInfo info{};
        info.sampler = presentSampler;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = gbufferFramebuffers[currentFrame]->GetAttachments()[0].imageView;

        writer.WriteImage(0, &info);

        VkDescriptorImageInfo info2{};
        info2.sampler = presentSampler;
        info2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info2.imageView = gbufferFramebuffers[currentFrame]->GetAttachments()[1].imageView;

        writer.WriteImage(1, &info2);

        VkDescriptorImageInfo info3{};
        info3.sampler = presentSampler;
        info3.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info3.imageView = gbufferFramebuffers[currentFrame]->GetAttachments()[2].imageView;

        writer.WriteImage(2, &info3);

        if (!presentDescriptorSet)
            writer.Build(presentDescriptorSet);
        else
            writer.Overwrite(presentDescriptorSet);

        lightingPass->Begin(commandBuffers[currentFrame],
                            presentFramebuffers[currentImageIndex],
                            vulkanSwapChain->GetExtent());

        SetViewportAndScissor();
        DrawMeshlet(*screenRect, ResourceManager::renderToScreenPipeline->GetPipeline(),
                    ResourceManager::renderToScreenPipeline->GetPipelineLayout(), presentDescriptorSet);

        DrawImGui();

        lightingPass->End(commandBuffers[currentFrame]);

        // End command buffer
        VkResult result = vkEndCommandBuffer(commandBuffers[currentFrame]);
        VK_CHECK_MSG(result, "Failed to record command buffer.");

        VkSemaphore* signalSemaphores = {(&renderFinishedSemaphores[currentFrame])};

        // Submit commands
        VK_CHECK_MSG(SubmitDrawCommands(signalSemaphores), "Failed to submit draw command buffer.");

        // Present frame
        result = PresentFrame(currentImageIndex, signalSemaphores);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;

            vkDeviceWaitIdle(device);
            gbufferFramebuffers.clear();

            CreateSwapchain();
            CreateFramebuffers();
        } else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swap chain image." + ' | ' + result);
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanDevice::ResizeFramebuffer(const int width, const int height)
    {
        viewportWidth = width;
        viewportHeight = height;
        framebufferResized = true;
    }

    void VulkanDevice::Init(const int width, const int height, GLFWwindow* glfwWindow)
    {
        viewportWidth = width;
        viewportHeight = height;

        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> device_extensions;
        const std::vector<const char*> validation_layer_list;

        device_extensions.reserve(glfw_extension_count);
        for (size_t i = 0; i < glfw_extension_count; i++)
            device_extensions.push_back(glfw_extensions[i]);

#ifdef PLATFORM_MACOS
            device_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
        device_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

        if (ENABLE_VALIDATION_LAYERS)
        {
            device_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            //validation_layer_list.push_back("VK_LAYER_LUNARG_crash_diagnostic");
            //validation_layer_list.push_back("VK_LAYER_KHRONOS_validation");
            //validation_layer_list.push_back("VK_LAYER_LUNARG_api_dump");
        }

        instance = CreateVkInstance(device_extensions, validation_layer_list);
        surface = CreateSurface(glfwWindow);
        physicalDevice = PickPhysicalDevice();
        device = CreateLogicalDevice();

        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        LOG_TRACE("Vulkan: VMA init");
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        vmaCreateAllocator(&allocatorInfo, &vmaAllocator);

        CreateSwapchain();
        CreateRenderpass();
        CreateFramebuffers();

        CreateCommandPool();
        CreateDescriptorPool();
        CreateCommandBuffers();
        CreateSyncObjects();

        screenRect = CreateScope<VulkanMeshlet>(this, Primitives::RECTANGLE_VERTICES, Primitives::RECTANGLE_INDICES);
    }

    void VulkanDevice::CreateSwapchain()
    {
        LOG_TRACE("Vulkan: create swapchain");
        vulkanSwapChain = nullptr;

        const auto swapChainSupport = VulkanUtils::QuerySwapChainSupport(physicalDevice, surface);
        const VkSurfaceFormatKHR surfaceFormat = VulkanUtils::ChooseSwapSurfaceFormat(swapChainSupport.formats);

        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(physicalDevice, surface);
        vulkanSwapChain = VulkanSwapchain::Builder(this)
                .SetResolution(viewportWidth, viewportHeight)
                .SetPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR)
                .SetImageCount(imageCount)
                .SetImageFormat(surfaceFormat.format)
                .Build();
    }

    void VulkanDevice::CreateFramebuffers()
    {
        const uint32_t imageCount = VulkanUtils::GetSwapchainImageCount(physicalDevice, surface);
        gbufferFramebuffers.resize(imageCount);
        presentFramebuffers.resize(imageCount);

        for (size_t i = 0; i < imageCount; i++)
        {
            gbufferFramebuffers[i] = VulkanFramebuffer::Builder(this)
                    .SetRenderpass(gBufferPass)
                    .SetResolution(viewportWidth, viewportHeight)
                    .AddAttachment(FramebufferAttachmentFormat::RGBA8)
                    .AddAttachment(FramebufferAttachmentFormat::RGBA8)
                    .AddAttachment(FramebufferAttachmentFormat::RGBA8)
                    .AddAttachment(FramebufferAttachmentFormat::DEPTH24_STENCIL8)
                    .Build();
        }

        for (size_t i = 0; i < imageCount; i++)
        {
            presentFramebuffers[i] = VulkanFramebuffer::Builder(this)
                    .SetRenderpass(lightingPass)
                    .SetResolution(viewportWidth, viewportHeight)
                    .AddAttachment(vulkanSwapChain->GetImageViews()[i])
                    .Build();
        }
    }

    void VulkanDevice::CreateRenderpass()
    {
        LOG_TRACE("Vulkan: create renderpass");
        gBufferPass = VulkanRenderPass::Builder(this)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .AddDepthAttachment()
                .Build();

        lightingPass = VulkanRenderPass::Builder(this)
                .AddColorAttachment(VK_FORMAT_R8G8B8A8_UNORM)
                .Build();
    }

    VkResult VulkanDevice::SubmitDrawCommands(VkSemaphore* signalSemaphores) const
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        const VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
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
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vulkanSwapChain->GetSwapChain();
        presentInfo.pImageIndices = &imageIndex;

        return vkQueuePresentKHR(presentQueue, &presentInfo);
    }

    bool VulkanDevice::SetupNextFrame()
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        const VkResult result = vkAcquireNextImageKHR(
            device,
            vulkanSwapChain->GetSwapChain(),
            UINT64_MAX,
            imageAvailableSemaphores[currentFrame],
            VK_NULL_HANDLE,
            &currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            LOG_WARN("Resize");
            vkDeviceWaitIdle(device);

            gbufferFramebuffers.clear();
            CreateSwapchain();
            CreateFramebuffers();
            return false;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swap chain image.");

        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

        return true;
    }

    void VulkanDevice::SetViewportAndScissor() const
    {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(vulkanSwapChain->GetExtent().width);
        viewport.height = static_cast<float>(vulkanSwapChain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = vulkanSwapChain->GetExtent();
        vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);
    }

    void VulkanDevice::ImmediateSubmit(std::function<void(VkCommandBuffer commandBuffer)>&& function) const
    {
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

        function(commandBuffer);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    VkSurfaceKHR VulkanDevice::CreateSurface(GLFWwindow* glfwWindow) const
    {
        VkSurfaceKHR surface;
        VK_CHECK_MSG(glfwCreateWindowSurface(instance, glfwWindow, nullptr, &surface), "Failed to create window surface.");

        return surface;
    }

    VkSampleCountFlagBits VulkanDevice::GetMaxMSAASampleCount() const
    {
        const VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.
                                          framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    VkPhysicalDevice VulkanDevice::PickPhysicalDevice() const
    {
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

        if (device_count == 0)
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        for (const auto& device: devices)
        {
            if (VulkanUtils::IsDeviceSuitable(device, surface))
            {
                physical_device = device;
                break;
            }
        }

        if (physical_device == VK_NULL_HANDLE)
            throw std::runtime_error("Failed to find a suitable GPU!");

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

        VK_CHECK_MSG(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "Failed to create logical device.");

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

    VkQueue VulkanDevice::GetDevicePresentQueue() const
    {
        VkQueue presentQueue;
        const VulkanUtils::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

        return presentQueue;
    }

    VkInstance VulkanDevice::CreateVkInstance(const std::vector<const char*>& deviceExtensions,
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

        vkApplicationInfo->apiVersion = VK_API_VERSION_1_3;

        createInfo.pApplicationInfo = vkApplicationInfo;

        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        if (!deviceExtensions.empty())
        {
            createInfo.enabledExtensionCount = deviceExtensions.size();
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        } else
        {
            createInfo.enabledExtensionCount = 0;
        }

        if (ENABLE_VALIDATION_LAYERS && !validationLayers.empty())
        {
            std::clog << "Validation layer enabled" << std::endl;
            for (auto& layer: validationLayers)
                std::clog << layer << std::endl;

            const auto debugCreateInfo = VulkanUtils::CreateDebugMessengerCreateInfo();

            createInfo.enabledLayerCount = validationLayers.size();
            createInfo.ppEnabledLayerNames = validationLayers.data();

            createInfo.pNext = &debugCreateInfo;
        } else
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

    void VulkanDevice::CreateCommandPool()
    {
        const VulkanUtils::QueueFamilyIndices queueFamilyIndices = VulkanUtils::FindQueueFamilies(physicalDevice, surface);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        VK_CHECK_MSG(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), "Failed to create command pool.");
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

    void VulkanDevice::CreateDescriptorPool()
    {
        globalUniformPool = VulkanDescriptorPool::Builder(this)
                .SetMaxSets(MAX_FRAMES_IN_FLIGHT)
                .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT)
                .Build();

        shaderDescriptorPool = VulkanDescriptorPool::Builder(this)
                .SetMaxSets(100)
                .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100)
                .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
                .Build();

        imguiDescriptorPool = VulkanDescriptorPool::Builder(this)
                .SetMaxSets(100)
                .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
                .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                .Build();
    }

    void VulkanDevice::CreateCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        VK_CHECK_MSG(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()), "Failed to allocate command buffers.");
    }

    uint32_t VulkanDevice::GetQueueFamilyIndex() const
    {
        return VulkanUtils::FindQueueFamilies(physicalDevice, surface).graphicsFamily.value();
    }
}
