#include "renderer/vulkan/vulkan_device.h"

#include <chrono>
#include <iostream>
#include <backends/imgui_impl_vulkan.h>

#include "util/core.h"
#include "renderer/vulkan/vulkan_mesh.h"
#include "renderer/vulkan/vulkan_utils.h"
#include "renderer/vulkan/vulkan_pipeline.h"
#include "GLFW/glfw3.h"

#define VMA_IMPLEMENTATION

#include <renderer/shader_cache.h>
#include <renderer/vulkan/vulkan_framebuffer.h>
#include <renderer/vulkan/vulkan_texture.h>
#include <tiny_gltf/tiny_gltf.h>

#include "renderer/vulkan/vulkan_descriptor_pool.h"
#include "renderer/vulkan/vulkan_descriptor_writer.h"
#include "renderer/camera.h"
#include "resource/resource_manager.h"
#include "util/log.h"
#include "vma/vk_mem_alloc.h"

namespace MongooseVK
{
#ifdef NDEBUG
    constexpr bool ENABLE_VALIDATION_LAYERS = true;
#else
	constexpr bool ENABLE_VALIDATION_LAYERS = false;
#endif

    VulkanDevice* VulkanDevice::s_Instance = nullptr;

    VulkanDevice::VulkanDevice(GLFWwindow* glfwWindow)
    {
        ASSERT(!s_Instance, "Vulkan device has been initialized already");
        s_Instance = this;
        Init(glfwWindow);
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

    VulkanDevice* VulkanDevice::Create(GLFWwindow* glfwWindow)
    {
        return new VulkanDevice(glfwWindow);
    }

    void VulkanDevice::DrawMeshlet(const DrawCommandParams& params)
    {
        VulkanPipeline* pipeline = GetPipeline(params.pipelineHandle);

        if (params.pushConstantParams.data)
        {
            vkCmdPushConstants(params.commandBuffer,
                               pipeline->pipelineLayout,
                               params.pushConstantParams.shaderStageFlags,
                               0,
                               params.pushConstantParams.size,
                               params.pushConstantParams.data);
        }

        vkCmdBindPipeline(params.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

        if (!params.descriptorSets.empty())
        {
            vkCmdBindDescriptorSets(params.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout,
                                    0, params.descriptorSets.size(), params.descriptorSets.data(), 0, nullptr);
        }

        params.meshlet->Bind(params.commandBuffer);
        vkCmdDrawIndexed(params.commandBuffer, params.meshlet->GetIndexCount(), 1, 0, 0, 0);
        drawCallCounter++;
    }

    void VulkanDevice::DrawFrame(VkSwapchainKHR swapchain, DrawFrameFunction draw, OutOfDateErrorCallback errorCallback)
    {
        VkResult result = SetupNextFrame(swapchain);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            errorCallback();
            return;
        }

        frameDeletionQueue.Flush();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CHECK_MSG(vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo), "Failed to begin recording command buffer.");

        draw(commandBuffers[currentFrame], currentImageIndex);

        // End command buffer
        result = vkEndCommandBuffer(commandBuffers[currentFrame]);
        VK_CHECK_MSG(result, "Failed to record command buffer.");

        // Submit commands
        VkSemaphore* signalSemaphores = {(&renderFinishedSemaphores[currentFrame])};
        VK_CHECK_MSG(SubmitDrawCommands(signalSemaphores), "Failed to submit draw command buffer.");

        // Present frame
        result = PresentFrame(swapchain, currentImageIndex, signalSemaphores);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || getReadyToResize)
        {
            getReadyToResize = false;
            errorCallback();
            return;
        }

        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to present swap chain image." + ' | ' + result);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        prevDrawCallCount = drawCallCounter;
        drawCallCounter = 0;
    }


    void VulkanDevice::GetReadyToResize()
    {
        getReadyToResize = true;
    }

    void VulkanDevice::Init(GLFWwindow* glfwWindow)
    {
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        VulkanUtils::GetAvailableExtensions();

        std::vector<const char*> device_extensions;
        std::vector<const char*> validation_layer_list;

        device_extensions.reserve(glfw_extension_count);

        for (size_t i = 0; i < glfw_extension_count; i++)
            device_extensions.push_back(glfw_extensions[i]);

#ifdef PLATFORM_MACOS
            device_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

        device_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

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

        msaaSamples = VulkanUtils::GetMaxMSAASampleCount(physicalDevice);

        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        LOG_TRACE("Vulkan: VMA init");
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &vmaAllocator);

        texturePool.Init(1024);
        materialPool.Init(10000);
        renderPassPool.Init(128);
        framebufferPool.Init(128);
        pipelinePool.Init(128);
        descriptorSetLayoutPool.Init(128);


        for (uint32_t i = 0; i < 128; i++)
        {
            VulkanDescriptorSetLayout* descriptorSetLayout = descriptorSetLayoutPool.Get(i);
            *descriptorSetLayout = VulkanDescriptorSetLayout();
        }

        CreateCommandPool();
        CreateDescriptorPool();
        CreateCommandBuffers();
        CreateSyncObjects();
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

    VkResult VulkanDevice::PresentFrame(VkSwapchainKHR swapchain, const uint32_t imageIndex,
                                        const VkSemaphore* signalSemaphores) const
    {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;

        return vkQueuePresentKHR(presentQueue, &presentInfo);
    }

    VkResult VulkanDevice::SetupNextFrame(VkSwapchainKHR swapchain)
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        const VkResult result = vkAcquireNextImageKHR(device, swapchain,UINT64_MAX,
                                                      imageAvailableSemaphores[currentFrame],VK_NULL_HANDLE, &currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
            return VK_ERROR_OUT_OF_DATE_KHR;

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swapchain image.");

        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);

        return VK_SUCCESS;
    }

    void VulkanDevice::SetViewportAndScissor(VkExtent2D extent2D) const
    {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent2D.width);
        viewport.height = static_cast<float>(extent2D.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent2D;
        vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);
    }

    void VulkanDevice::SetViewportAndScissor(VkExtent2D extent, VkCommandBuffer commandBuffer) const
    {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    TextureHandle VulkanDevice::CreateTexture(const TextureCreateInfo& _createInfo)
    {
        VulkanTexture* texture = texturePool.Obtain();
        TextureHandle textureHandle = {texture->index};
        TextureCreateInfo createInfo = _createInfo;

        createInfo.mipLevels = createInfo.generateMipMaps
                                   ? static_cast<uint32_t>(std::floor(std::log2(std::max(createInfo.width, createInfo.height)))) + 1
                                   : createInfo.mipLevels;

        texture->allocatedImage = ImageBuilder(this)
                                  .SetFormat(createInfo.format)
                                  .SetResolution(createInfo.width, createInfo.height)
                                  .SetTiling(VK_IMAGE_TILING_OPTIMAL)
                                  .AddUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                  .AddUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                                  .AddUsage(ImageUtils::GetUsageFromFormat(createInfo.format))
                                  .SetInitialLayout(ImageUtils::GetLayoutFromFormat(createInfo.format))
                                  .SetMipLevels(createInfo.mipLevels)
                                  .SetArrayLayers(createInfo.arrayLayers)
                                  .SetFlags(createInfo.flags)
                                  .Build();


        VkImageViewType imageViewType = createInfo.isCubeMap
                                            ? VK_IMAGE_VIEW_TYPE_CUBE
                                            : createInfo.arrayLayers > 1
                                                  ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
                                                  : VK_IMAGE_VIEW_TYPE_2D;
        texture->imageView = ImageViewBuilder(this)
                             .SetFormat(createInfo.format)
                             .SetImage(texture->allocatedImage.image)
                             .SetViewType(imageViewType)
                             .SetAspectFlags(ImageUtils::GetAspectFlagFromFormat(createInfo.format))
                             .SetMipLevels(createInfo.mipLevels)
                             .SetBaseArrayLayer(0)
                             .SetLayerCount(createInfo.arrayLayers)
                             .Build();

        for (size_t i = 0; i < createInfo.arrayLayers; i++)
        {
            texture->arrayImageViews[i] = ImageViewBuilder(this)
                                          .SetFormat(createInfo.format)
                                          .SetImage(texture->allocatedImage.image)
                                          .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                                          .SetAspectFlags(ImageUtils::GetAspectFlagFromFormat(createInfo.format))
                                          .SetMipLevels(createInfo.mipLevels)
                                          .SetLayerCount(1)
                                          .SetBaseArrayLayer(i)
                                          .Build();
        }

        for (size_t miplevel = 0; miplevel < createInfo.mipLevels; miplevel++)
        {
            for (size_t faceindex = 0; faceindex < 6; faceindex++)
            {
                texture->mipmapImageViews[miplevel][faceindex] = ImageViewBuilder(this)
                                                                 .SetFormat(createInfo.format)
                                                                 .SetImage(texture->allocatedImage.image)
                                                                 .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
                                                                 .SetAspectFlags(ImageUtils::GetAspectFlagFromFormat(createInfo.format))
                                                                 .SetLayerCount(1)
                                                                 .SetBaseArrayLayer(faceindex)
                                                                 .SetBaseMipLevel(miplevel)
                                                                 .Build();
            }
        }

        texture->sampler = ImageSamplerBuilder(this)
                           .SetFilter(createInfo.filter, createInfo.filter)
                           .SetFormat(createInfo.format)
                           .SetMipLevels(createInfo.mipLevels)
                           .SetAddressMode(createInfo.addressMode)
                           .SetBorderColor(createInfo.borderColor)
                           .SetCompareOp(createInfo.compareEnabled, createInfo.compareOp)
                           .Build();

        texture->createInfo = createInfo;

        if (createInfo.data && createInfo.size > 0)
        {
            UploadTextureData(textureHandle, createInfo.data, createInfo.size);
        } else if (ImageUtils::GetLayoutFromFormat(createInfo.format) != VK_IMAGE_LAYOUT_UNDEFINED)
        {
            ImmediateSubmit([&](const VkCommandBuffer cmd) {
                VulkanUtils::TransitionImageLayout(cmd, texture->allocatedImage,
                                                   ImageUtils::GetAspectFlagFromFormat(createInfo.format),
                                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                                   ImageUtils::GetLayoutFromFormat(createInfo.format),
                                                   texture->createInfo.mipLevels);
            });
        }

        MakeBindlessTexture({texture->index});

        return textureHandle;
    }

    VulkanTexture* VulkanDevice::GetTexture(const TextureHandle textureHandle)
    {
        return texturePool.Get(textureHandle.handle);
    }

    void VulkanDevice::UploadTextureData(TextureHandle textureHandle, void* data, uint64_t size)
    {
        if (!data || size == 0) return;

        VulkanTexture* texture = GetTexture(textureHandle);
        const auto stagingBuffer = CreateBuffer(size,
                                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetData(), data, stagingBuffer.GetBufferSize());

        ImmediateSubmit([&](const VkCommandBuffer cmd) {
            VulkanUtils::TransitionImageLayout(cmd, texture->allocatedImage,
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_UNDEFINED,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               texture->createInfo.mipLevels);

            VulkanUtils::CopyBufferToImage(cmd, texture->allocatedImage.image,
                                           texture->createInfo.width,
                                           texture->createInfo.height,
                                           stagingBuffer.buffer);

            if (texture->createInfo.mipLevels > 1)
            {
                VulkanUtils::GenerateMipmaps(cmd,
                                             GetPhysicalDevice(),
                                             texture->allocatedImage,
                                             VulkanUtils::ConvertImageFormat(texture->createInfo.format),
                                             texture->createInfo.width,
                                             texture->createInfo.height,
                                             texture->createInfo.mipLevels);
            } else
            {
                VulkanUtils::TransitionImageLayout(cmd, texture->allocatedImage,
                                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                   texture->createInfo.mipLevels);
            }
        });

        DestroyBuffer(stagingBuffer);
    }

    void VulkanDevice::UploadCubemapTextureData(TextureHandle textureHandle, Bitmap* cubemap)
    {
        VulkanTexture* texture = GetTexture(textureHandle);
        const TextureCreateInfo info = texture->createInfo;

        const auto stagingBuffer = CreateBuffer(cubemap->pixelData.size(),
                                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                VMA_MEMORY_USAGE_CPU_ONLY);

        memcpy(stagingBuffer.GetData(), cubemap->pixelData.data(), cubemap->pixelData.size());

        std::vector<VkBufferImageCopy> bufferCopyRegions;
        for (uint32_t face = 0; face < 6; face++)
        {
            const uint64_t offset = Bitmap::GetImageOffsetForFace(*cubemap, face);
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent = {info.width, info.height, 1};
            bufferCopyRegion.bufferOffset = offset;
            bufferCopyRegions.push_back(bufferCopyRegion);
        }

        ImmediateSubmit([&](const VkCommandBuffer cmd) {
            VulkanUtils::TransitionImageLayout(cmd,
                                               texture->allocatedImage,
                                               VK_IMAGE_ASPECT_COLOR_BIT,
                                               VK_IMAGE_LAYOUT_UNDEFINED,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            vkCmdCopyBufferToImage(cmd,
                                   stagingBuffer.buffer,
                                   texture->allocatedImage.image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   6,
                                   bufferCopyRegions.data());

            VulkanUtils::GenerateCubemapMipmaps(cmd,
                                                GetPhysicalDevice(),
                                                texture->allocatedImage,
                                                VulkanUtils::ConvertImageFormat(info.format),
                                                info.width,
                                                info.height,
                                                info.mipLevels);
        });

        DestroyBuffer(stagingBuffer);
    }

    void VulkanDevice::MakeBindlessTexture(TextureHandle textureHandle)
    {
        const VulkanTexture* texture = GetTexture(textureHandle);
        const auto bindlessDescriptorSetLayout = GetDescriptorSetLayout(bindlessTexturesDescriptorSetLayoutHandle);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture->GetImageView();
        imageInfo.sampler = texture->GetSampler();

        VulkanDescriptorWriter(*bindlessDescriptorSetLayout, *bindlessDescriptorPool)
                .WriteImage(0, imageInfo, texture->index)
                .BuildOrOverwrite(bindlessTextureDescriptorSet);
    }

    void VulkanDevice::DestroyTexture(TextureHandle textureHandle)
    {
        if (textureHandle == INVALID_TEXTURE_HANDLE) return;
        frameDeletionQueue.Push([=] {
            VulkanTexture* texture = GetTexture(textureHandle);

            vkDestroySampler(device, texture->sampler, nullptr);

            for (size_t i = 0; i < texture->createInfo.arrayLayers; i++)
            {
                vkDestroyImageView(device, texture->arrayImageViews[i], nullptr);
            }

            vmaDestroyImage(vmaAllocator, texture->allocatedImage.image, texture->allocatedImage.allocation);

            texturePool.Release(texture);
        });
    }

    MaterialHandle VulkanDevice::CreateMaterial(const MaterialCreateInfo& info)
    {
        VulkanMaterial* material = materialPool.Obtain();

        MaterialParams params{};
        params.baseColor = info.baseColor;
        params.metallic = info.metallic;
        params.roughness = info.roughness;
        params.baseColorTextureIndex = info.baseColorTextureHandle.handle;
        params.normalMapTextureIndex = info.normalMapTextureHandle.handle;
        params.metallicRoughnessTextureIndex = info.metallicRoughnessTextureHandle.handle;
        params.alphaTested = info.isAlphaTested;

        material->params = params;

        // Get specific params location from buffer and write into that
        void* materialParams;

        vmaMapMemory(vmaAllocator, materialBuffer.allocation, &materialParams);
        static_cast<MaterialParams*>(materialParams)[material->index] = params;

        vmaUnmapMemory(vmaAllocator, materialBuffer.allocation);

        return {material->index};
    }

    VulkanMaterial* VulkanDevice::GetMaterial(MaterialHandle materialHandle)
    {
        ASSERT(materialHandle != INVALID_MATERIAL_HANDLE, "Invalid material handle");
        return materialPool.Get(materialHandle.handle);
    }

    void VulkanDevice::DestroyMaterial(MaterialHandle materialHandle)
    {
        if (materialHandle == INVALID_MATERIAL_HANDLE) return;
        frameDeletionQueue.Push([=] {
            VulkanMaterial* material = GetMaterial(materialHandle);

            DestroyBuffer(material->materialBuffer);
            materialPool.Release(material);
        });
    }

    RenderPassHandle VulkanDevice::CreateRenderPass(VulkanRenderPass::RenderPassConfig config)
    {
        VulkanRenderPass* renderPass = renderPassPool.Obtain();

        // Setup attachments
        uint32_t attachmentIndex = 0;
        std::vector<VkAttachmentReference> colorAttachmentsRefs;
        std::vector<VkAttachmentReference> depthAttachmentsRefs;
        std::vector<VkAttachmentDescription> attachmentDescriptions;

        for (size_t i = 0; i < config.numColorAttachments; i++)
        {
            VulkanRenderPass::ColorAttachment colorAttachment = config.colorAttachments[i];

            VkAttachmentDescription attachment{};
            attachment.format = VulkanUtils::ConvertImageFormat(colorAttachment.imageFormat);
            attachment.samples = colorAttachment.sampleCount;
            attachment.loadOp = convertLoadOpToVk(colorAttachment.loadOp);
            attachment.storeOp = convertStoreOpToVk(colorAttachment.storeOp);
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = colorAttachment.initialLayout;
            attachment.finalLayout = colorAttachment.isSwapchainAttachment
                                         ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                         : colorAttachment.finalLayout;

            attachmentDescriptions.push_back(attachment);

            VkAttachmentReference attachmentRef{};
            attachmentRef.attachment = attachmentIndex;
            attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachmentIndex++;
            colorAttachmentsRefs.push_back(attachmentRef);
        }
        if (config.depthAttachment.has_value())
        {
            VkAttachmentDescription attachment{};
            attachment.format = VulkanUtils::ConvertImageFormat(config.depthAttachment->depthFormat);
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = convertLoadOpToVk(config.depthAttachment->loadOp);
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = config.depthAttachment->loadOp == RenderPassOperation::LoadOp::Load
                                           ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                           : VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            attachmentDescriptions.push_back(attachment);

            VkAttachmentReference attachmentRef{};
            attachmentRef.attachment = attachmentIndex;
            attachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            depthAttachmentsRefs.push_back(attachmentRef);
        }

        // Build VkRenderPass
        {
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = colorAttachmentsRefs.size();
            subpass.pColorAttachments = colorAttachmentsRefs.data();
            subpass.pDepthStencilAttachment = depthAttachmentsRefs.data();

            std::array<VkSubpassDependency, 2> dependencies;

            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            VkRenderPassCreateInfo render_pass_info{};
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_info.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
            render_pass_info.pAttachments = attachmentDescriptions.data();
            render_pass_info.subpassCount = 1;
            render_pass_info.pSubpasses = &subpass;
            render_pass_info.dependencyCount = dependencies.size();
            render_pass_info.pDependencies = dependencies.data();

            VK_CHECK_MSG(
                vkCreateRenderPass(device, &render_pass_info, nullptr, &renderPass->renderPass),
                "Failed to create render pass.");
        }

        renderPass->config = config;
        return {renderPass->index};
    }

    void VulkanDevice::DestroyRenderPass(RenderPassHandle renderPassHandle)
    {
        if (renderPassHandle == INVALID_RENDER_PASS_HANDLE) return;
        frameDeletionQueue.Push([=] {
            VulkanRenderPass* renderPass = renderPassPool.Get(renderPassHandle.handle);
            vkDestroyRenderPass(device, renderPass->Get(), nullptr);
            renderPassPool.Release(renderPass);
        });
    }

    FramebufferHandle VulkanDevice::CreateFramebuffer(FramebufferCreateInfo info)
    {
        VulkanFramebuffer* framebuffer = framebufferPool.Obtain();

        std::vector<VkImageView> imageViews;

        for (auto& attachment: info.attachments)
        {
            if (attachment.imageView != VK_NULL_HANDLE)
                imageViews.push_back(attachment.imageView);

            if (attachment.textureHandle != INVALID_TEXTURE_HANDLE)
                imageViews.push_back(GetTexture(attachment.textureHandle)->GetImageView());
        }

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = renderPassPool.Get(info.renderPassHandle.handle)->Get();
        framebuffer_info.attachmentCount = static_cast<uint32_t>(imageViews.size());
        framebuffer_info.pAttachments = imageViews.data();
        framebuffer_info.width = info.resolution.width;
        framebuffer_info.height = info.resolution.height;
        framebuffer_info.layers = 1;

        VkFramebuffer vkFramebuffer = VK_NULL_HANDLE;
        VK_CHECK_MSG(vkCreateFramebuffer(GetDevice(), &framebuffer_info, nullptr, &vkFramebuffer),
                     "Failed to create framebuffer.");

        framebuffer->framebuffer = vkFramebuffer;
        framebuffer->attachmentCount = imageViews.size();
        framebuffer->extent = info.resolution;
        std::copy_n(std::make_move_iterator(imageViews.begin()), imageViews.size(), framebuffer->attachments.begin());

        return {framebuffer->index};
    }

    VulkanFramebuffer* VulkanDevice::GetFramebuffer(FramebufferHandle framebufferHandle)
    {
        return framebufferPool.Get(framebufferHandle.handle);
    }

    void VulkanDevice::DestroyFramebuffer(FramebufferHandle framebufferHandle)
    {
        if (framebufferHandle == INVALID_FRAMEBUFFER_HANDLE) return;
        frameDeletionQueue.Push([=] {
            LOG_INFO("Destroy framebuffer");
            VulkanFramebuffer* framebuffer = framebufferPool.Get(framebufferHandle.handle);

            vkDestroyFramebuffer(GetDevice(), framebuffer->framebuffer, nullptr);
            framebufferPool.Release(framebuffer);
        });
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

        VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
        descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        descriptorIndexingFeatures.pNext = nullptr;
        descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
        descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

        VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
        bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        bufferDeviceAddressFeatures.pNext = &descriptorIndexingFeatures;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &bufferDeviceAddressFeatures;

        // Fetch all features
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);


        VkDeviceCreateInfo createInfo{};
        createInfo.pQueueCreateInfos = &queue_create_info;
        createInfo.queueCreateInfoCount = 1;
        //createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &deviceFeatures2;

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

    uint32_t VulkanDevice::GetDrawCallCount() const
    {
        return prevDrawCallCount;
    }


    VkInstance VulkanDevice::CreateVkInstance(const std::vector<const char*>& deviceExtensions,
                                              const std::vector<const char*>& validationLayers)
    {
        VulkanUtils::VulkanVersion vkVersion;
        VulkanUtils::GetInstanceVersion(vkVersion);

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

        vkApplicationInfo->apiVersion = VK_MAKE_API_VERSION(0, vkVersion.major, vkVersion.minor, 0);

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
                            .SetPoolFlags(
                                VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT)
                            .Build();

        shaderDescriptorPool = VulkanDescriptorPool::Builder(this)
                               .SetMaxSets(100)
                               .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100)
                               .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
                               .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100)
                               .SetPoolFlags(
                                   VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT)
                               .Build();

        imguiDescriptorPool = VulkanDescriptorPool::Builder(this)
                              .SetMaxSets(100)
                              .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100)
                              .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                              .Build();

        bindlessDescriptorPool = VulkanDescriptorPool::Builder(this)
                                 .SetMaxSets(2 * MAX_BINDLESS_RESOURCES)
                                 .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_RESOURCES)
                                 .AddPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_BINDLESS_RESOURCES)
                                 .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT)
                                 .Build();

        // Bindless Textures
        {
            bindlessTexturesDescriptorSetLayoutHandle = VulkanDescriptorSetLayoutBuilder(this)
                                                        .AddBinding({
                                                                        0, DescriptorSetBindingType::TextureSampler,
                                                                        {ShaderStage::VertexShader, ShaderStage::FragmentShader}
                                                                    },
                                                                    MAX_BINDLESS_RESOURCES)
                                                        .AddBinding({
                                                                        1, DescriptorSetBindingType::StorageImage,
                                                                        {ShaderStage::VertexShader, ShaderStage::FragmentShader}
                                                                    },
                                                                    MAX_BINDLESS_RESOURCES)
                                                        .Build();

            auto descriptorSetLayout = GetDescriptorSetLayout(bindlessTexturesDescriptorSetLayoutHandle);
            VulkanDescriptorWriter(*descriptorSetLayout, *bindlessDescriptorPool)
                    .Build(bindlessTextureDescriptorSet);
        }

        // Bindless materials
        {
            const uint64_t bufferSize = sizeof(MaterialParams) * MAX_OBJECTS;

            materialBuffer = CreateBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            materialsDescriptorSetLayoutHandle = VulkanDescriptorSetLayoutBuilder(this)
                                                 .AddBinding({0, DescriptorSetBindingType::StorageBuffer, {ShaderStage::FragmentShader}})
                                                 .Build();

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = materialBuffer.buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = bufferSize;

            auto descriptorSetLayout = GetDescriptorSetLayout(materialsDescriptorSetLayoutHandle);
            VulkanDescriptorWriter(*descriptorSetLayout, *shaderDescriptorPool)
                    .WriteBuffer(0, bufferInfo)
                    .Build(materialDescriptorSet);
        }
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

    AllocatedBuffer VulkanDevice::CreateBuffer(uint64_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
    {
        LOG_TRACE("Allocate buffer: " + std::to_string(size / 1024) + " kB");

        AllocatedBuffer allocatedBuffer;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage = memoryUsage;
        vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VK_CHECK(vmaCreateBuffer(vmaAllocator,
            &bufferInfo,
            &vmaallocInfo,
            &allocatedBuffer.buffer,
            &allocatedBuffer.allocation,
            &allocatedBuffer.info));

        VkBufferDeviceAddressInfo deviceAdressInfo{};
        deviceAdressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        deviceAdressInfo.buffer = allocatedBuffer.buffer;

        allocatedBuffer.address = vkGetBufferDeviceAddress(device, &deviceAdressInfo);

        return allocatedBuffer;
    }

    void VulkanDevice::DestroyBuffer(AllocatedBuffer buffer)
    {
        frameDeletionQueue.Push([=] {
            vmaDestroyBuffer(vmaAllocator, buffer.buffer, buffer.allocation);
        });
    }

    void VulkanDevice::CopyBuffer(const AllocatedBuffer& src, const AllocatedBuffer& dst)
    {
        LOG_INFO("COPY BUFFER");
        LOG_WARN("Src: " + std::to_string(src.GetBufferSize()));
        LOG_WARN("Dst: " + std::to_string(dst.GetBufferSize()));

        ImmediateSubmit([&](VkCommandBuffer commandBuffer) {
            VkBufferCopy copy_region;
            copy_region.srcOffset = 0;
            copy_region.dstOffset = 0;
            copy_region.size = src.GetBufferSize();
            vkCmdCopyBuffer(commandBuffer, src.buffer, dst.buffer, 1, &copy_region);
        });
    }

    PipelineHandle VulkanDevice::CreatePipeline()
    {
        const VulkanPipeline* pipeline = pipelinePool.Obtain();

        return {pipeline->index};
    }

    VulkanPipeline* VulkanDevice::GetPipeline(PipelineHandle pipelineHandle)
    {
        return pipelinePool.Get(pipelineHandle.handle);
    }

    void VulkanDevice::DestroyPipeline(PipelineHandle pipelineHandle)
    {
        if (pipelineHandle == INVALID_PIPELINE_HANDLE) return;
        frameDeletionQueue.Push([=] {
            VulkanPipeline* pipeline = pipelinePool.Get(pipelineHandle.handle);

            vkDestroyShaderModule(GetDevice(), pipeline->vertexShaderModule, nullptr);
            vkDestroyShaderModule(GetDevice(), pipeline->fragmentShaderModule, nullptr);
            vkDestroyPipeline(GetDevice(), pipeline->pipeline, nullptr);
            vkDestroyPipelineLayout(GetDevice(), pipeline->pipelineLayout, nullptr);

            pipelinePool.Release(pipeline);
        });
    }

    DescriptorSetLayoutHandle VulkanDevice::CreateDescriptorSetLayout(DescriptorSetLayoutCreateInfo& info)
    {
        VulkanDescriptorSetLayout* descriptorSetLayout = descriptorSetLayoutPool.Obtain();

        descriptorSetLayout->bindingCount = info.bindings.size();

        for (auto [bindingIndex, layoutBinding]: info.bindings)
            descriptorSetLayout->bindings[bindingIndex] = layoutBinding;

        for (auto [bindingIndex, flags]: info.bindingFlags)
            descriptorSetLayout->bindingFlags[bindingIndex] = flags;

        VkDescriptorSetLayoutBindingFlagsCreateInfo layoutBindingFlags{};
        layoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        layoutBindingFlags.bindingCount = descriptorSetLayout->bindingCount;
        layoutBindingFlags.pBindingFlags = descriptorSetLayout->bindingFlags.data();

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = descriptorSetLayout->bindingCount;
        descriptorSetLayoutInfo.pBindings = descriptorSetLayout->bindings.data();
        descriptorSetLayoutInfo.pNext = &layoutBindingFlags;
        descriptorSetLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

        VK_CHECK_MSG(vkCreateDescriptorSetLayout(GetDevice(), &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout->descriptorSetLayout),
                     "Failed to create descriptor set layout.");

        return {descriptorSetLayout->index};
    }

    VulkanDescriptorSetLayout* VulkanDevice::GetDescriptorSetLayout(DescriptorSetLayoutHandle descriptorSetLayoutHandle)
    {
        return descriptorSetLayoutPool.Get(descriptorSetLayoutHandle.handle);
    }

    void VulkanDevice::DestroyDescriptorSetLayout(DescriptorSetLayoutHandle descriptorSetLayoutHandle)
    {
        if (descriptorSetLayoutHandle == INVALID_DESCRIPTOR_SET_LAYOUT_HANDLE) return;
        frameDeletionQueue.Push([=] {
            VulkanDescriptorSetLayout* descriptorSetLayout = descriptorSetLayoutPool.Get(descriptorSetLayoutHandle.handle);
            if (descriptorSetLayout && descriptorSetLayout->descriptorSetLayout)
            {
                vkDestroyDescriptorSetLayout(GetDevice(), descriptorSetLayout->descriptorSetLayout, nullptr);
                descriptorSetLayoutPool.Release(descriptorSetLayout);
            }
        });
    }
}
