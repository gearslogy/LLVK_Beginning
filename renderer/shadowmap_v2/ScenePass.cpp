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


    auto uboSetAllocInfo = FnDescriptor::setAllocateInfo(pool,&ubo_setLayout, 1);
    auto texSetAllocInfo = FnDescriptor::setAllocateInfo(pool,&tex_setLayout, 1);
    auto allocateSetForObjects = [&](auto &objs) {
        for(auto &geo : objs) {
            UT_Fn::invoke_and_check("Error create --A scene ubo sets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,&geo.setUBO);
            UT_Fn::invoke_and_check("Error create --A scene tex sets",vkAllocateDescriptorSets,device, &texSetAllocInfo,&geo.setTexture);
        }
    };
    allocateSetForObjects(opacityRenderableObjects);
    allocateSetForObjects(opaqueRenderableObjects);

    for(auto &geo : opacityRenderableObjects) {
        UT_Fn::invoke_and_check("Error create opacity scene ubo sets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,&geo.setUBO);
        UT_Fn::invoke_and_check("Error create opacity scene tex sets",vkAllocateDescriptorSets,device, &texSetAllocInfo,&geo.setTexture);
    }
    for(auto &geo : opaqueRenderableObjects) {
        UT_Fn::invoke_and_check("Error create opaque scene ubo sets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,&geo.setUBO);
        UT_Fn::invoke_and_check("Error create opaque scene tex sets",vkAllocateDescriptorSets,device, &texSetAllocInfo,&geo.setTexture);
    }


    auto updateWriteSets = [&,this](auto &&objs) {
        for(const auto &geo: objs) {
            const auto &ubo_set = geo.setUBO;
            const auto &tex_set = geo.setTexture;
            // UBO
            std::vector<VkWriteDescriptorSet> writeSets;
            writeSets.emplace_back(FnDescriptor::writeDescriptorSet(ubo_set,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &ubo.descBufferInfo));
            // tex
            for(auto &&[idx, tex] : UT_Fn::enumerate(geo.pTextures))
                writeSets.emplace_back(FnDescriptor::writeDescriptorSet(tex_set,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,idx, &tex->descImageInfo));
            vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
        }
    };
    updateWriteSets(opacityRenderableObjects);
    updateWriteSets(opaqueRenderableObjects);

}

ScenePass::ScenePass(const VulkanRenderer *renderer, const VkDescriptorPool *descPool): pRenderer(renderer), pDescriptorPool(descPool){}


void ScenePass::prepare() {
    prepareUniformBuffers();
    prepareDescriptorSets();
    preparePipelines();
}
void ScenePass::cleanup() {
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_pipeline(device, opacityPipeline, opaquePipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, textureDescSetLayout, uboDescSetLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    uboBuffer.cleanup(); // ubo buffer clean
}


void ScenePass::prepareUniformBuffers() {
    setRequiredObjects(pRenderer, uboBuffer);
    uboBuffer.createAndMapping(sizeof(uniformDataScene));
    //updateUniformBuffers();
}

void ScenePass::updateUniformBuffers(const glm::mat4 &depthMVP, const glm::vec4 &lightPos) {
    auto [width, height] =   pRenderer->getSwapChainExtent();
    auto &&mainCamera = const_cast<VulkanRenderer *>(pRenderer)->getMainCamera();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uniformDataScene.projection = mainCamera.projection();
    uniformDataScene.projection[1][1] *= -1;
    uniformDataScene.view = mainCamera.view();
    uniformDataScene.model = glm::mat4(1.0f);
    uniformDataScene.depthBiasMVP = depthMVP;
    uniformDataScene.lightPos = lightPos;
    memcpy(uboBuffer.mapped, &uniformDataScene, sizeof(uniformDataScene));
}


void ScenePass::prepareDescriptorSets() {
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    const auto &physicalDevice = mainDevice.physicalDevice;
    // we only have one set.
    auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);           // ubo
    auto set1_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, VK_SHADER_STAGE_FRAGMENT_BIT); // base
    auto set1_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // ordp
    auto set1_ubo_binding2 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT); // depth
    const std::array ubo_setLayout_bindings = {set0_ubo_binding0};
    const std::array tex_setLayout_bindings = {set1_ubo_binding0, set1_ubo_binding1, set1_ubo_binding2};

    const VkDescriptorSetLayoutCreateInfo uboSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(ubo_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create uboDescSetLayout set layout",vkCreateDescriptorSetLayout,device, &uboSetLayoutCIO, nullptr, &uboDescSetLayout);

    const VkDescriptorSetLayoutCreateInfo texSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(tex_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create textureDescSetLayout set layout",vkCreateDescriptorSetLayout,device, &texSetLayoutCIO, nullptr, &textureDescSetLayout);


    SceneGeometryContainer::RequiredObjects SCRequiredObjects{};
    SCRequiredObjects.pVulkanRenderer = pRenderer;
    SCRequiredObjects.pPool = pDescriptorPool;
    SCRequiredObjects.pUBO = &uboBuffer;
    SCRequiredObjects.pSetLayoutUBO = &uboDescSetLayout;
    SCRequiredObjects.pSetLayoutTexture = &textureDescSetLayout;

    geoContainer.setRequiredObjects(std::move(SCRequiredObjects));
    geoContainer.buildSet();
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
    const std::array sceneSetLayouts{uboDescSetLayout, textureDescSetLayout};
    VkPipelineLayoutCreateInfo sceneSetLayout_CIO = FnPipeline::layoutCreateInfo(sceneSetLayouts);
    UT_Fn::invoke_and_check("ERROR create scene pipeline layout",vkCreatePipelineLayout,device, &sceneSetLayout_CIO,nullptr, &pipelineLayout );
    // back data to pso
    pipelinePSOs.setShaderStages(sceneVertShaderStageCreateInfo, sceneFragShaderStageCreateInfo);
    pipelinePSOs.setPipelineLayout(pipelineLayout);
    pipelinePSOs.setRenderPass(pRenderer->getMainRenderPass());
    // create pipeline
    UT_GraphicsPipelinePSOs::createPipeline(device, pipelinePSOs, pRenderer->getPipelineCache(), opaquePipeline);
    pipelinePSOs.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
    UT_GraphicsPipelinePSOs::createPipeline(device, pipelinePSOs, pRenderer->getPipelineCache(), opacityPipeline);
    UT_Fn::cleanup_shader_module(device, sceneFragModule, sceneVertMoudule);
}

void ScenePass::recordCommandBuffer() {
    // ----------  render scene, using shadow map
    std::vector<VkClearValue> sceneClearValues(2);
    sceneClearValues[0].color = {0.4, 0.4, 0.4, 1};
    sceneClearValues[1].depthStencil = { 1.0f, 0 };
    const auto sceneRenderPass = pRenderer->getMainRenderPass();
    const auto sceneRenderExtent = pRenderer->getSwapChainExtent();
    const auto sceneFramebuffer = pRenderer->getMainFramebuffer();
    const auto sceneCommandBuffer = pRenderer->getMainCommandBuffer();
    const auto sceneRenderPassBeginInfo = FnCommand::renderPassBeginInfo(sceneFramebuffer, sceneRenderPass, sceneRenderExtent,sceneClearValues);

    vkCmdBeginRenderPass(sceneCommandBuffer, &sceneRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    auto [vs_width , vs_height] = sceneRenderExtent;
    auto viewport = FnCommand::viewport(vs_width,vs_height );
    auto scissor = FnCommand::scissor(vs_width, vs_height);
    vkCmdSetViewport(sceneCommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(sceneCommandBuffer,0, 1, &scissor);
    VkDeviceSize offsets[1] = { 0 };


    // render funciton:
    auto renderObjects = [&](auto &&objs) {
        for(const auto &geo : objs) {
            const GLTFLoader::Part *gltfPartGeo = geo.pGeometry;
            vkCmdBindVertexBuffers(sceneCommandBuffer, 0, 1, &gltfPartGeo->verticesBuffer, offsets);
            vkCmdBindIndexBuffer(sceneCommandBuffer,gltfPartGeo->indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
            std::array bindSets = {geo.setUBO, geo.setTexture};
            vkCmdBindDescriptorSets(sceneCommandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, bindSets.size(), bindSets.data(), 0, nullptr);
            vkCmdDrawIndexed(sceneCommandBuffer, gltfPartGeo->indices.size(), 1, 0, 0, 0);
        }
    };

    // render opacity object
    auto &&opacityObjs = geoContainer.getRenderableObjects(SceneGeometryContainer::opacity);
    vkCmdBindPipeline(sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , opacityPipeline); // FIRST generate depth for opaque object
    renderObjects(opacityObjs);

    // render opaque object
    auto &&opaqueObjs = geoContainer.getRenderableObjects(SceneGeometryContainer::opaque);
    vkCmdBindPipeline(sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , opaquePipeline); // FIRST generate depth for opaque object
    renderObjects(opaqueObjs);

    vkCmdEndRenderPass(sceneCommandBuffer);
}



LLVK_NAMESPACE_END
