//
// Created by lp on 2024/7/9.
//

#include "BasicRenderer.h"
#include "Image.h"
#include "PushConstant.h"
void BasicRenderer::cleanupObjects() {
    geometryBufferManager.cleanup();
    simpleDescriptorManager.cleanup();
    simplePipeline.cleanup();
}

void BasicRenderer::bindResources() {
    geometryBufferManager.bindDevice = mainDevice.logicalDevice;
    geometryBufferManager.bindPhysicalDevice = mainDevice.physicalDevice;
    geometryBufferManager.bindQueue = mainDevice.graphicsQueue;
    geometryBufferManager.bindCommandPool = graphicsCommandPool;

    simpleDescriptorManager.bindPhysicalDevice = mainDevice.physicalDevice;
    simpleDescriptorManager.bindDevice = mainDevice.logicalDevice;
    simpleDescriptorManager.bindQueue = mainDevice.graphicsQueue;
    simpleDescriptorManager.bindCommandPool = graphicsCommandPool;
    simpleDescriptorManager.bindSwapChainExtent = &simpleSwapchain.swapChainExtent;

    simplePipeline.bindDevice = mainDevice.logicalDevice;
    simplePipeline.bindPipelineCache = &simplePipelineCache;
    simplePipeline.bindRenderPass = simplePass.pass;
}


void BasicRenderer::loadModel() {
    //simpleObjLoader.readFile("content/pig.obj");
    //simpleObjLoader.readFile("content/viking_room.obj");
    simpleObjLoader.readFile("content/veqhch1/veqhch1_LOD0.obj");
    createVertexAndIndexBuffer(geometryBufferManager, simpleObjLoader);
}

void BasicRenderer::loadTexture() {
    //simpleDescriptorManager.createTexture("content/viking_room.png");
    simpleDescriptorManager.createTexture("content/veqhch1/veqhch1_4K_Albedo.jpg");
}
void BasicRenderer::setupDescriptors() {
    simpleDescriptorManager.createDescriptorSetLayout();
    simpleDescriptorManager.createUniformBuffers();
    simpleDescriptorManager.createDescriptorPool();
    simpleDescriptorManager.createDescriptorSets();
}
void BasicRenderer::preparePipelines() {
    simplePipeline.bindDescriptorSetLayouts[0] = simpleDescriptorManager.ubo_descriptorSetLayout;
    simplePipeline.bindDescriptorSetLayouts[1] = simpleDescriptorManager.texture_descriptorSetLayout;
    simplePipeline.init();
}

void BasicRenderer::updateUniformBuffer() {
    // update the PushConstants of the CPP
    PushConstant::update<VK_SHADER_STAGE_VERTEX_BIT>(currentFrame, [](PushVertexStageData &dataToChange) {
        dataToChange = {0,1,0,0};
    });
    PushConstant::update<VK_SHADER_STAGE_FRAGMENT_BIT>(currentFrame, [](PushFragmentStageData &dataToChange) {
        dataToChange ={1,0,0,1};
    });
    simpleDescriptorManager.simpleUniformBuffer.updateUniform(currentFrame);
}

void BasicRenderer::recordCommandBuffer() {
    // select framebuffer
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.6f, 0.65f, 0.4, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    const VkFramebuffer &framebuffer = activeSwapChainFramebuffer;
    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(framebuffer,
        simplePass.pass,
        &simpleSwapchain.swapChainExtent,clearValues);

    auto result = vkBeginCommandBuffer(activedFrameCommandBuferToSubmit, &cmdBufferBeginInfo);
    if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
    vkCmdBeginRenderPass(activedFrameCommandBuferToSubmit, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(activedFrameCommandBuferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS ,simplePipeline.graphicsPipeline);

    auto viewport = FnCommand::viewport(activedFrameCommandBuferToSubmit, simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    auto scissor = FnCommand::scissor(activedFrameCommandBuferToSubmit, simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    vkCmdSetViewport(activedFrameCommandBuferToSubmit, 0, 1, &viewport);
    vkCmdSetScissor(activedFrameCommandBuferToSubmit,0, 1, &scissor);

    // 更新当前帧的 Push Constants
    //std::cout << "push range1:" << 0 << " " << sizeof(PushVertexStageData) << std::endl;
    vkCmdPushConstants(activedFrameCommandBuferToSubmit,
        simplePipeline.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(PushVertexStageData),
        &PushConstant::vertexPushConstants[currentFrame]);
    //std::cout << "push range2:" << sizeof(PushFragmentStageData) << " " << sizeof(PushVertexStageData) << std::endl;
    vkCmdPushConstants(activedFrameCommandBuferToSubmit,
        simplePipeline.pipelineLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        sizeof(PushVertexStageData), // 偏移量为 Vertex Push Constant 的大小
        sizeof(PushFragmentStageData),
        &PushConstant::fragmentPushConstants[currentFrame]);

    renderIndicesGeometryCommand(simpleObjLoader, activedFrameCommandBuferToSubmit, [this]() {
        vkCmdBindDescriptorSets(activedFrameCommandBuferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS,
                simplePipeline.pipelineLayout, 0, 2,
                &(simpleDescriptorManager.descriptorSets)[currentFrame * 2],
                0, nullptr);
    });
    vkCmdEndRenderPass(activedFrameCommandBuferToSubmit);
    if (vkEndCommandBuffer(activedFrameCommandBuferToSubmit) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

