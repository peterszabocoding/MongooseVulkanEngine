#include "vulkan_renderpass.h"
#include "vulkan_device.h"

#include <array>

#include "vulkan_framebuffer.h"
#include "vulkan_utils.h"

namespace Raytracing
{
    VulkanRenderPass::Builder::Builder(VulkanDevice* vulkanDevice): vulkanDevice(vulkanDevice){}

    VulkanRenderPass::Builder& VulkanRenderPass::Builder::AddColorAttachment(const VkFormat imageFormat, const VkSampleCountFlagBits sampleCount)
    {
        colorAttachments.push_back({ imageFormat, sampleCount });
        return *this;
    }

    VulkanRenderPass::Builder& VulkanRenderPass::Builder::AddDepthAttachment(VkFormat depthFormat)
    {
        depthAttachments.push_back({ depthFormat });
        return *this;
    }

    Ref<VulkanRenderPass> VulkanRenderPass::Builder::Build()
    {
        uint32_t attachmentIndex = 0;
        std::vector<VkAttachmentReference> colorAttachmentsRefs;
        std::vector<VkAttachmentReference> depthAttachmentsRefs;
        std::vector<VkAttachmentDescription> attachmentDescriptions;

        for (auto colorAttachment : colorAttachments)
        {
            VkAttachmentDescription attachment{};
            attachment.format = colorAttachment.imageFormat;
            attachment.samples = colorAttachment.sampleCount;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            attachmentDescriptions.push_back(attachment);

            VkAttachmentReference attachmentRef{};
            attachmentRef.attachment = attachmentIndex;
            attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachmentIndex++;

            colorAttachmentsRefs.push_back(attachmentRef);
        }

        for (auto depthAttachment : depthAttachments)
        {
            VkAttachmentDescription attachment{};
            attachment.format = depthAttachment.depthFormat;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            attachmentDescriptions.push_back(attachment);

            VkAttachmentReference attachmentRef{};
            attachmentRef.attachment = attachmentIndex;
            attachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            attachmentIndex++;

            depthAttachmentsRefs.push_back(attachmentRef);
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = colorAttachments.size();
        subpass.pColorAttachments = colorAttachmentsRefs.data();
        subpass.pDepthStencilAttachment = depthAttachmentsRefs.data();

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
        render_pass_info.pAttachments = attachmentDescriptions.data();
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;

        VkRenderPass renderPass = VK_NULL_HANDLE;
        VK_CHECK_MSG(
            vkCreateRenderPass(vulkanDevice->GetDevice(), &render_pass_info, nullptr, &renderPass),
            "Failed to create render pass.");

        return CreateRef<VulkanRenderPass>(vulkanDevice, renderPass);
    }

    VulkanRenderPass::~VulkanRenderPass()
    {
        vkDestroyRenderPass(device->GetDevice(), renderPass, nullptr);
    }

    void VulkanRenderPass::Begin(const VkCommandBuffer commandBuffer, const Ref<VulkanFramebuffer>& framebuffer, const VkExtent2D extent)
    {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer->Get();
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderPass::End(const VkCommandBuffer commandBuffer)
    {
        vkCmdEndRenderPass(commandBuffer);
    }
}
