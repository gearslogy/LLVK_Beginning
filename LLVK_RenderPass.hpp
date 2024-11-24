//
// Created by liuya on 11/23/2024.
//

#pragma once

#include "LLVK_SYS.hpp"
#include "Utils.h"
LLVK_NAMESPACE_BEGIN
namespace FnRenderPass {

     inline VkAttachmentDescription colorAttachmentDescription(VkFormat format) {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        return colorAttachment;
    }
    //
    inline VkAttachmentDescription depthAttachmentDescription(VkFormat format) {
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = format;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        return depthAttachment;
    }

    inline VkRenderPassCreateInfo renderPassCreateInfo(const VkSubpassDescription &subpass,
        const Concept::is_range auto &attachments,
        const Concept::is_range auto &dependencies) {
        //const VkAttachmentDescription*    pAttachments;
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = attachments.size();
        renderPassCreateInfo.pAttachments = attachments.data();
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;
        renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassCreateInfo.pDependencies = dependencies.data();
        return renderPassCreateInfo;
    }

    constexpr VkAttachmentReference attachmentReference(uint32_t attachmentPosition , VkImageLayout imageLayout) {
        return {attachmentPosition, imageLayout};
    }
    constexpr auto subPassDescription( const Concept::is_range auto &colorAttachmentRefs) {
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = colorAttachmentRefs.size();
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = nullptr;
        return subpass;
    }
    constexpr auto subPassDescription() {
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        return subpass;
    }
    constexpr auto subpassDescription( const Concept::is_range auto &colorAttachmentRefs,
        const VkAttachmentReference &depthRef) {
        auto subpass = subPassDescription();
        subpass.colorAttachmentCount = colorAttachmentRefs.size();
        subpass.pColorAttachments = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = &depthRef;
        return subpass;
    }
    constexpr auto subpassDescription( const VkAttachmentReference &colorRef,
      const VkAttachmentReference &depthRef) {
        auto subpass = subPassDescription();
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;
        return subpass;
    }
    constexpr auto renderPassCreateInfo() {
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        return renderPassInfo;
    }
    // attachments include depthAttachmentDescription
    constexpr auto renderPassCreateInfo(const Concept::is_range auto &attachments) {
        VkRenderPassCreateInfo renderPassInfo = renderPassCreateInfo();
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        return renderPassInfo;
    }

    inline auto framebufferCreateInfo(const uint32_t width, const uint32_t height, const VkRenderPass &pass,
        const Concept::is_range auto &attachmentsImageView) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentsImageView.size());
        framebufferInfo.pAttachments = attachmentsImageView.data();
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;
        return framebufferInfo;
    }


}


LLVK_NAMESPACE_END


