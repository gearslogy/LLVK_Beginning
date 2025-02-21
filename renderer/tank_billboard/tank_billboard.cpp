//
// Created by liuyangping on 2025/2/8.
//

#include "tank_billboard.h"
#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_VmaBuffer.hpp>
#include "libs/json.hpp"
#include "LLVK_UT_Json.hpp"
LLVK_NAMESPACE_BEGIN
void tank_billboard::createInstanceBuffer() {
    const char * path = "content/scene/tank_billboard/scene_json/instance.json";
    std::ifstream in(path);
    if (!in.good())
        throw std::runtime_error{std::string{"Could not open file "} + path + "."};
    nlohmann::json jsHandle;
    in >> jsHandle;
    if(not jsHandle.contains("points") )
        throw std::runtime_error{std::string{"Could not parse JSON file, no points key "} };
    const nlohmann::json &points = jsHandle["points"];

    std::vector <InstanceData> instances;
    for (auto &data: points) {
        const auto &P = data["P"].get<glm::vec3>();
        const auto &orient = data["orient"].get<glm::vec4>();
        const auto &pscale = data["scale"].get<float>();
        instances.emplace_back(InstanceData{P, orient, pscale});
    }
    std::cout << "[[tank_billboard instance points]]" << " npts:" << instances.size() << std::endl;

    instanceCount = std::size(instances);
    VkDeviceSize bufferSize = sizeof(InstanceData) * instanceCount;
    geomManager.createBufferWithStagingBuffer<VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT>(bufferSize, instances.data());
    instanceBuffer = geomManager.createVertexBuffers.back().buffer;
}



void tank_billboard::prepare() {
    mainCamera.mMoveSpeed = 20;
    mainCamera.setRotation({-15,689,-0});
    mainCamera.mPosition = {-91,48,127};
    const auto &device = getMainDevice().logicalDevice;
    const auto &phyDevice = getMainDevice().physicalDevice;
    setRequiredObjectsByRenderer(this, geomManager);
    setRequiredObjectsByRenderer(this, uboBuffers);
    setRequiredObjectsByRenderer(this, diffTex);
    createDescPool();

    // tex and sampler
    colorSampler = FnImage::createImageSampler(phyDevice,device);
    diffTex.create("content/scene/tank_billboard/tree2/diff.png",colorSampler);
    // geometry
    GLTFLoaderV2::CustomAttribLoader<VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID> attribSet;
    mGeoLoader.load("content/scene/tank_billboard/geo/tree02.gltf", attribSet);
    std::cout << "indices size:" <<std::size(mGeoLoader.parts[0].indices) << std::endl;
    UT_VmaBuffer::addGeometryToSimpleBufferManager(mGeoLoader,geomManager);
    createInstanceBuffer();
    for (auto &ubo: uboBuffers) {
        ubo.createAndMapping(sizeof(uboData));
    }
    createDescSetsAndDescSetLayout();

    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/tank_billboard_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/tank_billboard_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(getMainRenderPass());

    std::array<VkVertexInputAttributeDescription,11> attribsDesc{};
    attribsDesc[0] = { 0,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID, P)};
    attribsDesc[1] = { 1,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID, N)};
    attribsDesc[2] = { 2,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID, T)};
    attribsDesc[3] = { 3,0,VK_FORMAT_R32G32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID, uv0)};
    attribsDesc[4] = { 4,0,VK_FORMAT_R32G32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID, uv1)};
    attribsDesc[5] = { 5,0,VK_FORMAT_R32G32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID, uv2)};
    attribsDesc[6] = { 6,0,VK_FORMAT_R32G32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID, uv3)};
    attribsDesc[7] = { 7,0,VK_FORMAT_R32_SINT , offsetof(VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID, idx)};
    // Per-Instance attributes
    attribsDesc[8] = { 8,1,VK_FORMAT_R32G32B32_SFLOAT , offsetof(InstanceData, P)};
    attribsDesc[9] = { 9,1,VK_FORMAT_R32G32B32A32_SFLOAT , offsetof(InstanceData, orient)};
    attribsDesc[10] = { 10,1,VK_FORMAT_R32_SFLOAT , offsetof(InstanceData, pscale)};

    // binding desc
    VkVertexInputBindingDescription vertexBinding{0, sizeof(VTXFmt_P_N_T_UV0_UV1_UV2_UV3_ID), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputBindingDescription instanceBinding{1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE};
    std::array<VkVertexInputBindingDescription,2> bindingsDesc{vertexBinding, instanceBinding};
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);

    UT_GraphicsPipelinePSOs::createPipeline(device, pso, getPipelineCache(), pipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);

}


void tank_billboard::cleanupObjects() {
    const auto &device = getMainDevice().logicalDevice;
    const auto &phyDevice = getMainDevice().physicalDevice;
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_pipeline(device, pipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, setLayout);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_sampler(device, colorSampler);
    UT_Fn::cleanup_resources(geomManager, diffTex);
    UT_Fn::cleanup_descriptor_pool(device, descPool);
}

void tank_billboard::createDescSetsAndDescSetLayout() {
    const auto &device = getMainDevice().logicalDevice;
    const auto &phyDevice = getMainDevice().physicalDevice;
    // set layout
    using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO, MetaDesc::CIS>; // MVP
    using descPos = MetaDesc::desc_binding_position_t<0,1>;
    using descBindingUsage = MetaDesc::desc_binding_usage_t< VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT>;
    constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
    const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
    if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&setLayout) != VK_SUCCESS) throw std::runtime_error("error create set layout");
    // allocate sets
    std::array<VkDescriptorSetLayout,2> layouts = {setLayout, setLayout};
    auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
    UT_Fn::invoke_and_check("create scene sets-0 error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, descSets.data());
    // update sets
    namespace FnDesc = FnDescriptor;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::array<VkWriteDescriptorSet, 2> writes = {
            FnDesc::writeDescriptorSet(descSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),
            FnDesc::writeDescriptorSet(descSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 , &diffTex.descImageInfo)
        };
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
    }
    // pipelineLayout and pipeline
    const std::array setLayouts{setLayout}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );
}



void tank_billboard::render() {
    updateUBO();
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}

void tank_billboard::recordCommandBuffer() const {
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    const auto &cmdBuf = activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("begin shadow command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    std::vector<VkClearValue> sceneClearValues(2);
    sceneClearValues[0].color = {0.4, 0.4, 0.4, 1};
    sceneClearValues[1].depthStencil = { 1.0f, 0 };
    const auto sceneRenderPass =getMainRenderPass();
    const auto sceneRenderExtent = getSwapChainExtent();
    const auto sceneFramebuffer = getMainFramebuffer();
    const auto sceneRenderPassBeginInfo = FnCommand::renderPassBeginInfo(sceneFramebuffer, sceneRenderPass, sceneRenderExtent,sceneClearValues);

    vkCmdBeginRenderPass(cmdBuf, &sceneRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    auto [vs_width , vs_height] = sceneRenderExtent;
    auto viewport = FnCommand::viewport(vs_width,vs_height );
    auto scissor = FnCommand::scissor(vs_width, vs_height);
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDeviceSize offsets[1] = { 0 };
    // render instance geometry
    const auto &geoPart = mGeoLoader.parts[0];
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &geoPart.verticesBuffer, offsets);     // Binding point 0 : Mesh vertex buffer
    vkCmdBindVertexBuffers(cmdBuf, 1, 1, &instanceBuffer, offsets);             // Binding point 1 : Instance data buffer
    vkCmdBindIndexBuffer(cmdBuf,geoPart.indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
        1, &descSets[currentFlightFrame], 0, nullptr);
    vkCmdDrawIndexed(cmdBuf, geoPart.indices.size(), instanceCount, 0, 0, 0);


    vkCmdEndRenderPass(cmdBuf);
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );

}


void tank_billboard::createDescPool() {
    const auto &device = getMainDevice().logicalDevice;
    // desc pool
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    if (result != VK_SUCCESS) throw std::runtime_error{"ERROR"};
}

void tank_billboard::updateUBO() {
    auto [width, height] =   getSwapChainExtent();
    auto &&mainCamera = getMainCamera();
    const auto frame = getCurrentFlightFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = mainCamera.view();
    uboData.model = glm::mat4(1.0f);
    memcpy(uboBuffers[frame].mapped, &uboData, sizeof(uboData));
}

LLVK_NAMESPACE_END