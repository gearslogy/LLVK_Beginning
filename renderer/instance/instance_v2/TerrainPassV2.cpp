//
// Created by lp on 2024/10/16.
//

#include "TerrainPassV2.h"
#include "LLVK_Descriptor.hpp"
#include "VulkanRenderer.h"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
void TerrainGeometryContainerV2::buildSet() {
    const auto &device = requiredObjects.pVulkanRenderer->getMainDevice().logicalDevice;
    const auto &pool = *requiredObjects.pPool;
    const auto &ubo_setLayout = *requiredObjects.pSetLayoutUBO;
    const auto &tex_setLayout = *requiredObjects.pSetLayoutTexture;



    const std::vector ubo_layouts(MAX_FRAMES_IN_FLIGHT, ubo_setLayout);
    const std::vector tex_layouts(MAX_FRAMES_IN_FLIGHT, tex_setLayout);

    auto uboSetAllocInfo = FnDescriptor::setAllocateInfo(pool,ubo_layouts);
    auto texSetAllocInfo = FnDescriptor::setAllocateInfo(pool,tex_layouts);
    auto allocateSetForObjects = [&](std::vector<RenderableObject> &objs) {
        for(auto &geo : objs) {
            UT_Fn::invoke_and_check("Error create terrain ubo sets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,geo.setUBOs.data());
            UT_Fn::invoke_and_check("Error create terrain tex sets",vkAllocateDescriptorSets,device, &texSetAllocInfo,geo.setTextures.data());
        }
    };
    allocateSetForObjects(opaqueRenderableObjects);

    auto updateWriteSets = [&,this](auto &&objs) {
        for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
            const auto &ubo = *requiredObjects.pUBOs[i];
            for(const auto &geo: objs) {
                const auto &ubo_set = geo.setUBOs[i];
                const auto &tex_set = geo.setTextures[i];
                // UBO
                std::vector<VkWriteDescriptorSet> writeSets;
                writeSets.emplace_back(FnDescriptor::writeDescriptorSet(ubo_set,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &ubo.descBufferInfo));
                // tex
                for(auto &&[idx, tex] : UT_Fn::enumerate(geo.pTextures))
                    writeSets.emplace_back(FnDescriptor::writeDescriptorSet(tex_set,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,idx, &tex->descImageInfo));
                vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
            }
        }
    };
    updateWriteSets(opaqueRenderableObjects);

}
TerrainPassV2::TerrainPassV2(const VulkanRenderer *renderer, const VkDescriptorPool *descPool):pRenderer(renderer),pDescriptorPool(descPool) { }


void TerrainPassV2::prepare() {
    prepareUniformBuffers();
    prepareDescriptorSets();
    preparePipelines();
}
void TerrainPassV2::cleanup() {
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_pipeline(device, opaquePipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, textureDescSetLayout, uboDescSetLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_range_resources(uboBuffers);
}


void TerrainPassV2::prepareUniformBuffers() {
    setRequiredObjectsByRenderer(pRenderer, uboBuffers[0], uboBuffers[1]);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uniformDataScene));
}

void TerrainPassV2::updateUniformBuffers(const glm::mat4 &depthMVP, const glm::vec4 &lightPos) {
    auto [width, height] =   pRenderer->getSwapChainExtent();
    auto &&mainCamera = const_cast<VulkanRenderer *>(pRenderer)->getMainCamera();
    const auto frame = pRenderer->getCurrentFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uniformDataScene.projection = mainCamera.projection();
    uniformDataScene.projection[1][1] *= -1;
    uniformDataScene.view = mainCamera.view();
    uniformDataScene.model = glm::mat4(1.0f);
    uniformDataScene.depthBiasMVP = depthMVP;
    uniformDataScene.lightPos = lightPos;
    memcpy(uboBuffers[frame].mapped, &uniformDataScene, sizeof(uniformDataScene));
}


void TerrainPassV2::prepareDescriptorSets() {
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    const auto &physicalDevice = mainDevice.physicalDevice;

    auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);           // ubo
    auto set1_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, VK_SHADER_STAGE_FRAGMENT_BIT); // albedo
    auto set1_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // ordp
    auto set1_ubo_binding2 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT); // n
    auto set1_ubo_binding3 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, VK_SHADER_STAGE_FRAGMENT_BIT); // terrain mask
    const std::array ubo_setLayout_bindings = {set0_ubo_binding0};
    const std::array tex_setLayout_bindings = {set1_ubo_binding0, set1_ubo_binding1, set1_ubo_binding2, set1_ubo_binding3};

    const VkDescriptorSetLayoutCreateInfo uboSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(ubo_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create uboDescSetLayout set layout",vkCreateDescriptorSetLayout,device, &uboSetLayoutCIO, nullptr, &uboDescSetLayout);

    const VkDescriptorSetLayoutCreateInfo texSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(tex_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create textureDescSetLayout set layout",vkCreateDescriptorSetLayout,device, &texSetLayoutCIO, nullptr, &textureDescSetLayout);


    TerrainGeometryContainerV2::RequiredObjects SCRequiredObjects{};
    SCRequiredObjects.pVulkanRenderer = pRenderer;
    SCRequiredObjects.pPool = pDescriptorPool;
    SCRequiredObjects.pUBOs = {&uboBuffers[0], &uboBuffers[1]};
    SCRequiredObjects.pSetLayoutUBO = &uboDescSetLayout;
    SCRequiredObjects.pSetLayoutTexture = &textureDescSetLayout;

    geoContainer.setRequiredObjects(std::move(SCRequiredObjects));
    geoContainer.buildSet();
}

void TerrainPassV2::preparePipelines() {
    const auto &mainDevice = pRenderer->getMainDevice();
    auto device = mainDevice.logicalDevice;
    //shader modules
    const auto sceneVertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/terrain_vert_v2.spv",  device);
    const auto sceneFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/terrain_frag_v2.spv",  device);
    //shader stages
    VkPipelineShaderStageCreateInfo sceneVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, sceneVertModule);
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
    UT_Fn::cleanup_shader_module(device,sceneVertModule, sceneFragModule);
}

void TerrainPassV2::recordCommandBuffer() {
    const auto sceneCommandBuffer = pRenderer->getMainCommandBuffer();
    /*
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
    */
    VkDeviceSize offsets[1] = { 0 };


    // render funciton:
    auto renderObjects = [&](auto &&objs) {
        for(const auto &geo : objs) {
            const GLTFLoader::Part *gltfPartGeo = geo.pGeometry;
            vkCmdBindVertexBuffers(sceneCommandBuffer, 0, 1, &gltfPartGeo->verticesBuffer, offsets);
            vkCmdBindIndexBuffer(sceneCommandBuffer,gltfPartGeo->indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
            std::array bindSets = {geo.setUBOs[pRenderer->getCurrentFrame()], geo.setTextures[pRenderer->getCurrentFrame()]};
            vkCmdBindDescriptorSets(sceneCommandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, bindSets.size(), bindSets.data(), 0, nullptr);
            vkCmdDrawIndexed(sceneCommandBuffer, gltfPartGeo->indices.size(), 1, 0, 0, 0);
        }
    };

    // render opaque object
    auto &&opaqueObjs = geoContainer.getRenderableObjects();
    vkCmdBindPipeline(sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , opaquePipeline); // FIRST generate depth for opaque object
    renderObjects(opaqueObjs);

    //vkCmdEndRenderPass(sceneCommandBuffer);
}


LLVK_NAMESPACE_END