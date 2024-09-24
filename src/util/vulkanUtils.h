#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <vulkan/vulkan.h>

#include "application/application.h"

namespace Raytracing {

    namespace VulkanUtils {

        std::string GetVkResultString(VkResult vulkan_result)
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

        std::vector<VkExtensionProperties> GetAvailableExtensions()
        {
            // List Vulkan available extensions
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

            std::cout << "available extensions:\n";
            for (const auto& extension : availableExtensions) std::cout << '\t' << extension.extensionName << '\n';

            return availableExtensions;
        }

        bool CheckIfExtensionSupported(const char* ext)
        {
            auto availableExtensions = GetAvailableExtensions();
            for (const auto& extension : availableExtensions) {
                if (strcmp(extension.extensionName, ext) == 0) return true;
            }
            return false;
        }
    
        class VulkanInstanceBuilder
        {
            public:
                VulkanInstanceBuilder(){}

                VulkanInstanceBuilder& ApplicationInfo(const AppInfo& appInfo)
                {
                    vkApplicationInfo = new VkApplicationInfo();
                    vkApplicationInfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                    vkApplicationInfo->pApplicationName = appInfo.windowTitle.c_str();
                    vkApplicationInfo->pEngineName = "No Engine";

                    vkApplicationInfo->applicationVersion = VK_MAKE_VERSION(appInfo.appVersion.major, appInfo.appVersion.minor, appInfo.appVersion.patch);
                    vkApplicationInfo->engineVersion = VK_MAKE_VERSION(appInfo.appVersion.major, appInfo.appVersion.minor, appInfo.appVersion.patch);
                    vkApplicationInfo->apiVersion = VK_API_VERSION_1_0;

                    createInfo.pApplicationInfo = vkApplicationInfo;
                    return *this;
                }

                VulkanInstanceBuilder& AddDeviceExtension(const char* deviceExtension)
                {
                    deviceExtensions.push_back(deviceExtension);
                    return *this;
                }

                VulkanInstanceBuilder& AddDeviceExtensions(std::vector<const char*> deviceExtensionList)
                {
                    for (auto ext : deviceExtensionList)
                        deviceExtensions.push_back(ext);
                    return *this;
                }

                VulkanInstanceBuilder& AddDeviceExtensions(const char** ext, int count)
                {
                    for (int i = 0; i < count; i++)
                        deviceExtensions.push_back(ext[i]);
                    return *this;
                }

                VulkanInstanceBuilder& AddValidationLayer(const char* validationLayer)
                {
                    validationLayers.push_back(validationLayer);
                    return *this;
                }

                VulkanInstanceBuilder& AddValidationLayers(std::vector<const char*> validationLayerList)
                {
                    for (auto val : validationLayerList)
                        validationLayers.push_back(val);
                    return *this;
                }

                VkInstance Build()
                {
                    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

                    createInfo.enabledExtensionCount =  deviceExtensions.size();
                    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

                    createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
                    createInfo.enabledLayerCount = validationLayers.size();
                    createInfo.ppEnabledLayerNames = validationLayers.data();

                    VkInstance instance;
                    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
                    if (result != VK_SUCCESS)
                    {
                        std::string message = "ERROR: Instance creation failed with result '";
                        message += VulkanUtils::GetVkResultString(result);
                        message += "'.";
                        throw std::runtime_error(message);
                    }

                    return instance;
                }

            private:
                VkApplicationInfo* vkApplicationInfo;

                VkInstanceCreateInfo createInfo;
                std::vector<const char*> deviceExtensions;
                std::vector<const char*> validationLayers;
        };
    
    }

}