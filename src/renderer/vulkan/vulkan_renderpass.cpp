#include "vulkan_renderpass.h"
#include "vulkan_device.h"

#include <array>

#include "vulkan_framebuffer.h"
#include "vulkan_utils.h"

namespace Raytracing
{
    VulkanRenderPass::Builder::Builder(VulkanDevice* vulkanDevice): vulkanDevice(vulkanDevice) {}

    VulkanRenderPass::Builder& VulkanRenderPass::Builder::AddColorAttachment(const VkFormat imageFormat,
                                                                             const glm::vec4 clearColor,
                                                                             const VkSampleCountFlagBits sampleCount)
    {
        colorAttachments.push_back({imageFormat, sampleCount, clearColor});
        return *this;
    }

    VulkanRenderPass::Builder& VulkanRenderPass::Builder::AddDepthAttachment(VkFormat depthFormat)
    {
        depthAttachments.push_back({depthFormat});
        return *this;
    }

    Ref<VulkanRenderPass> VulkanRenderPass::Builder::Build()
    {
        uint32_t attachmentIndex = 0;
        std::vector<VkAttachmentReference> colorAttachmentsRefs;
        std::vector<VkAttachmentReference> depthAttachmentsRefs;
        std::vector<VkAttachmentDescription> attachmentDescriptions;

        for (auto colorAttachment: colorAttachments)
        {
            VkAttachmentDescription attachment{};
            attachment.format = colorAttachment.imageFormat;
            attachment.samples = colorAttachment.sampleCount;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment.finalLayout = attachmentIndex == 0 ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachmentDescriptions.push_back(attachment);

            VkAttachmentReference attachmentRef{};
            attachmentRef.attachment = attachmentIndex;
            attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachmentIndex++;

            colorAttachmentsRefs.push_back(attachmentRef);

            VkClearValue clearValue{};

            clearValue.color = {
                {
                    colorAttachment.clearColor.x,
                    colorAttachment.clearColor.y,
                    colorAttachment.clearColor.z,
                    colorAttachment.clearColor.w
                }
            };
            clearValues.push_back(clearValue);
        }

        for (auto depthAttachment: depthAttachments)
        {
            VkAttachmentDescription attachment{};
            attachment.format = depthAttachment.depthFormat;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_NONE;
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

            VkClearValue clearValue{};
            clearValue.depthStencil = {1.0f, 0};
            clearValues.push_back(clearValue);
        }

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

        VkRenderPass renderPass = VK_NULL_HANDLE;
        VK_CHECK_MSG(
            vkCreateRenderPass(vulkanDevice->GetDevice(), &render_pass_info, nullptr, &renderPass),
            "Failed to create render pass.");

        return CreateRef<VulkanRenderPass>(vulkanDevice, renderPass, clearValues);
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
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderPass::End(const VkCommandBuffer commandBuffer)
    {
        vkCmdEndRenderPass(commandBuffer);
    }
}
