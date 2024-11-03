//
// Created by lp on 2024/9/19.
//

#include "IndirectDrawPass.h"
#include <fstream>
#include <renderer/public/UT_CustomRenderer.hpp>

#include "LLVK_UT_Json.hpp"
#include "LLVK_Descriptor.hpp"
#include "VulkanRenderer.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "LLVK_GeomtryLoader.h"
LLVK_NAMESPACE_BEGIN

IndirectDrawPass::JsonPointsParser::JsonPointsParser(const std::string &path){
    std::ifstream in(path.data());
    if (!in.good())
        throw std::runtime_error{std::string{"Could not open file "} + path + "."};
    in >> jsHandle;
    if(not jsHandle.contains("points") )
        throw std::runtime_error{std::string{"Could not parse JSON file, no points key "} };
    const nlohmann::json &points = jsHandle["points"];

    for (auto &data: points) {
        const auto &P = data["P"].get<glm::vec3>();
        const auto &orient = data["orient"].get<glm::vec4>();
        const auto &pscale = data["scale"].get<float>();
        instanceData.emplace_back(InstanceData{P, orient, pscale});
    }
    std::cout << "[[JsonSceneParser]]" << " npts:" << instanceData.size() << std::endl;
}



IndirectDrawPass::IndirectDrawPass(const VulkanRenderer *renderer, const VkDescriptorPool *descPool):pRenderer(renderer),pDescriptorPool(descPool) { }

void IndirectDrawPass::cleanup() {
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_pipeline(device,pipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, textureDescSetLayout, uboDescSetLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_range_resources(uboBuffers);

}

void IndirectDrawPass::prepare() {
    prepareUniformBuffers();
    prepareDescriptorSets();
    preparePipelines();
}

void IndirectDrawPass::prepareUniformBuffers() {
    setRequiredObjectsByRenderer(pRenderer, uboBuffers[0], uboBuffers[1]);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uniformDataScene));
}

void IndirectDrawPass::updateUniformBuffers( const glm::mat4 &depthMVP, const glm::vec4 &lightPos) {
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

void IndirectDrawPass::prepareDescriptorSets() {
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    // we only have one set.
    auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);           // ubo
    auto set1_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, VK_SHADER_STAGE_FRAGMENT_BIT); // albedo array
    auto set1_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // n array
    auto set1_ubo_binding2 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT); // rough/metal/ao/unkown array
    //auto set1_ubo_binding3 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, VK_SHADER_STAGE_FRAGMENT_BIT); // may be depth.
    const std::array ubo_setLayout_bindings = {set0_ubo_binding0};
    const std::array tex_setLayout_bindings = {set1_ubo_binding0, set1_ubo_binding1, set1_ubo_binding2};

    const VkDescriptorSetLayoutCreateInfo uboSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(ubo_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create uboDescSetLayout set layout",vkCreateDescriptorSetLayout,device, &uboSetLayoutCIO, nullptr, &uboDescSetLayout);
    const VkDescriptorSetLayoutCreateInfo texSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(tex_setLayout_bindings);
    UT_Fn::invoke_and_check("Error create textureDescSetLayout set layout",vkCreateDescriptorSetLayout,device, &texSetLayoutCIO, nullptr, &textureDescSetLayout);

    // allocate set
    std::array uboSetLayouts = {uboDescSetLayout, uboDescSetLayout};
    std::array texSetLayouts = {textureDescSetLayout, textureDescSetLayout};
    auto uboSetAllocInfo = FnDescriptor::setAllocateInfo(*pDescriptorPool, uboSetLayouts);
    auto texSetAllocInfo = FnDescriptor::setAllocateInfo(*pDescriptorPool, texSetLayouts);
    UT_Fn::invoke_and_check("Error create indirect pass ubo sets",vkAllocateDescriptorSets,device, &uboSetAllocInfo,setUBOs.data());
    UT_Fn::invoke_and_check("Error create indirect pass tex sets",vkAllocateDescriptorSets,device, &texSetAllocInfo,setTextures.data());

    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        std::vector<VkWriteDescriptorSet> writeSets;
        writeSets.emplace_back(FnDescriptor::writeDescriptorSet( setUBOs[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &uboBuffers[i].descBufferInfo));
        for(auto &&[idx, tex] : UT_Fn::enumerate(pTextures))
            writeSets.emplace_back(FnDescriptor::writeDescriptorSet(setTextures[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,idx, &tex->descImageInfo));
        vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
    }
}

void IndirectDrawPass::preparePipelines() {
    const auto &mainDevice = pRenderer->getMainDevice();
    auto device = mainDevice.logicalDevice;
    //shader modules
    const auto sceneVertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/indirect_instance_vert.spv",  device);
    const auto sceneFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/indirect_instance_frag.spv",  device);
    //shader stages
    VkPipelineShaderStageCreateInfo sceneVertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, sceneVertModule);
    VkPipelineShaderStageCreateInfo sceneFragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, sceneFragModule);
    // point attributes desc
    constexpr int vertexBufferBindingID = 0;
    constexpr int instanceBufferBindingID  = 1;
    std::array<VkVertexInputAttributeDescription,11> attribsDesc{};
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
    attribsDesc[10] = { 10,instanceBufferBindingID,VK_FORMAT_R32_SINT , offsetof(InstanceData, texId)};

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
    pipelinePSOs.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
    UT_GraphicsPipelinePSOs::createPipeline(device, pipelinePSOs, pRenderer->getPipelineCache(), pipeline);
    UT_Fn::cleanup_shader_module(device,sceneVertModule, sceneFragModule);
}

void IndirectDrawPass::recordCommandBuffer() {
    VkDeviceSize offsets[1] = { 0 };
    const auto sceneCommandBuffer = pRenderer->getMainCommandBuffer();
    // [POI] Instanced multi draw rendering of the plants
    vkCmdBindPipeline(sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    // Binding point 0 : Mesh vertex buffer
    vkCmdBindVertexBuffers(sceneCommandBuffer, 0, 1, &verticesBuffer, offsets);
    // Binding point 1 : Instance data buffer
    vkCmdBindVertexBuffers(sceneCommandBuffer, 1, 1, &instanceBuffer, offsets);
    vkCmdBindIndexBuffer(sceneCommandBuffer, indicesBuffer , 0, VK_INDEX_TYPE_UINT32);
    std::array bindSets = {setUBOs[pRenderer->getCurrentFrame()], setTextures[pRenderer->getCurrentFrame()]};
    vkCmdBindDescriptorSets(sceneCommandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, bindSets.size(), bindSets.data(), 0, nullptr);
    vkCmdDrawIndexedIndirect(sceneCommandBuffer, indirectCommandBuffer, 0, indirectDrawCount, sizeof(VkDrawIndexedIndirectCommand));
}




LLVK_NAMESPACE_END