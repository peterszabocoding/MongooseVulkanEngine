#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <set>
#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>


#define VK_CHECK(x,msg)                                                     \
do                                                                          \
    {                                                                       \
        VkResult err = x;                                                   \
        if (err)                                                            \
        {                                                                   \
            std::cout << msg << std::endl;                                  \
            std::cout <<"Detected Vulkan error: " << err << std::endl;      \
            abort();                                                        \
        }                                                                   \
} while (0)

#define VK_CHECK(x)                                                         \
do                                                                          \
    {                                                                       \
        VkResult err = x;                                                   \
        if (err)                                                            \
        {                                                                   \
            std::cout <<"Detected Vulkan error: " << err << std::endl;      \
            abort();                                                        \
        }                                                                   \
} while (0)

namespace Raytracing::VulkanUtils
{
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool IsComplete() const { return graphicsFamily.has_value(); }
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << '\n';
        return VK_FALSE;
    }

    static std::string GetVkResultString(const VkResult vulkan_result)
    {
        switch (vulkan_result)
        {
            case VK_SUCCESS:
                return "SUCCESS";
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "OUT OF HOST MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "OUT OF DEVICE MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED:
                return "INITIALIZATION FAILED";
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "LAYER NOT PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "EXTENSION NOT PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "INCOMPATIBLE DRIVER";
            default:
                return "UNKNOWN RESULT '" + std::to_string(vulkan_result);
        }
    }

    static std::vector<VkExtensionProperties> GetAvailableExtensions()
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

    static bool CheckIfExtensionSupported(const char* ext)
    {
        const auto available_extensions = VulkanUtils::GetAvailableExtensions();
        for (const auto& extension: available_extensions)
        {
            if (strcmp(extension.extensionName, ext) == 0) return true;
        }
        return false;
    }

    static std::vector<VkExtensionProperties> GetSupportedDeviceExtensions(const VkPhysicalDevice physicalDevice)
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extension_count, available_extensions.data());

        std::cout << "Supported Device Extensions:" << '\n';
        for (auto [extensionName, specVersion]: available_extensions) std::cout << extensionName << '\n';

        return available_extensions;
    }

    static std::vector<VkLayerProperties> GetSupportedValidationLayers()
    {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        std::cout << "available validation layers:\n";
        for (const auto& layer: available_layers) std::cout << '\t' << layer.layerName << '\n';

        return available_layers;
    }

    static bool CheckIfValidationLayerSupported(const char* layer)
    {
        const auto validation_layers = GetSupportedValidationLayers();
        for (const auto& lay: validation_layers)
        {
            if (strcmp(lay.layerName, layer) == 0) return true;
        }
        return false;
    }

    static bool CheckDeviceExtensionSupport(const VkPhysicalDevice physicalDevice, std::vector<std::string>& deviceExtensions)
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

    static VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo()
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

    static QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
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

    static bool IsDeviceSuitable(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
    {
        const QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

        return indices.IsComplete() && supportedFeatures.samplerAnisotropy;;
    }

    static VkShaderModule CreateShaderModule(const VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shader_module;

        VK_CHECK(
            vkCreateShaderModule(device, &create_info, nullptr, &shader_module),
            "Failed to create shader module.");

        return shader_module;
    }

    static uint32_t FindMemoryType(const VkPhysicalDevice physicalDevice, const uint32_t typeFilter,
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

    static VkCommandBuffer BeginSingleTimeCommands(const VkDevice device, const VkCommandPool commandPool)
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

        return commandBuffer;
    }

    static void EndSingleTimeCommands(const VkDevice device, const VkCommandPool commandPool, const VkQueue submitQueue,
                                      const VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(submitQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(submitQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    static VkImageView CreateImageView(const VkDevice device, const VkImage image, const VkFormat format, VkImageAspectFlags aspectFlags)
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
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }
}
