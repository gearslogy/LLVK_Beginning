//
// Created by lp on 2024/9/19.
//

#include "JsonSceneParser.h"
#include <fstream>
#include "LLVK_UT_Json.hpp"
#include "LLVK_Descriptor.hpp"
#include "VulkanRenderer.h"
#include "renderer/shadowmap_v2/UT_ShadowMap.hpp"
#include "LLVK_GeomtryLoader.h"
LLVK_NAMESPACE_BEGIN
void InstanceGeometryContainer::buildSet() {
    const auto &device = requiredObjects.pVulkanRenderer->getMainDevice().logicalDevice;
    const auto &pool = *requiredObjects.pPool;
    const auto &ubo_setLayout = *requiredObjects.pSetLayoutUBO;
    const auto &tex_setLayout = *requiredObjects.pSetLayoutTexture;
    const auto &ubo = *requiredObjects.pUBO;


    auto uboSetAllocInfo = FnDescriptor::setAllocateInfo(pool,&ubo_setLayout, 1);
    auto texSetAllocInfo = FnDescriptor::setAllocateInfo(pool,&tex_setLayout, 1);
    auto allocateSetForObjects = [&](auto &objs) {
        for(auto &geo : objs) {
            UT_Fn::invoke_and_check("Error create terrain ubo sets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,&geo.setUBO);
            UT_Fn::invoke_and_check("Error create terrain tex sets",vkAllocateDescriptorSets,device, &texSetAllocInfo,&geo.setTexture);
        }
    };
    allocateSetForObjects(opacityRenderableObjects);
    allocateSetForObjects(opaqueRenderableObjects);

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

JsonPointsParser::JsonPointsParser(const std::string &path){
    std::ifstream in(path.data());
    if (!in.good())
        throw std::runtime_error{std::string{"Could not open file "} + path + "."};
    in >> jsHandle;
    if(not jsHandle.contains("points") )
        throw std::runtime_error{std::string{"Could not parse JSON file, no points key "} };
    const nlohmann::json &points = jsHandle[points];

    for (auto &data: points) {
        const auto &P = data["P"].get<glm::vec3>();
        const auto &orient = data["orient"].get<glm::vec4>();
        const auto &pscale = data["scale"].get<float>();
        instanceData.emplace_back(InstanceData{P, orient, pscale});
    }
    std::cout << "[[JsonSceneParser]]" << " npts:" << instanceData.size() << std::endl;
}



InstancedObjectPass::InstancedObjectPass(const VulkanRenderer *renderer, const VkDescriptorPool *descPool):pRenderer(renderer),pDescriptorPool(descPool) { }

void InstancedObjectPass::cleanup() {
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    instanceBufferManager.cleanup();
    UT_Fn::cleanup_pipeline(device, opacityPipeline, opaquePipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, textureDescSetLayout, uboDescSetLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    uboBuffer.cleanup(); // ubo buffer clean
}

void InstancedObjectPass::prepare() {
    prepareUniformBuffers();
    prepareDescriptorSets();
    preparePipelines();
}

void InstancedObjectPass::prepareUniformBuffers() {
    setRequiredObjectsByRenderer(pRenderer, uboBuffer);
    uboBuffer.createAndMapping(sizeof(uniformDataScene));
}

void InstancedObjectPass::updateUniformBuffers(const glm::mat4 &depthMVP, const glm::vec4 &lightPos) {
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

void InstancedObjectPass::prepareDescriptorSets() {
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    // we only have one set.
    auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);           // ubo
    auto set1_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, VK_SHADER_STAGE_FRAGMENT_BIT); // albedo
    auto set1_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // n
    auto set1_ubo_binding2 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT); // rough/metal/ao/unkown
    //auto set1_ubo_binding3 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, VK_SHADER_STAGE_FRAGMENT_BIT); // may be depth.
    const std::array ubo_setLayout_bindings = {set0_ubo_binding0};
    const std::array tex_setLayout_bindings = {set1_ubo_binding0, set1_ubo_binding1, set1_ubo_binding2};

    const VkDescriptorSetLayoutCreateInfo uboSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(ubo_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create uboDescSetLayout set layout",vkCreateDescriptorSetLayout,device, &uboSetLayoutCIO, nullptr, &uboDescSetLayout);
    const VkDescriptorSetLayoutCreateInfo texSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(tex_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create textureDescSetLayout set layout",vkCreateDescriptorSetLayout,device, &texSetLayoutCIO, nullptr, &textureDescSetLayout);

    InstanceGeometryContainer::RequiredObjects SCRequiredObjects{};
    SCRequiredObjects.pVulkanRenderer = pRenderer;
    SCRequiredObjects.pPool = pDescriptorPool;
    SCRequiredObjects.pUBO = &uboBuffer;
    SCRequiredObjects.pSetLayoutUBO = &uboDescSetLayout;
    SCRequiredObjects.pSetLayoutTexture = &textureDescSetLayout;
    geoContainer.setRequiredObjects(std::move(SCRequiredObjects));
    geoContainer.buildSet();

}

void InstancedObjectPass::preparePipelines() {
    const auto &mainDevice = pRenderer->getMainDevice();
    auto device = mainDevice.logicalDevice;
    //shader modules
    const auto sceneVertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/terrain_vert.spv",  device);
    const auto sceneFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/terrain_frag.spv",  device);
    //shader stages
    VkPipelineShaderStageCreateInfo sceneVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, sceneVertModule);
    VkPipelineShaderStageCreateInfo sceneFragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, sceneFragModule);
    // point attributes desc
    constexpr int vertexBufferBindingID = 0;
    constexpr int instanceBufferBindingID  = 1;
    std::array<VkVertexInputAttributeDescription,10> attribsDesc{};
    attribsDesc[0] = { 0,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, P)};
    attribsDesc[1] = { 1,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, Cd)};
    attribsDesc[2] = { 2,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, N)};
    attribsDesc[3] = { 3,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, T)};
    attribsDesc[4] = { 4,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertex, B)};
    attribsDesc[5] = { 5,vertexBufferBindingID,VK_FORMAT_R32G32_SFLOAT , offsetof(GLTFVertex, uv0)};
    attribsDesc[6] = { 6,vertexBufferBindingID,VK_FORMAT_R32G32_SFLOAT , offsetof(GLTFVertex, uv1)};
    // Per-Instance attributes
    attribsDesc[7] = { 7,instanceBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(InstanceData, P)};
    attribsDesc[8] = { 8,instanceBufferBindingID,VK_FORMAT_R32G32B32A32_SFLOAT , offsetof(InstanceData, orient)};
    attribsDesc[9] = { 9,instanceBufferBindingID,VK_FORMAT_R32_SFLOAT , offsetof(InstanceData, pscale)};

    // binding desc
    VkVertexInputBindingDescription vertexBinding{vertexBufferBindingID, sizeof(GLTFVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputBindingDescription instanceBinding{instanceBufferBindingID, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE};
    std::array<VkVertexInputBindingDescription,2> bindingsDesc{vertexBinding, instanceBinding};
    pipelinePSOs.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);

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
    UT_Fn::cleanup_shader_module(device,sceneVertModule, sceneFragModule);
}

void InstancedObjectPass::recordCommandBuffer() {
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
            // Binding point 0 : Mesh vertex buffer
            vkCmdBindVertexBuffers(sceneCommandBuffer, 0, 1, &gltfPartGeo->verticesBuffer, offsets);
            // Binding point 1 : Instance data buffer
            vkCmdBindVertexBuffers(sceneCommandBuffer, 1, 1, &geo->instanceBuffer, offsets);

            vkCmdBindIndexBuffer(sceneCommandBuffer,gltfPartGeo->indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
            std::array bindSets = {geo.setUBO, geo.setTexture};
            vkCmdBindDescriptorSets(sceneCommandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, bindSets.size(), bindSets.data(), 0, nullptr);
            vkCmdDrawIndexed(sceneCommandBuffer, gltfPartGeo->indices.size(), 1, 0, 0, 0);
        }
    };

    // render opacity objs
    auto &&opacityObjs = geoContainer.getRenderableObjects(InstanceGeometryContainer::opacity);
    vkCmdBindPipeline(sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , opacityPipeline); // FIRST generate depth for opaque object
    renderObjects(opacityObjs);

    // render opaque object
    auto &&opaqueObjs = geoContainer.getRenderableObjects(InstanceGeometryContainer::opaque);
    vkCmdBindPipeline(sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , opaquePipeline); // FIRST generate depth for opaque object
    renderObjects(opaqueObjs);

    vkCmdEndRenderPass(sceneCommandBuffer);

}

VkBuffer InstancedObjectPass::loadInstanceData(std::string_view path) {
    setRequiredObjectsByRenderer(pRenderer,instanceBufferManager);
    JsonPointsParser js{path.data()};
    VkDeviceSize bufferSize = sizeof(InstanceData) * std::size(js.instanceData);
    instanceBufferManager.createBufferWithStagingBuffer<VK_BUFFER_USAGE_VERTEX_BUFFER_BIT>(bufferSize, js.instanceData.data());
    return instanceBufferManager.createVertexBuffers.back().buffer;
}




LLVK_NAMESPACE_END