//
// Created by liuya on 11/10/2024.
//

#include "CSMScenePass.h"
#include "CSMRenderer.h"
LLVK_NAMESPACE_BEGIN
CSMScenePass::CSMScenePass(CSMRenderer *rd) : pRenderer(rd){}

void CSMScenePass::prepare() {
    const auto &device = pRenderer->mainDevice.logicalDevice;
    // pipeline
    const std::array deferredLayouts{pRenderer->sceneDescLayout};
    VkPipelineLayoutCreateInfo deferredLayout_CIO = FnPipeline::layoutCreateInfo(deferredLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &deferredLayout_CIO,nullptr, &pipelineLayout );

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

}



LLVK_NAMESPACE_END