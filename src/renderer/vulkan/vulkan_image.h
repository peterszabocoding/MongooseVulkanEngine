#pragma once
#include <vulkan/vulkan_core.h>

#include "resource/resource.h"
#include "util/core.h"

namespace Raytracing
{
    class VulkanDevice;
    class VulkanImageView;

    class VulkanImage {
    public:
        VulkanImage(VulkanDevice* device, const VkImage image, Ref<VulkanImageView> imageView): device(device), image(image),
            imageView(imageView) {}

        virtual ~VulkanImage();

        VkImage GetImage() const { return image; };
        Ref<VulkanImageView> GetImageView() const { return imageView; };

    protected:
        VulkanDevice* device;
        VkImage image{};
        Ref<VulkanImageView> imageView{};
    };

    class VulkanTextureImage : public VulkanImage {
    public:
        VulkanTextureImage(VulkanDevice* device,
                           const VkImage image, const
                           VkDeviceMemory memory,
                           const Ref<VulkanImageView>& imageView,
                           const VkSampler sampler): VulkanImage(device, image, imageView),
                                                     imageMemory(memory),
                                                     sampler(sampler) {}

        ~VulkanTextureImage() override;

        void SetImageResource(const ImageResource& imageResource)
        {
            this->imageResource = imageResource;
        }

        VkSampler GetSampler() const
        {
            return sampler;
        }

    private:
        ImageResource imageResource{};
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
    };

    class VulkanDepthImage : public VulkanImage {
    public:
        VulkanDepthImage(VulkanDevice* device,
                         const VkImage image, const
                         VkDeviceMemory memory,
                         const Ref<VulkanImageView>& imageView,
                         const VkSampler sampler): VulkanImage(device, image, imageView),
                                                   imageMemory(memory),
                                                   sampler(sampler) {}

        ~VulkanDepthImage() override;


        void SetImageResource(const ImageResource& imageResource)
        {
            this->imageResource = imageResource;
        }

        VkSampler GetSampler() const
        {
            return sampler;
        }

    private:
        ImageResource imageResource{};
        VkDeviceMemory imageMemory{};
        VkSampler sampler{};
    };

    class VulkanImageBuilder {
    public:
        VulkanImageBuilder() = default;
        virtual ~VulkanImageBuilder() = default;

    protected:
        static VkDeviceMemory AllocateImageMemory(const VulkanDevice* device, VkImage image, VkMemoryPropertyFlags properties);

        VkImage CreateImage(const VulkanDevice* device, VkImageUsageFlags usage) const;
        Ref<VulkanImageView> CreateImageView(VulkanDevice* device, VkImageAspectFlags aspectFlags) const;
        VkSampler CreateSampler(const VulkanDevice* device) const;

        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) const;
        void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer) const;

    protected:
        void* data;
        uint64_t size;
        uint32_t width;
        uint32_t height;
        VkFilter minFilter;
        VkFilter magFilter;
        VkImageTiling tiling;
        VkFormat format;

        VkImage image{};
        VkDeviceMemory imageMemory{};
        Ref<VulkanImageView> imageView{};
        VkSampler sampler{};
    };

    class VulkanSimpleImageBuilder {
    public:
        VulkanSimpleImageBuilder() = default;
        virtual ~VulkanSimpleImageBuilder() = default;

        VulkanSimpleImageBuilder& SetImage(VkImage image)
        {
            this->image = image;
            return *this;
        }

        VulkanSimpleImageBuilder& SetFormat(const VkFormat format)
        {
            this->format = format;
            return *this;
        }

        Ref<VulkanImage> Build(VulkanDevice* device);

    private:
        VkImage image{};
        Ref<VulkanImageView> imageView{};
        VkFormat format;
    };

    class VulkanTextureImageBuilder : public VulkanImageBuilder {
    public:
        VulkanTextureImageBuilder() = default;
        ~VulkanTextureImageBuilder() override = default;

        VulkanTextureImageBuilder& SetData(void* data, const uint64_t size)
        {
            this->data = data;
            this->size = size;
            return *this;
        }

        VulkanTextureImageBuilder& SetResolution(const uint32_t width, const uint32_t height)
        {
            this->width = width;
            this->height = height;
            return *this;
        }

        VulkanTextureImageBuilder& SetFormat(const VkFormat format)
        {
            this->format = format;
            return *this;
        }

        VulkanTextureImageBuilder& SetFilter(const VkFilter minFilter, const VkFilter magFilter)
        {
            this->minFilter = minFilter;
            this->magFilter = magFilter;
            return *this;
        }

        VulkanTextureImageBuilder& SetTiling(const VkImageTiling tiling)
        {
            this->tiling = tiling;
            return *this;
        }

        Ref<VulkanTextureImage> Build(VulkanDevice* device);
    };

    class VulkanDepthImageBuilder : public VulkanImageBuilder {
    public:
        VulkanDepthImageBuilder() = default;
        ~VulkanDepthImageBuilder() override = default;

        VulkanDepthImageBuilder& SetResolution(const uint32_t width, const uint32_t height)
        {
            this->width = width;
            this->height = height;
            return *this;
        }

        Ref<VulkanDepthImage> Build(VulkanDevice* device);
    };
}
