//
// Created by liuya on 11/11/2024.
//

#include "DualPass.h"
#include <LLVK_SYS.hpp>
#include "LLVK_Descriptor.hpp"
#include "LLVK_RenderPass.hpp"
#include "UT_DualVkRenderPass.hpp"
#include "DualPassGlobal.hpp"
#include "VulkanRenderer.h"
#include "DualPassRenderer.h"
#include "renderer/public/GeometryContainers.h"
LLVK_NAMESPACE_BEGIN
OpaqueScenePass::OpaqueScenePass(DualPassRenderer *renderer): pRenderer(renderer) {
    renderContainer = std::make_unique<RenderContainerOneSet>();
}
OpaqueScenePass::~OpaqueScenePass() =default;


void OpaqueScenePass::prepare() {
    // prepare the head and grid geo & tex

    renderContainer->requiredObjects.pRenderer = pRenderer;
    renderContainer->requiredObjects.pPool = &pRenderer->descPool;
    renderContainer->requiredObjects.pSetLayout = &pRenderer->hairDescSetLayout;
    renderContainer->requiredObjects.pUBOs[0] = &pRenderer->uboBuffers[0];
    renderContainer->requiredObjects.pUBOs[1] = &pRenderer->uboBuffers[1];

    RenderContainerOneSet::RenderDelegate grid;
    grid.bindGeometry(&pRenderer->gridLoader.parts[0]);
    grid.bindTextures(&pRenderer->gridTex);
    RenderContainerOneSet::RenderDelegate head;
    head.bindGeometry(&pRenderer->headLoader.parts[0]);
    head.bindTextures(&pRenderer->headTex);
    renderContainer->renderDelegates.emplace_back(grid);
    renderContainer->renderDelegates.emplace_back(head);
    renderContainer->buildSet();

    createRenderPass();
    createFramebuffer();
    createPipeline();
}

void OpaqueScenePass::recordCommandBuffer() {
    // UBO is shared, so we dont need ubo update here.
    auto [width, height] = pRenderer->getSwapChainExtent();
    VkDeviceSize offsets[1] = { 0 };
    auto viewport = FnCommand::viewport(width,height );
    auto scissor = FnCommand::scissor(width,height);
    // clear
    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    const auto renderPassBeginInfo= FnCommand::renderPassBeginInfo(opaqueFB, opaqueRenderPass, pRenderer->getSwapChainExtent(),clearValues);
    const auto cmdBuf = pRenderer->getMainCommandBuffer();
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);
    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , opaquePipeline);
    renderContainer->draw(cmdBuf, pRenderer->dualPipelineLayout);
    vkCmdEndRenderPass(cmdBuf);

}

void OpaqueScenePass::cleanup() {
    const auto device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_render_pass(device, opaqueRenderPass);
    UT_Fn::cleanup_pipeline(device, opaquePipeline);
    UT_Fn::cleanup_framebuffer(device, opaqueFB);
}


void OpaqueScenePass::onSwapChainResize() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_framebuffer(device, opaqueFB);
    createFramebuffer(); // rebuild framebuffer
}

void OpaqueScenePass::createFramebuffer() {
    const auto device = pRenderer->getMainDevice().logicalDevice;
    const auto [width,height ] = pRenderer->getSwapChainExtent();
    std::array attachments = {
        pRenderer->renderTargets.colorAttachment.view,
        pRenderer->renderTargets.depthAttachment.view
    };
    auto fbCIO = FnRenderPass::framebufferCreateInfo(width,height, opaqueRenderPass,attachments);
    if (vkCreateFramebuffer(device, &fbCIO, nullptr, &opaqueFB) != VK_SUCCESS)
        throw std::runtime_error("failed to create main pass framebuffer!");
}


void OpaqueScenePass::createRenderPass() {
    auto cdATM = FnRenderPass::colorAttachmentDescription(DualPassGlobal::colorAttachmentFormat);
    cdATM.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    auto depthATM = FnRenderPass::depthAttachmentDescription(DualPassGlobal::depthStencilAttachmentFormat);
    depthATM.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    const auto cdATMRef = FnRenderPass::attachmentReference(0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    const auto depthATMRef = FnRenderPass::attachmentReference(1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    const auto subpass = FnRenderPass::subpassDescription(cdATMRef,  depthATMRef);

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // before renderpass
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;    // before stage
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;   // after stage
    dependency.srcAccessMask = 0; // before access 首次写入，不需要等待之前的访问
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ; // after access

    std::array attachments = {cdATM, depthATM};
    auto renderPassCIO = FnRenderPass::renderPassCreateInfo(attachments);
    renderPassCIO.subpassCount = 1;
    renderPassCIO.pSubpasses = &subpass;
    renderPassCIO.dependencyCount = 1;
    renderPassCIO.pDependencies = &dependency;
    // create
    const auto device = pRenderer->getMainDevice().logicalDevice;
    const auto ret = vkCreateRenderPass(device, &renderPassCIO, nullptr, &opaqueRenderPass);
    if(ret != VK_SUCCESS) throw std::runtime_error{"ERROR"};
}
void OpaqueScenePass::createPipeline() {
    const auto device = pRenderer->getMainDevice().logicalDevice;
    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/dp_grid_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/dp_grid_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pRenderer->dualPipelineLayout);
    pso.setRenderPass(opaqueRenderPass);
    pso.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), opaquePipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);
}





LLVK_NAMESPACE_END
