//
// Created by star on 4/24/2024.
//

#include "RenderPass.h"
#include "Utils.h"
#include <array>
void RenderPass::init() {
    std::array<VkAttachmentDescription,1> attachments{};
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM; // same as swapchain format
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;            // before rendering
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;          // after rendering
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // 渲染之前stencil 干什么
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 渲染之后stencil 干什么
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachRefs[1] {
        {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}
    };
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = attachRefs;

    /*
    constexpr  int dependencyCount= 2;
    VkSubpassDependency subpassDependency[dependencyCount]= {};
    // --- 1. VK_IMAGE_LAYOUT_UNDEFINED -----> VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // MUST after
    subpassDependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    // must before
    subpassDependency[0].dstSubpass = 0;
    subpassDependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency[0].dependencyFlags = 0;
    // --- 2. VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL -----> VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // must after
    subpassDependency[1].srcSubpass = 0;
    subpassDependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // must before
    subpassDependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependency[1].dependencyFlags = 0;*/

    constexpr  int dependencyCount= 1;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstSubpass = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    // create render pass
    VkRenderPassCreateInfo pass_CIO{};
    pass_CIO.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass_CIO.attachmentCount = 1;
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


