//
// Created by liuya on 11/11/2024.
//

#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_VmaBuffer.hpp>

#include "DualPassGlobal.hpp"
#include "DualpassRenderer.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "UT_DualVkRenderPass.hpp"
#include "ScenePass.h"

LLVK_NAMESPACE_BEGIN
DualPassRenderer::DualPassRenderer() {
    mainCamera.setRotation({-5.31,11.92,1.3});
    mainCamera.mPosition = {0.22,1.6,0.68};
    mainCamera.mMoveSpeed = 0.01;
    opaqueScenePass = std::make_unique<OpaqueScenePass>(this);
    compPass = std::make_unique<CompPass>(this);
}
DualPassRenderer::~DualPassRenderer() = default;


void DualPassRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_resources(geometryManager, hairTex, gridTex, headTex);
    UT_Fn::cleanup_sampler(mainDevice.logicalDevice, colorSampler, depthSampler);
    vkDestroyDescriptorPool(device, descPool, VK_NULL_HANDLE);
    UT_Fn::cleanup_descriptor_set_layout(device, hairDescSetLayout);
    UT_Fn::cleanup_pipeline(device, hairPipeline1, hairPipeline2);
    UT_Fn::cleanup_pipeline_layout(device, dualPipelineLayout);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_render_pass(device, hairRenderPass1, hairRenderPass2);
    cleanupRenderTargets(); // this renderer will use COLOR + Depth target
    cleanupHairFramebuffers();
    opaqueScenePass->cleanup();
    compPass->cleanup();
}
void DualPassRenderer::cleanupRenderTargets() {
    UT_Fn::cleanup_resources(renderTargets.colorAttachment, renderTargets.depthAttachment);
}
void DualPassRenderer::cleanupHairFramebuffers() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_framebuffer(device, frameBuffersHairs.FBPass1, frameBuffersHairs.FBPass2);
}
void DualPassRenderer::swapChainResize() {
    cleanupRenderTargets();
    createRenderTargets(); // attachments rebuild
    cleanupHairFramebuffers();
    createHairFramebuffers(); // pass1 pass2 framebuffer rebuild
    // bind attachments to comp rendering ...  writeSets;
    opaqueScenePass->onSwapChainResize();
    compPass->onSwapChainResize();
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
    /*
    const std::array setLayoutBindings = {
        FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT),
        FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
    };*/
    using desc_types = MetaDesc::desc_types_t<MetaDesc::UBO,MetaDesc::CIS>;
    using binding_positions_t = MetaDesc::desc_binding_position_t<0,1>; // 0 1
    using binding_usages_t =  MetaDesc::desc_binding_usage_t<VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT>;
    constexpr std::array setLayoutBindings= MetaDesc::generateSetLayoutBindings<desc_types, binding_positions_t, binding_usages_t>();

    const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(setLayoutBindings);
    UT_Fn::invoke_and_check("Error create desc set layout",vkCreateDescriptorSetLayout,device, &setLayoutCIO, nullptr, &hairDescSetLayout);

    // 4. model resource loading
    colorSampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    depthSampler = FnImage::createDepthSampler(mainDevice.logicalDevice);
    setRequiredObjectsByRenderer(this, geometryManager);
    headLoader.load("content/scene/dualpass/head.gltf");
    hairLoader.load("content/scene/dualpass/hair.gltf");
    gridLoader.load("content/scene/dualpass/grid.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(headLoader, geometryManager); // GPU buffer allocation
    UT_VmaBuffer::addGeometryToSimpleBufferManager(hairLoader, geometryManager); // GPU buffer allocation
    UT_VmaBuffer::addGeometryToSimpleBufferManager(gridLoader, geometryManager); // GPU buffer allocation
    setRequiredObjectsByRenderer(this, hairTex, gridTex, headTex);
    hairTex.create("content/scene/dualpass/gpu_hair_D.ktx2", colorSampler);
    gridTex.create("content/scene/dualpass/gpu_grid_D.ktx2", colorSampler);
    headTex.create("content/scene/dualpass/gpu_head_D.ktx2", colorSampler);

    // 5 create desc sets
    const std::array<VkDescriptorSetLayout,2> setLayouts({hairDescSetLayout,hairDescSetLayout});
    auto setAllocInfo = FnDescriptor::setAllocateInfo(descPool,setLayouts);
    UT_Fn::invoke_and_check("Error create RenderContainerOneSet::uboSets", vkAllocateDescriptorSets, device, &setAllocInfo, hairDescSets.data());

    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        std::array<VkWriteDescriptorSet,2> writeSets {
            // hair
            FnDescriptor::writeDescriptorSet(hairDescSets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[currentFrame].descBufferInfo),
            FnDescriptor::writeDescriptorSet(hairDescSets[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &hairTex.descImageInfo),
        };
        vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
    }

    // attachments render target
    createRenderTargets();
    // prepare 1: hair rendering resources
    hairRenderPass1 = UT_DualRenderPass::pass1(device);
    hairRenderPass2 = UT_DualRenderPass::pass2(device);
    // create pipelines
    UT_DualRenderPass::createPipelines(device, hairRenderPass1, hairRenderPass2, hairDescSetLayout, getPipelineCache(),
        dualPipelineLayout, hairPipeline1, hairPipeline2);
    createHairFramebuffers();

    // prepare 2: opaque scene pass
    opaqueScenePass->prepare();
    compPass->prepare();

}



void DualPassRenderer::createRenderTargets() {
    setRequiredObjectsByRenderer(this, renderTargets.colorAttachment, renderTargets.depthAttachment);
    auto [width, height] = simpleSwapchain.swapChainExtent;
    VkImageUsageFlagBits colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    renderTargets.colorAttachment.create(width, height, VK_FORMAT_R8G8B8A8_UNORM, colorSampler, colorUsage);
    renderTargets.depthAttachment.create(width,height, VK_FORMAT_D32_SFLOAT, depthSampler, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
void DualPassRenderer::createHairFramebuffers() {
    const auto &device = mainDevice.logicalDevice;
    auto [width, height] = simpleSwapchain.swapChainExtent;
    frameBuffersHairs.FBPass1 =  UT_DualRenderPass::createFramebuffer(device, hairRenderPass1,  renderTargets.colorAttachment.view, renderTargets.depthAttachment.view, width, height);
    frameBuffersHairs.FBPass2 = UT_DualRenderPass::createFramebuffer(device, hairRenderPass2,  renderTargets.colorAttachment.view, renderTargets.depthAttachment.view, width, height);
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
    recordAll();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}

void DualPassRenderer::recordAll() {
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("begin dual pass command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    opaqueScenePass->recordCommandBuffer();
    recordHairPass1();
    recordHairPass2();
    compPass->recordCommandBuffer();
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}


void DualPassRenderer::recordHairPass1() {
     // Clear values for all attachments written in the fragment shader
    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    auto [width, height]= simpleSwapchain.swapChainExtent;
    VkRenderPassBeginInfo renderPassBeginInfo {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = hairRenderPass1;
    renderPassBeginInfo.framebuffer = frameBuffersHairs.FBPass1;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues =clearValues.data();

    vkCmdBeginRenderPass(activatedFrameCommandBufferToSubmit, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS , hairPipeline1);
    cmdRenderHair();
    vkCmdEndRenderPass(activatedFrameCommandBufferToSubmit);
}
void DualPassRenderer::recordPass1DepthOnly() {
    auto [width, height]= simpleSwapchain.swapChainExtent;
    VkClearValue cleaValue;
    cleaValue.depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo renderPassBeginInfo {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = hairRenderPass1;
    renderPassBeginInfo.framebuffer = frameBuffersHairs.FBPass1;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &cleaValue;
    vkCmdBeginRenderPass(activatedFrameCommandBufferToSubmit, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS , hairPipeline1);
    cmdRenderHair();
    vkCmdEndRenderPass(activatedFrameCommandBufferToSubmit);
}



void DualPassRenderer::recordHairPass2() {
    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    auto [width, height]= simpleSwapchain.swapChainExtent;
    VkRenderPassBeginInfo renderPassBeginInfo {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = hairRenderPass2;
    renderPassBeginInfo.framebuffer = frameBuffersHairs.FBPass2;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(activatedFrameCommandBufferToSubmit, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS , hairPipeline2);
    cmdRenderHair();
    vkCmdEndRenderPass(activatedFrameCommandBufferToSubmit);

}

void DualPassRenderer::cmdRenderHair() {
    VkDeviceSize offsets[1] = { 0 };
    const auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    const auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    vkCmdSetViewport(activatedFrameCommandBufferToSubmit, 0, 1, &viewport);
    vkCmdSetScissor(activatedFrameCommandBufferToSubmit,0, 1, &scissor);
    vkCmdBindDescriptorSets(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, dualPipelineLayout,
0, 1, &hairDescSets[currentFrame], 0, nullptr);
    vkCmdBindVertexBuffers(activatedFrameCommandBufferToSubmit, 0, 1, &hairLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(activatedFrameCommandBufferToSubmit,hairLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(activatedFrameCommandBufferToSubmit, hairLoader.parts[0].indices.size(), 1, 0, 0, 0);

}

LLVK_NAMESPACE_END