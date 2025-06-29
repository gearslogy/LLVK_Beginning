//
// Created by liuya on 6/9/2025.
//

#include "MS_TriangleRenderer.h"
#include "renderer/public/Helper.hpp"
LLVK_NAMESPACE_BEGIN
void MS_TriangleRenderer::cleanupObjects(){
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    UT_Fn::cleanup_descriptor_pool(device, descPool);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_pipeline(device, pipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, descSetLayout);
}


void MS_TriangleRenderer::prepare(){
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    HLP::createSimpleDescPool(device, descPool);
    // set layout
    // set
    // update set
    // pipeline layout
    // pipeline

}

void MS_TriangleRenderer::render(){
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    const auto &cmdBuf = getMainCommandBuffer();


    // Record command buffer info
    VkCommandBufferBeginInfo beginInfo = FnCommand::commandBufferBeginInfo();
    auto [width, height] = getSwapChainExtent();
    // clear
    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    const auto renderPassBeginInfo= FnCommand::renderPassBeginInfo(getMainFramebuffer(),
      simplePass.pass,
      getSwapChainExtent(),
      clearValues);
    const auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    const auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );


    vkBeginCommandBuffer(cmdBuf, &beginInfo );
    {
        vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf,0, 1, &scissor);
        // ---------------------------  RENDER PASS ---------------------------
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        // RECORD COMMAND BUFFER
        vkCmdDrawMeshTasksEXT(cmdBuf, 1, 1, 1);
        vkCmdEndRenderPass(cmdBuf);
    }
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}


LLVK_NAMESPACE_END