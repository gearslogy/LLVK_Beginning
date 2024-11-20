//
// Created by liuya on 11/11/2024.
//

#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_VmaBuffer.hpp>
#include "DualpassRenderer.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "UT_DualVkRenderPass.hpp"
LLVK_NAMESPACE_BEGIN
DualPassRenderer::DualPassRenderer() {
    mainCamera.setRotation({-13,16,2.6});
    mainCamera.mPosition = {0.475032,2.13,2.40};
    mainCamera.mMoveSpeed = 0.01;
}

void DualPassRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_resources(geometryManager, tex);
    UT_Fn::cleanup_sampler(mainDevice.logicalDevice, colorSampler, depthSampler);
    vkDestroyDescriptorPool(device, descPool, VK_NULL_HANDLE);
    UT_Fn::cleanup_descriptor_set_layout(device, descSetLayout);
    UT_Fn::cleanup_pipeline(device, hairPipeline1, hairPipeline2);
    UT_Fn::cleanup_pipeline_layout(device, dualPipelineLayout);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_render_pass(device, renderpass1, renderpass2);
    UT_Fn::cleanup_resources(renderTargets.colorAttachment, renderTargets.depthAttachment);
    UT_Fn::cleanup_framebuffer(device, frameBuffers.FBPass1, frameBuffers.FBPass2);
}


void DualPassRenderer::prepare() {
    // 1. pool
    const auto &device = mainDevice.logicalDevice;
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    assert(result == VK_SUCCESS);
    // 2. UBO create
    setRequiredObjectsByRenderer(this, uboBuffers[0], uboBuffers[1]);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uboData));

    // 3 descSetLayout
    const std::array setLayoutBindings = {
        FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT),
        FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
    };
    const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(setLayoutBindings);
    UT_Fn::invoke_and_check("Error create desc set layout",vkCreateDescriptorSetLayout,device, &setLayoutCIO, nullptr, &descSetLayout);

    // 4. model resource loading
    colorSampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    depthSampler = FnImage::createDepthSampler(mainDevice.logicalDevice);
    setRequiredObjectsByRenderer(this, geometryManager);
    headLoader.load("content/scene/dualpass/head.gltf");
    hairLoader.load("content/scene/dualpass/hair.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(headLoader, geometryManager); // GPU buffer allocation
    UT_VmaBuffer::addGeometryToSimpleBufferManager(hairLoader, geometryManager); // GPU buffer allocation
    setRequiredObjectsByRenderer(this, tex);
    tex.create("content/scene/dualpass/gpu_D.ktx2", colorSampler);

    // 5 create desc sets
    const std::array<VkDescriptorSetLayout,2> setLayouts({descSetLayout,descSetLayout});
    auto setAllocInfo = FnDescriptor::setAllocateInfo(descPool,setLayouts);
    UT_Fn::invoke_and_check("Error create RenderContainerOneSet::uboSets", vkAllocateDescriptorSets, device, &setAllocInfo, dualDescSets.data());

    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        std::array<VkWriteDescriptorSet,2> writeSets {
            FnDescriptor::writeDescriptorSet(dualDescSets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[currentFrame].descBufferInfo),
            FnDescriptor::writeDescriptorSet(dualDescSets[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &tex.descImageInfo)
        };
        vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
    }

    // attachments render target
    createRenderTargets();
    // create renderpass
    renderpass1 = UT_DualRenderPass::pass1(device);
    renderpass2 = UT_DualRenderPass::pass2(device);
    // create pipelines
    UT_DualRenderPass::createPipelines(device, renderpass1, renderpass2, descSetLayout, getPipelineCache(),
        dualPipelineLayout, hairPipeline1, hairPipeline2);
    createFramebuffers();

}
void DualPassRenderer::createRenderTargets() {
    setRequiredObjectsByRenderer(this, renderTargets.colorAttachment, renderTargets.depthAttachment);
    auto [width, height] = simpleSwapchain.swapChainExtent;
    VkImageUsageFlagBits colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    renderTargets.colorAttachment.create(width, height, VK_FORMAT_R8G8B8A8_UNORM, colorSampler, colorUsage);
    renderTargets.depthAttachment.create(width,height, VK_FORMAT_D32_SFLOAT, depthSampler, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
void DualPassRenderer::createFramebuffers() {
    const auto &device = mainDevice.logicalDevice;
    auto [width, height] = simpleSwapchain.swapChainExtent;
    frameBuffers.FBPass1 = UT_DualRenderPass::createFramebuffer(device, renderpass1,  renderTargets.colorAttachment.view, renderTargets.depthAttachment.view, width, height);
    frameBuffers.FBPass2 = UT_DualRenderPass::createFramebuffer(device, renderpass1,  renderTargets.colorAttachment.view, renderTargets.depthAttachment.view, width, height);
}

void DualPassRenderer::updateDualUBOs() {
    auto [width, height] =  getSwapChainExtent();
    auto &&mainCamera = getMainCamera();
    const auto frame = getCurrentFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = mainCamera.view();
    uboData.model = glm::mat4(1.0f);
    memcpy(uboBuffers[frame].mapped, &uboData, sizeof(uboData));
}


void DualPassRenderer::render() {
    updateDualUBOs();
    twoPassRender();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}

void DualPassRenderer::twoPassRender() {

    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("begin shadow command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    recordPass1();
    //recordPass2();
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}



void DualPassRenderer::recordPass1() {
     // Clear values for all attachments written in the fragment shader
    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    auto [width, height]= simpleSwapchain.swapChainExtent;
    VkRenderPassBeginInfo renderPassBeginInfo {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderpass1;
    renderPassBeginInfo.framebuffer = frameBuffers.FBPass1;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues =clearValues.data();

    vkCmdBeginRenderPass(getMainCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(getMainCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS , hairPipeline2);
    cmdRenderHair();
    vkCmdEndRenderPass(getMainCommandBuffer());

}

void DualPassRenderer::recordPass2() {
    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    auto [width, height]= simpleSwapchain.swapChainExtent;
    VkRenderPassBeginInfo renderPassBeginInfo {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderpass2;
    renderPassBeginInfo.framebuffer = frameBuffers.FBPass2;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues =clearValues.data();

    vkCmdBeginRenderPass(getMainCommandBuffer(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(getMainCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS , hairPipeline2);
    cmdRenderHair();
    vkCmdEndRenderPass(getMainCommandBuffer());

}

void DualPassRenderer::cmdRenderHair() {
    VkDeviceSize offsets[1] = { 0 };
    const auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    const auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    vkCmdSetViewport(getMainCommandBuffer(), 0, 1, &viewport);
    vkCmdSetScissor(getMainCommandBuffer(),0, 1, &scissor);
    vkCmdBindDescriptorSets(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, dualPipelineLayout,
0, 1, &dualDescSets[currentFrame], 0, nullptr);
    vkCmdBindVertexBuffers(getMainCommandBuffer(), 0, 1, &hairLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(getMainCommandBuffer(),hairLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(getMainCommandBuffer(), hairLoader.parts[0].indices.size(), 1, 0, 0, 0);

}


LLVK_NAMESPACE_END