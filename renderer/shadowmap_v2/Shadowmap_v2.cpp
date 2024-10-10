//
// Created by liuya on 9/1/2024.
//
#include "ShadowMapPass.h"
#include "Shadowmap_v2.h"
#include <libs/magic_enum.hpp>
#include "LLVK_UT_VmaBuffer.hpp"
#include "LLVK_Descriptor.hpp"
#include "Pipeline.hpp"

LLVK_NAMESPACE_BEGIN
Shadowmap_v2::Shadowmap_v2() {
    mainCamera.mPosition = glm::vec3(15,20,192);
    shadowMapPass = std::make_unique<ShadowMapPass>(this,
        &descPool,
        &activatedFrameCommandBufferToSubmit);
    lightPos = {281.654, 120,316.942};
}
Shadowmap_v2::~Shadowmap_v2() = default;



void Shadowmap_v2::render() {
    updateUniformBuffers();
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}

void Shadowmap_v2::cleanupObjects() {
    auto device = mainDevice.logicalDevice;
    shadowMapPass->cleanup();
    vkDestroyDescriptorPool(device, descPool, nullptr);
    UT_Fn::cleanup_resources(geoBufferManager, foliageAlbedoTex, foliageOrdpTex);
    UT_Fn::cleanup_resources(gridAlbedoTex, gridOrdpTex);
    UT_Fn::cleanup_sampler(device, colorSampler);

    // cleanup scene
    UT_Fn::cleanup_pipeline(device, scenePipeline.opacity, scenePipeline.opaque);
    UT_Fn::cleanup_descriptor_set_layout(device, sceneDescriptorSetLayout);
    UT_Fn::cleanup_pipeline_layout(device, scenePipelineLayout);

    // cleaun up uniform buffers
    uniformBuffers.scene.cleanup();
}

void Shadowmap_v2::prepare() {
    colorSampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    loadTextures();
    loadModels();
    createDescriptorPool();
    // shadow pass prepare
    shadowMapPass->lightPos = lightPos;
    auto &&geoContainer = shadowMapPass->getGeometryContainer();
    ShadowMapGeometryContainer::RenderableObject renderObjs[2];
    renderObjs[0].pGeometry = &gridGeo.parts[0];
    renderObjs[0].pTexture  = &gridAlbedoTex;
    renderObjs[1].pGeometry = &foliageGeo.parts[0];
    renderObjs[1].pTexture  = &foliageAlbedoTex;
    geoContainer.addRenderableGeometry(renderObjs[0]);
    geoContainer.addRenderableGeometry(renderObjs[1]);
    shadowMapPass->prepare();

    // scene pass prepare
    prepareUniformBuffers();
    prepareDescriptorSets();
    preparePipelines();

}

void Shadowmap_v2::loadTextures() {
    setRequiredObjects(foliageAlbedoTex, foliageOrdpTex,gridAlbedoTex,gridOrdpTex);
    foliageAlbedoTex.create("content/shadowmap_v2/plant/basecolor.ktx2", colorSampler );
    foliageOrdpTex.create("content/shadowmap_v2/plant/ordp.ktx2",colorSampler);
    gridAlbedoTex.create("content/shadowmap_v2/ground/basecolor.ktx2", colorSampler );
    gridOrdpTex.create("content/shadowmap_v2/ground/ordp.ktx2", colorSampler );
}

void Shadowmap_v2::loadModels() {
    setRequiredObjects(geoBufferManager);
    gridGeo.load("content/shadowmap/grid.gltf");
    foliageGeo.load("content/plants/gardenplants/var0.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(gridGeo, geoBufferManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(foliageGeo, geoBufferManager);
}



void Shadowmap_v2::prepareUniformBuffers() {
    setRequiredObjects(uniformBuffers.scene);
    uniformBuffers.scene.createAndMapping(sizeof(uniformDataScene));
    updateUniformBuffers();
}
void Shadowmap_v2::updateUniformBuffers() {
    // offscreen
    shadowMapPass->updateUniformBuffers();

    auto [width, height] = simpleSwapchain.swapChainExtent;
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uniformDataScene.projection = mainCamera.projection();
    uniformDataScene.projection[1][1] *= -1;
    uniformDataScene.view = mainCamera.view();
    uniformDataScene.model = glm::mat4(1.0f);
    uniformDataScene.depthBiasMVP = shadowMapPass->depthMVP;
    memcpy(uniformBuffers.scene.mapped, &uniformDataScene, sizeof(uniformDataScene));
}
void Shadowmap_v2::createDescriptorPool() {
    const auto &device = mainDevice.logicalDevice;
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 4); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    assert(result == VK_SUCCESS);
}


void Shadowmap_v2::prepareDescriptorSets() {
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



void Shadowmap_v2::preparePipelines() {
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
    scenePSO.setShaderStages(sceneVertShaderStageCreateInfo, sceneFragShaderStageCreateInfo);
    scenePSO.setPipelineLayout(scenePipelineLayout);
    scenePSO.setRenderPass(simplePass.pass);
    // create pipeline
    UT_GraphicsPipelinePSOs::createPipeline(device, scenePSO, simplePipelineCache.pipelineCache, scenePipeline.opaque);
    scenePSO.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
    UT_GraphicsPipelinePSOs::createPipeline(device, scenePSO, simplePipelineCache.pipelineCache, scenePipeline.opacity);

    UT_Fn::cleanup_shader_module(device, sceneFragModule, sceneVertMoudule);
}

void Shadowmap_v2::recordCommandBuffer() {
    //vkResetCommandBuffer(activatedFrameCommandBufferToSubmit,/*VkCommandBufferResetFlagBits*/ 0); //0: command buffer reset
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("begin shadow command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    {
        shadowMapPass->recordCommandBuffer();
    }
    {
        // ----------  render scene, using shadow map
        std::vector<VkClearValue> sceneClearValues(2);
        sceneClearValues[0].color = {0.4, 0.4, 0.4, 1};
        sceneClearValues[1].depthStencil = { 1.0f, 0 };
        const auto sceneRenderPass = simplePass.pass;
        const auto sceneRenderExtent = simpleSwapchain.swapChainExtent;
        const auto sceneRenderPassBeginInfo = FnCommand::renderPassBeginInfo(activatedSwapChainFramebuffer, sceneRenderPass, sceneRenderExtent,sceneClearValues);
        vkCmdBeginRenderPass(cmdBuf, &sceneRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , scenePipeline.opaque); // FIRST generate depth for opaque object
        auto [vs_width , vs_height] = sceneRenderExtent;
        auto viewport = FnCommand::viewport(vs_width,vs_height );
        auto scissor = FnCommand::scissor(vs_width, vs_height);
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf,0, 1, &scissor);
        VkDeviceSize offsets[1] = { 0 };
        //vkCmdPushConstants(cmdBuf,scenePipelineLayout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(float),&disable_opacity_texture);
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &gridGeo.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,gridGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipelineLayout, 0, 1, &sceneSets.opaque, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, gridGeo.parts[0].indices.size(), 1, 0, 0, 0);

        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , scenePipeline.opacity); // FIRST generate depth for opaque object
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf,0, 1, &scissor);
        //vkCmdPushConstants(cmdBuf,scenePipelineLayout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(float),&enable_opacity_texture);
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &foliageGeo.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,foliageGeo.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipelineLayout, 0, 1, &sceneSets.opacity, 0, nullptr);
        vkCmdDrawIndexed(cmdBuf, foliageGeo.parts[0].indices.size(), 1, 0, 0, 0);

        vkCmdEndRenderPass(cmdBuf);
    }
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}




LLVK_NAMESPACE_END
