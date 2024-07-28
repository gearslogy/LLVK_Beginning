//
// Created by star on 4/24/2024.
//

#include "RenderPass.h"
#include "Utils.h"
#include <array>
#include "LLVK_Image.h"
#include <iostream>
#include "magic_enum.hpp"
LLVK_NAMESPACE_BEGIN

void RenderPass::init() {
    std::array<VkAttachmentDescription,2> attachments{};
    //color attachment
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM; // same as swapchain format
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;            // before rendering
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;          // after rendering
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // 渲染之前stencil 干什么
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 渲染之后stencil 干什么
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    //depth attachment
    attachments[1].format = FnImage::findDepthFormat(bindPhysicalDevice);
    std::cout << "[[RENDER PASS]]:selected depth format:" <<magic_enum::enum_name(attachments[1].format) <<  " idx:"<<attachments[1].format << std::endl;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    constexpr int dependencyCount= 1;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    // before
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    // after
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    // create render pass
    VkRenderPassCreateInfo pass_CIO{};
    pass_CIO.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass_CIO.attachmentCount = attachments.size();
    pass_CIO.pAttachments = attachments.data();
    pass_CIO.subpassCount = 1;
    pass_CIO.pSubpasses = &subpass;
    pass_CIO.dependencyCount = dependencyCount;
    pass_CIO.pDependencies = &dependency;
    // create
    auto ret = vkCreateRenderPass(bindDevice, &pass_CIO, nullptr, &pass);
    if(ret != VK_SUCCESS) throw std::runtime_error{"ERROR"};

}

void RenderPass::cleanup() {
    vkDestroyRenderPass(bindDevice, pass, nullptr);
}

LLVK_NAMESPACE_END
