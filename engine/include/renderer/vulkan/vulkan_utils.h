#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <set>
#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>

#include "util/core.h"
#include "resource/resource.h"

#define VK_CHECK_MSG(x,msg)                                                                                 \
do                                                                                                          \
    {                                                                                                       \
        VkResult err = x;                                                                                   \
        if (err)                                                                                            \
        {                                                                                                   \
            std::cout << msg << std::endl;                                                                  \
            std::cout <<"Detected Vulkan error: " << VulkanUtils::GetVkResultString(err) << std::endl;      \
            abort();                                                                                        \
        }                                                                                                   \
} while (0)

#define VK_CHECK(x)                                                                                         \
do                                                                                                          \
    {                                                                                                       \
        VkResult err = x;                                                                                   \
        if (err)                                                                                            \
        {                                                                                                   \
            std::cout <<"Detected Vulkan error: " << VulkanUtils::GetVkResultString(err) << std::endl;      \
            abort();                                                                                        \
        }                                                                                                   \
} while (0)

namespace MongooseVK::VulkanUtils
{
    struct VulkanVersion {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool IsComplete() const { return graphicsFamily.has_value(); }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct CopyParams {
        uint32_t srcMipLevel;
        uint32_t dstMipLevel;
        uint32_t srcBaseArrayLayer;
        uint32_t dstBaseArrayLayer;

        uint32_t regionWidth;
        uint32_t regionHeight;
    };

    inline VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';
        return VK_FALSE;
    }

    inline VkFormat ConvertImageFormat(const ImageFormat imageFormat)
    {
        switch (imageFormat)
        {
            case ImageFormat::R8_UNORM:
                return VK_FORMAT_R8_UNORM;

            case ImageFormat::R8_SNORM:
                return VK_FORMAT_R8_SNORM;

            case ImageFormat::R8_UINT:
                return VK_FORMAT_R8_UINT;

            case ImageFormat::R8_SINT:
                return VK_FORMAT_R8_SINT;

            case ImageFormat::R16_UNORM:
                return VK_FORMAT_R16_UNORM;

            case ImageFormat::R16_SNORM:
                return VK_FORMAT_R16_SNORM;

            case ImageFormat::R16_UINT:
                return VK_FORMAT_R16_UINT;

            case ImageFormat::R16_SINT:
                return VK_FORMAT_R16_SINT;

            case ImageFormat::R32_SFLOAT:
                return VK_FORMAT_R32_SFLOAT;

            case ImageFormat::R32_UINT:
                return VK_FORMAT_R32_UINT;

            case ImageFormat::R32_SINT:
                return VK_FORMAT_R32_SINT;

            case ImageFormat::RGB8_UNORM:
                return VK_FORMAT_R8G8B8_UNORM;

            case ImageFormat::RGB8_SRGB:
                return VK_FORMAT_R8G8B8_SRGB;

            case ImageFormat::RGBA8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;

            case ImageFormat::RGBA8_SRGB:
                return VK_FORMAT_R8G8B8A8_SRGB;

            case ImageFormat::RGB16_UNORM:
                return VK_FORMAT_R16G16B16_UNORM;

            case ImageFormat::RGB16_SFLOAT:
                return VK_FORMAT_R16G16B16_SFLOAT;

            case ImageFormat::RGBA16_UNORM:
                return VK_FORMAT_R16G16B16A16_UNORM;

            case ImageFormat::RGBA16_SFLOAT:
                return VK_FORMAT_R16G16B16A16_SFLOAT;

            case ImageFormat::RGB32_SFLOAT:
                return VK_FORMAT_R32G32B32_SFLOAT;

            case ImageFormat::RGB32_UINT:
                return VK_FORMAT_R32G32B32_UINT;

            case ImageFormat::RGB32_SINT:
                return VK_FORMAT_R32G32B32_SINT;

            case ImageFormat::RGBA32_SFLOAT:
                return VK_FORMAT_R32G32B32A32_SFLOAT;

            case ImageFormat::RGBA32_UINT:
                return VK_FORMAT_R32G32B32A32_UINT;

            case ImageFormat::RGBA32_SINT:
                return VK_FORMAT_R32G32B32A32_SINT;

            case ImageFormat::DEPTH24_STENCIL8:
                return VK_FORMAT_D24_UNORM_S8_UINT;

            case ImageFormat::DEPTH32:
                return VK_FORMAT_D32_SFLOAT;

            default:
                ASSERT(false, "Unknown image format");
        }
        return VK_FORMAT_UNDEFINED;
    }


    inline ImageFormat ConvertVulkanFormat(const VkFormat format)
    {
        switch (format)
        {
            case VK_FORMAT_R8G8B8_UNORM:
                return ImageFormat::RGB8_UNORM;

            case VK_FORMAT_R8G8B8_SRGB:
                return ImageFormat::RGB8_SRGB;

            case VK_FORMAT_R8G8B8A8_UNORM:
                return ImageFormat::RGBA8_UNORM;

            case VK_FORMAT_R8G8B8A8_SRGB:
                return ImageFormat::RGBA8_SRGB;

            case VK_FORMAT_R16G16B16_UNORM:
                return ImageFormat::RGB16_UNORM;

            case VK_FORMAT_R16G16B16_SFLOAT:
                return ImageFormat::RGB16_SFLOAT;

            case VK_FORMAT_R16G16B16A16_UNORM:
                return ImageFormat::RGBA16_UNORM;

            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return ImageFormat::RGBA16_SFLOAT;

            case VK_FORMAT_R32G32B32_SFLOAT:
                return ImageFormat::RGB32_SFLOAT;

            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return ImageFormat::RGBA32_SFLOAT;

            case VK_FORMAT_D24_UNORM_S8_UINT:
                return ImageFormat::DEPTH24_STENCIL8;

            case VK_FORMAT_D32_SFLOAT:
                return ImageFormat::DEPTH32;

            default:
                ASSERT(false, "Unknown image format");
        }
        return ImageFormat::Unknown;
    }


    inline std::string GetVkResultString(const VkResult vulkan_result)
    {
        switch (vulkan_result)
        {
            case VK_SUCCESS:
                return "VK_SUCCESS";
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED:
                return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_DEVICE_LOST:
                return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                return "VK_ERROR_OUT_OF_POOL_MEMORY";
            default:
                return "UNKNOWN RESULT '" + std::to_string(vulkan_result);
        }
    }

    inline std::vector<VkExtensionProperties> GetAvailableExtensions()
    {
        // List Vulkan available extensions
        uint32_t extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, available_extensions.data());

        std::cout << "available extensions:\n";
        for (const auto& extension: available_extensions)
            std::cout << '\t' << extension.extensionName << '\n';

        return available_extensions;
    }

    inline bool CheckIfExtensionSupported(const char* ext)
    {
        const auto available_extensions = VulkanUtils::GetAvailableExtensions();
        for (const auto& extension: available_extensions)
        {
            if (strcmp(extension.extensionName, ext) == 0) return true;
        }
        return false;
    }

    inline std::vector<VkExtensionProperties> GetSupportedDeviceExtensions(const VkPhysicalDevice physicalDevice)
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, available_extensions.data());

        std::cout << "Supported Device Extensions:" << '\n';
        for (auto [extensionName, specVersion]: available_extensions) std::cout << extensionName << '\n';

        return available_extensions;
    }

    inline void GetInstanceVersion(VulkanVersion& version)
    {
        uint32_t instanceVersion = 0;
        VkResult result = vkEnumerateInstanceVersion(&instanceVersion);
        VK_CHECK_MSG(result, "Failed to get instance version");

        version.major = VK_API_VERSION_MAJOR(instanceVersion);
        version.minor = VK_API_VERSION_MINOR(instanceVersion);
        version.patch = VK_API_VERSION_PATCH(instanceVersion);

        LOG_INFO("Vulkan version: {0}.{1}.{2}", version.major, version.minor, version.patch);
    }

    inline std::vector<VkLayerProperties> GetSupportedValidationLayers()
    {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        std::cout << "available validation layers:\n";
        for (const auto& layer: available_layers) std::cout << '\t' << layer.layerName << '\n';

        return available_layers;
    }

    inline bool CheckIfValidationLayerSupported(const char* layer)
    {
        const auto validation_layers = GetSupportedValidationLayers();
        for (const auto& lay: validation_layers)
        {
            if (strcmp(lay.layerName, layer) == 0) return true;
        }
        return false;
    }

    inline bool CheckDeviceExtensionSupport(const VkPhysicalDevice physicalDevice, std::vector<std::string>& deviceExtensions)
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, available_extensions.data());

        std::set required_extensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& [extensionName, specVersion]: available_extensions)
        {
            required_extensions.erase(extensionName);
        }

        return required_extensions.empty();
    }

    inline VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};

        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        createInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        createInfo.pfnUserCallback = debugCallback;

        return createInfo;
    }

    inline QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, queue_families.data());

        int i = 0;
        for (const auto& queueFamily: queue_families)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = 0;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

            if (presentSupport)
                indices.presentFamily = i;

            if (indices.IsComplete())
                break;

            i++;
        }

        return indices;
    }

    inline bool IsDeviceSuitable(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
    {
        const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

        return indices.IsComplete() && supportedFeatures.samplerAnisotropy;
    }

    inline VkShaderModule CreateShaderModule(const VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shader_module;

        VK_CHECK_MSG(
            vkCreateShaderModule(device, &create_info, nullptr, &shader_module),
            "Failed to create shader module.");

        return shader_module;
    }

    inline VkShaderModule CreateShaderModule(const VkDevice device, const std::vector<uint32_t>& code)
    {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = 4 * code.size();
        create_info.pCode = code.data();

        VkShaderModule shader_module;

        VK_CHECK_MSG(
            vkCreateShaderModule(device, &create_info, nullptr, &shader_module),
            "Failed to create shader module.");

        return shader_module;
    }

    inline uint32_t FindMemoryType(const VkPhysicalDevice physicalDevice, const uint32_t typeFilter,
                                   const VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mem_properties);

        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    inline VkImageView CreateImageView(const VkDevice device, const VkImage image, const VkFormat format,
                                       const VkImageAspectFlags aspectFlags)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        VK_CHECK_MSG(vkCreateImageView(device, &viewInfo, nullptr, &imageView), "Failed to create texture image view.");

        return imageView;
    }

    inline SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
    {
        uint32_t formatCount;
        uint32_t presentModeCount;
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    inline VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat: availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    inline uint32_t GetSwapchainImageCount(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
    {
        SwapChainSupportDetails support = QuerySwapChainSupport(physicalDevice, surface);
        uint32_t imageCount = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
        {
            imageCount = support.capabilities.maxImageCount;
        }

        return imageCount;
    }

    inline void SetSrcAccessFlag(const VkImageLayout oldLayout, VkImageMemoryBarrier& imageBarrier)
    {
        switch (oldLayout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                imageBarrier.srcAccessMask = 0;
                break;
            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                imageBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                throw std::invalid_argument("Unsupported layout type");
        }
    }

    inline void SetDstAccessFlag(const VkImageLayout newLayout, VkImageMemoryBarrier& imageBarrier)
    {
        switch (newLayout)
        {
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                imageBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
                imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                imageBarrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
                imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                imageBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                imageBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                imageBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                imageBarrier.srcAccessMask |= VK_ACCESS_HOST_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
                imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            default:
                throw std::invalid_argument("Unsupported layout type");
        }
    }

    inline void TransitionImageLayout(const VkCommandBuffer commandBuffer,
                                      const VkImage image,
                                      const VkImageAspectFlags aspectFlags,
                                      const VkImageLayout oldLayout,
                                      const VkImageLayout newLayout,
                                      const uint32_t mipLevels = 1)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspectFlags;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        SetSrcAccessFlag(oldLayout, barrier);
        SetDstAccessFlag(newLayout, barrier);

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout ==
                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL && newLayout ==
                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        } else
        {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    inline void InsertImageMemoryBarrier(const VkCommandBuffer cmd, const VkImage image,
                                         const VkAccessFlags srcAccessMask,
                                         const VkAccessFlags dstAccessMask,
                                         const VkImageLayout oldImageLayout,
                                         const VkImageLayout newImageLayout,
                                         const VkPipelineStageFlags srcStageMask,
                                         const VkPipelineStageFlags dstStageMask,
                                         const VkImageSubresourceRange& subresourceRange)
    {
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageMemoryBarrier.srcAccessMask = srcAccessMask;
        imageMemoryBarrier.dstAccessMask = dstAccessMask;
        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = subresourceRange;

        vkCmdPipelineBarrier(cmd, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }

    inline void InsertPipelineBarrier(const VkCommandBuffer cmd, const VkPipelineStageFlags srcStageMask,
                                      const VkPipelineStageFlags dstStageMask)
    {
        vkCmdPipelineBarrier(cmd, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 0, nullptr);
    }

    inline void CopyBufferToImage(const VkCommandBuffer commandBuffer, const VkImage image, const uint32_t width, const uint32_t height,
                                  const VkBuffer buffer)
    {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
    }

    inline void CopyImage(const VkCommandBuffer commandBuffer, const VkImage srcImage, const VkImage dstImage, CopyParams params)
    {
        TransitionImageLayout(commandBuffer, srcImage,
                              VK_IMAGE_ASPECT_COLOR_BIT,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              1);

        VkImageCopy copyRegion{};

        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.baseArrayLayer = params.srcBaseArrayLayer;
        copyRegion.srcSubresource.mipLevel = params.srcMipLevel;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = {0, 0, 0};

        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.baseArrayLayer = params.dstBaseArrayLayer;
        copyRegion.dstSubresource.mipLevel = params.dstMipLevel;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = {0, 0, 0};

        copyRegion.extent.width = params.regionWidth;
        copyRegion.extent.height = params.regionHeight;
        copyRegion.extent.depth = 1;

        vkCmdCopyImage(commandBuffer,
                       srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &copyRegion);

        TransitionImageLayout(commandBuffer, srcImage,
                              VK_IMAGE_ASPECT_COLOR_BIT,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                              1);
    }

    inline VkDeviceMemory AllocateImageMemory(const VkDevice device, const VkPhysicalDevice physicalDevice, const VkImage image,
                                              const VkMemoryPropertyFlags properties)
    {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

        VkDeviceMemory imageMemory;
        VK_CHECK_MSG(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory), "Failed to allocate image memory.");
        vkBindImageMemory(device, image, imageMemory, 0);

        return imageMemory;
    }

    inline void GenerateMipmaps(const VkCommandBuffer commandBuffer,
                                const VkPhysicalDevice physicalDevice,
                                const VkImage image,
                                const VkFormat imageFormat,
                                const int32_t width,
                                const int32_t height,
                                const uint32_t mipLevels)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            throw std::runtime_error("Texture image format does not support linear blitting.");
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = width;
        int32_t mipHeight = height;

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {
                mipWidth > 1 ? mipWidth / 2 : 1,
                mipHeight > 1 ? mipHeight / 2 : 1,
                1
            };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &blit,
                           VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);
    }

    inline void GenerateCubemapMipmaps(const VkCommandBuffer commandBuffer,
                                       VkPhysicalDevice physicalDevice,
                                       const VkImage image,
                                       VkFormat imageFormat,
                                       const int32_t width,
                                       const int32_t height,
                                       const uint32_t mipLevels)
    {
        for (size_t face = 0; face < 6; face++)
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = face;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mipWidth = width;
            int32_t mipHeight = height;

            for (uint32_t i = 1; i < mipLevels; i++)
            {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = face;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {
                    mipWidth > 1 ? mipWidth / 2 : 1,
                    mipHeight > 1 ? mipHeight / 2 : 1,
                    1
                };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = face;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(commandBuffer,
                               image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &blit,
                               VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }
    }

    inline VkSampleCountFlagBits GetMaxMSAASampleCount(const VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        const VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

        if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
        if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
        if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
        if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
        if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
        if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;

        return VK_SAMPLE_COUNT_1_BIT;
    }
}
