//
// Created by lp on 2024/10/11.
//

#include "ScenePass.h"
#include "LLVK_Descriptor.hpp"
#include "VulkanRenderer.h"
#include "UT_ShadowMap.hpp"
LLVK_NAMESPACE_BEGIN
void SceneGeometryContainer::buildSet() {
    const auto &device = requiredObjects.pVulkanRenderer->getMainDevice().logicalDevice;
    const auto &pool = *requiredObjects.pPool;
    const auto &ubo_setLayout = *requiredObjects.pSetLayoutUBO;
    const auto &tex_setLayout = *requiredObjects.pSetLayoutTexture;
    const auto &ubo = *requiredObjects.pUBO;

    for(auto &geo: renderableObjects) {
        auto uboSetAllocInfo = FnDescriptor::setAllocateInfo(pool,&ubo_setLayout, 1);
        UT_Fn::invoke_and_check("Error create offscreen sets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,&geo.setUBO);
        auto texSetAllocInfo = FnDescriptor::setAllocateInfo(pool,&tex_setLayout, 1);
        UT_Fn::invoke_and_check("Error create offscreen sets",vkAllocateDescriptorSets,device, &texSetAllocInfo,&geo.setTexture);
    }


    // update set
    for(const auto &geo: renderableObjects) {
        const auto &ubo_set = geo.setUBO;
        const auto &tex_set = geo.setUBO;
        // UBO
        std::vector<VkWriteDescriptorSet> writeSets;
        writeSets.emplace_back(FnDescriptor::writeDescriptorSet(ubo_set,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &ubo.descBufferInfo));
        // tex
        for(auto &&[idx, tex] : UT_Fn::enumerate(geo.pTextures))
            writeSets.emplace_back(FnDescriptor::writeDescriptorSet(tex_set,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, &tex->descImageInfo));
        vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
    }
}

ScenePass::ScenePass(const VulkanRenderer *renderer, const VkDescriptorPool *descPool,
                     const VkCommandBuffer *cmd): pRenderer(renderer), pDescriptorPool(descPool), pCommandBuffer(cmd) {

}


void ScenePass::prepare() {
    prepareUniformBuffers();
    prepareDescriptorSets();

}

void ScenePass::prepareUniformBuffers() {
    setRequiredObjects(pRenderer, uniformBuffers.scene);
    uniformBuffers.scene.createAndMapping(sizeof(uniformDataScene));
    updateUniformBuffers();
}

void ScenePass::updateUniformBuffers() {
    auto [width, height] = simpleSwapchain.swapChainExtent;
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uniformDataScene.projection = mainCamera.projection();
    uniformDataScene.projection[1][1] *= -1;
    uniformDataScene.view = mainCamera.view();
    uniformDataScene.model = glm::mat4(1.0f);
    uniformDataScene.depthBiasMVP = shadowMapPass->depthMVP;
    memcpy(uniformBuffers.scene.mapped, &uniformDataScene, sizeof(uniformDataScene));
}


void ScenePass::prepareDescriptorSets() {
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    const auto &physicalDevice = mainDevice.physicalDevice;
    // we only have one set.
    auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);           // ubo
    auto set0_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // base
    auto set0_ubo_binding2 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT); // ordp
    auto set0_ubo_binding3 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, VK_SHADER_STAGE_FRAGMENT_BIT); // depth
    const std::array scene_setLayout_bindings = {set0_ubo_binding0, set0_ubo_binding1, set0_ubo_binding2, set0_ubo_binding3};
    const VkDescriptorSetLayoutCreateInfo sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(scene_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create offscreen descriptor set layout",vkCreateDescriptorSetLayout,device, &sceneSetLayoutCIO, nullptr, &sceneDescriptorSetLayout);
    auto sceneAllocInfo = FnDescriptor::setAllocateInfo(descPool,&sceneDescriptorSetLayout,1); // only one set
    UT_Fn::invoke_and_check("Error create scene sets",vkAllocateDescriptorSets,device, &sceneAllocInfo,&sceneSets.opacity );
    UT_Fn::invoke_and_check("Error create scene sets",vkAllocateDescriptorSets,device, &sceneAllocInfo,&sceneSets.opaque );

    // - scene opacity
    std::vector<VkWriteDescriptorSet> opacitySceneWriteSets;
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.scene.descBufferInfo));
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, &foliageAlbedoTex.descImageInfo));
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2, &foliageOrdpTex.descImageInfo));
    opacitySceneWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opacity,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3, &shadowMapPass->shadowFramebuffer.depthAttachment.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(opacitySceneWriteSets.size()),opacitySceneWriteSets.data(),0, nullptr);

    // - scene opaque
    std::vector<VkWriteDescriptorSet> sceneOpaqueWriteSets;
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uniformBuffers.scene.descBufferInfo));
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, &gridAlbedoTex.descImageInfo));
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2, &gridOrdpTex.descImageInfo));
    sceneOpaqueWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(sceneSets.opaque,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3, &shadowMapPass->shadowFramebuffer.depthAttachment.descImageInfo));
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(sceneOpaqueWriteSets.size()),sceneOpaqueWriteSets.data(),0, nullptr);
}

void ScenePass::preparePipelines() {
    const auto &mainDevice = pRenderer->getMainDevice();
    auto device = mainDevice.logicalDevice;
    //shader modules
    const auto sceneVertMoudule = FnPipeline::createShaderModuleFromSpvFile("shaders/sm_v2_scene_vert.spv",  device);
    const auto sceneFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/sm_v2_scene_frag.spv",  device); // need VK_CULLING_NONE
    //shader stages
    VkPipelineShaderStageCreateInfo sceneVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, sceneVertMoudule);
    VkPipelineShaderStageCreateInfo sceneFragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, sceneFragModule);
    // layout
    const std::array sceneSetLayouts{sceneDescriptorSetLayout};
    VkPipelineLayoutCreateInfo sceneSetLayout_CIO = FnPipeline::layoutCreateInfo(sceneSetLayouts);
    UT_Fn::invoke_and_check("ERROR create scene pipeline layout",vkCreatePipelineLayout,device, &sceneSetLayout_CIO,nullptr, &scenePipelineLayout );
    // back data to pso
    pipelinePSOs.setShaderStages(sceneVertShaderStageCreateInfo, sceneFragShaderStageCreateInfo);
    pipelinePSOs.setPipelineLayout(scenePipelineLayout);
    pipelinePSOs.setRenderPass(simplePass.pass);
    // create pipeline
    UT_GraphicsPipelinePSOs::createPipeline(device, scenePSO, simplePipelineCache.pipelineCache, scenePipeline.opaque);
    pipelinePSOs.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
    UT_GraphicsPipelinePSOs::createPipeline(device, scenePSO, simplePipelineCache.pipelineCache, scenePipeline.opacity);

    UT_Fn::cleanup_shader_module(device, sceneFragModule, sceneVertMoudule);
}

void ScenePass::recordCommandBuffer() {
}

void ScenePass::cleanup() {
}

LLVK_NAMESPACE_END
