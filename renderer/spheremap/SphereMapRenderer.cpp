//
// Created by liuya on 1/1/2025.
//

#include "SphereMapRenderer.h"

LLVK_NAMESPACE_BEGIN
namespace SPHEREMAP_NAMESPACE {

void SphereMapRenderer::prepare() {
    const auto &device = getMainDevice().logicalDevice;
    // desc pool
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    if (result != VK_SUCCESS) throw std::runtime_error{"ERROR"};
    sphereMapPass.pRenderer = this;
    sphereMapPass.prepare();
    scenePass.pRenderer = this;
    scenePass.pCubeTex = &sphereMapPass.mSphereTex;
    scenePass.prepare();
}

void SphereMapRenderer::render() {
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}

void SphereMapRenderer::cleanupObjects() {
    const auto &device = getMainDevice().logicalDevice;
    UT_Fn::cleanup_descriptor_pool(device, descPool);
    sphereMapPass.cleanup();
    scenePass.cleanup();
}

void SphereMapRenderer::recordCommandBuffer() {
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.0f, 0.0f, 0.0, 0.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    auto cmdBuf  = getMainCommandBuffer();
    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(getMainFramebuffer(),
        simplePass.pass,
        &simpleSwapchain.swapChainExtent,clearValues);
    const auto [width , height]= getSwapChainExtent();
    UT_Fn::invoke_and_check("begin vat storage pass command", vkBeginCommandBuffer, cmdBuf, &cmdBufferBeginInfo);
    {
        vkCmdBeginRenderPass(cmdBuf, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        sphereMapPass.recordCommandBuffer(cmdBuf);
        scenePass.recordCommandBuffer(cmdBuf);
        vkCmdEndRenderPass(cmdBuf);
    }
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );

}
}
LLVK_NAMESPACE_END

