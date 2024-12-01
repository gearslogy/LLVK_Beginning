//
// Created by liuya on 11/10/2024.
//

#include "CSMScenePass.h"
#include "CSMRenderer.h"
#include "renderer/public/GeometryContainers.h"
LLVK_NAMESPACE_BEGIN
CSMScenePass::CSMScenePass(CSMRenderer *rd) : pRenderer(rd){}

void CSMScenePass::prepare() {
    const auto &device = pRenderer->mainDevice.logicalDevice;

    uint32_t enableInstance{0};
    VkSpecializationMapEntry specMapEntry = {0,0, sizeof(uint32_t)};
    VkSpecializationInfo specInfo = {};
    specInfo.mapEntryCount = 1;
    specInfo.pMapEntries = &specMapEntry;
    specInfo.dataSize = sizeof(uint32_t);
    specInfo.pData = &enableInstance;

    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/csm_scene_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/csm_scene_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    vsMD_ssCIO.pSpecializationInfo = &specInfo; // inject specialize var
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);

    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pRenderer->pipelineLayout);
    pso.setRenderPass(pRenderer->getMainRenderPass());
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), normalPipeline);
    enableInstance = 1;
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), instancePipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);

}

void CSMScenePass::cleanup() {
    const auto &device = pRenderer->mainDevice.logicalDevice;
    UT_Fn::cleanup_pipeline(device, instancePipeline, normalPipeline);
}

void CSMScenePass::recordCommandBuffer() {
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.6f, 0.65f, 0.4, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    auto renderPassBeginInfo = FnCommand::renderPassBeginInfo(pRenderer->getMainFramebuffer(), pRenderer->getMainRenderPass(), pRenderer->getSwapChainExtent(), clearValues);
    const auto &cmdBuf = pRenderer->getMainCommandBuffer();
    vkCmdBeginRenderPass(cmdBuf,&renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
    const auto [width , height]= pRenderer->getSwapChainExtent();
    auto viewport = FnCommand::viewport(width, height );
    auto scissor = FnCommand::scissor(width, height );
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);

    pRenderer->renderGeometry(normalPipeline, instancePipeline);
    vkCmdEndRenderPass(cmdBuf);
}



LLVK_NAMESPACE_END