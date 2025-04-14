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
#include <renderer/components.h>

#include "vulkan_descriptor_pool.h"
#include "renderer/camera.h"
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
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

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

    void VulkanDevice::DrawMesh(Ref<VulkanPipeline> pipeline, Ref<Camera> camera, const Ref<VulkanMesh> mesh, const Transform& transform,
                                const Ref<VulkanImage> texture) const
    {
        const glm::mat4 modelMatrix = transform.GetTransform();

        SimplePushConstantData pushConstantData;
        pushConstantData.transform = camera->GetProjection() * camera->GetView() * modelMatrix;
        pushConstantData.normalMatrix = transform.GetNormalMatrix();

        pipeline->GetShader()->SetImage(texture);

        mesh->Bind(commandBuffers[currentFrame]);
        vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());
        vkCmdBindDescriptorSets(commandBuffers[currentFrame],
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline->GetPipelineLayout(), 0, 1,
                                &pipeline->GetShader()->GetDescriptorSet(), 0,
                                nullptr);
        vkCmdPushConstants(
            commandBuffers[currentFrame],
            pipeline->GetPipelineLayout(),
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(SimplePushConstantData),
            &pushConstantData);

        vkCmdDrawIndexed(commandBuffers[currentFrame], mesh->GetIndexCount(), 1, 0, 0, 0);
    }

    void VulkanDevice::DrawImGui() const
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);
    }

    bool VulkanDevice::BeginFrame()
    {
        if (!SetupNextFrame())
            return false;

        BeginRenderPass();
        SetViewportAndScissor();
        return true;
    }

    void VulkanDevice::EndFrame()
    {
        // End render pass
        vkCmdEndRenderPass(commandBuffers[currentFrame]);

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
            vulkanSwapChain->RecreateSwapChain();
        } else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swap chain image." + ' | ' + result);
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanDevice::Init(const int width, const int height, GLFWwindow* glfwWindow)
    {
        viewportWidth = width;
        viewportHeight = height;

        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> device_extensions;
        std::vector<const char*> validation_layer_list;

        device_extensions.reserve(glfw_extension_count);
        for (size_t i = 0; i < glfw_extension_count; i++)
            device_extensions.push_back(glfw_extensions[i]);

#ifdef APPLE
            device_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
        device_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

        if (ENABLE_VALIDATION_LAYERS)
        {
            device_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        std::cout << "Vulkan: create instance" << '\n';
        instance = CreateVkInstance(device_extensions, validation_layer_list);
        std::cout << "Vulkan: create surface" << '\n';
        surface = CreateSurface(glfwWindow);
        std::cout << "Vulkan: pick physical device" << '\n';
        physicalDevice = PickPhysicalDevice();
        std::cout << "Vulkan: create logical device" << '\n';
        device = CreateLogicalDevice();

        std::cout << "Vulkan: create renderpass" << '\n';
        vulkanRenderPass = CreateRef<VulkanRenderPass>(this, VulkanSwapchain::GetImageFormat(this));
        vulkanSwapChain = CreateRef<VulkanSwapchain>(this, viewportWidth, viewportHeight);

        CreateCommandPool();
        CreateDescriptorPool();
        CreateCommandBuffers();
        CreateSyncObjects();

        std::cout << "Vulkan: VMA init" << '\n';
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        vmaCreateAllocator(&allocatorInfo, &vmaAllocator);
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
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vulkanSwapChain->GetSwapChain();
        presentInfo.pImageIndices = &imageIndex;

        return vkQueuePresentKHR(presentQueue, &presentInfo);
    }

    bool VulkanDevice::SetupNextFrame()
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(
            device,
            vulkanSwapChain->GetSwapChain(),
            UINT64_MAX,
            imageAvailableSemaphores[currentFrame],
            VK_NULL_HANDLE,
            &currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            vulkanSwapChain->RecreateSwapChain();
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

    void VulkanDevice::BeginRenderPass() const
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CHECK_MSG(
            vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo),
            "Failed to begin recording command buffer.");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vulkanRenderPass->Get();
        renderPassInfo.framebuffer = vulkanSwapChain->GetSwapChainFramebuffers()[currentImageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vulkanSwapChain->GetExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.
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

        if (device_count == 0) throw std::runtime_error("Failed to find GPUs with Vulkan support!");

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

        VK_CHECK_MSG(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), "Failed to create descriptor pool.");

        globalUniformPool = VulkanDescriptorPool::Builder(this)
                .SetMaxSets(MAX_FRAMES_IN_FLIGHT)
                .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT)
                .Build();

        shaderDescriptorPool = VulkanDescriptorPool::Builder(this)
                .SetMaxSets(MAX_FRAMES_IN_FLIGHT)
                .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100)
                .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
                .Build();

        imguiDescriptorPool = VulkanDescriptorPool::Builder(this)
                .SetMaxSets(IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE)
                .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
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

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    uint32_t VulkanDevice::GetQueueFamilyIndex() const
    {
        return VulkanUtils::FindQueueFamilies(physicalDevice, surface).graphicsFamily.value();
    }
}
