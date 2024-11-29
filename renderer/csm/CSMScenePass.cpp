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
    // pipeline layout
    const std::array layouts{pRenderer->sceneDescLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(layouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device,
        &pipelineLayoutCIO,nullptr, &pipelineLayout );


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
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(pRenderer->getMainRenderPass());
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), normalPipeline);
    enableInstance = 1;
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), instancePipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);

}

void CSMScenePass::cleanup() {
    const auto &device = pRenderer->mainDevice.logicalDevice;
    UT_Fn::cleanup_pipeline(device, instancePipeline, normalPipeline);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
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

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , normalPipeline);
    // render ground
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
      0, 1, &pRenderer->setGround[pRenderer->getCurrentFrame()] ,
      0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &pRenderer->resourceManager.geos.ground.parts[0]);

    // render 35
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
      0, 1, &pRenderer->set35[pRenderer->getCurrentFrame()] ,
      0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &pRenderer->resourceManager.geos.geo_35.parts[0]);
    // render 36
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
      0, 1, &pRenderer->set36[pRenderer->getCurrentFrame()] ,
      0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &pRenderer->resourceManager.geos.geo_36.parts[0]);
    // render 39
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
      0, 1, &pRenderer->set39[pRenderer->getCurrentFrame()] ,
      0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &pRenderer->resourceManager.geos.geo_39.parts[0]);

    // render instance geometry
    constexpr auto instanceCount = 4;
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , instancePipeline);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
       0, 1, &pRenderer->set29[pRenderer->getCurrentFrame()] ,
       0, nullptr);
    UT_GeometryContainer::renderPart(cmdBuf, &pRenderer->resourceManager.geos.geo_29.parts[0], instanceCount);
    vkCmdEndRenderPass(cmdBuf);
}



LLVK_NAMESPACE_END