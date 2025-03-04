﻿//
// Created by liuya on 12/24/2024.
//

#include "RbdVatStorageBufferRenderer.h"

#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_VmaBuffer.hpp>

#include "libs/json.hpp"
#include "LLVK_UT_Json.hpp"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN
    RbdVatStorageBufferRenderer::RbdVatStorageBufferRenderer() {
    mainCamera.setRotation({1.18, 7.34, 0});
    mainCamera.mPosition = glm::vec3(10, 4.4, 59);
    //mainCamera.updateCameraVectors();  // DO NOT CALL THIS , because setRotation will update mFront mRight mUp
}

void RbdVatStorageBufferRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    UT_Fn::cleanup_resources(geomManager, texDiff, ssboBuffer);
    UT_Fn::cleanup_sampler(device, colorSampler);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_pipeline(device, scenePipeline);
    UT_Fn::cleanup_descriptor_set_layout(device,descSetLayout_set0, descSetLayout_set1);
}

void RbdVatStorageBufferRenderer::prepare() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    setRequiredObjectsByRenderer(this, geomManager);
    setRequiredObjectsByRenderer(this, texDiff);
    auto fracture_index_loader = GLTFLoaderV2::CustomAttribLoader<GLTFVertexVATFracture>{};
    // 1.geo
    buildings.load("content/scene/rbdvat_ssbo/gltf/destruct_house.gltf", std::move(fracture_index_loader));
    UT_VmaBuffer::addGeometryToSimpleBufferManager(buildings,geomManager);
    // 2.samplers
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    // 3.tex
    texDiff.create("content/scene/rbdvat_ssbo/resources/gpu_textures/39_MedBuilding_gpu_D.ktx2", colorSampler);

    // desc pool
    std::array<VkDescriptorPoolSize, 3> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    if (result != VK_SUCCESS) throw std::runtime_error{"ERROR"};

    prepareUBO();
    parseStorageData();
    prepareSSBO();
    prepareDescSets();
    preparePipeline();
}

void RbdVatStorageBufferRenderer::render(){
    updateTime();
    updateUBO();
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}
void RbdVatStorageBufferRenderer::updateUBO() {
    auto [width, height] =   getSwapChainExtent();
    auto &&mainCamera = getMainCamera();
    const auto frame = getCurrentFlightFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = mainCamera.view();
    uboData.model = glm::mat4(1.0f);
    auto tcFrame =  fmod(tc_currentFrame, numFrames);
    //std::cout << " tc frame:" <<tcFrame << std::endl;
    uboData.timeData = { tcFrame, static_cast<float>(numPacks), 0, 0}; // 确保在0-59范围内循环
    memcpy(uboBuffers[frame].mapped, &uboData, sizeof(uboData));
}


void RbdVatStorageBufferRenderer::updateTime() {
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto tc_currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(tc_currentTime - lastTime).count();
    lastTime = tc_currentTime;

    // 累加时间
    tc_accumulator += deltaTime;

    // 当累积的时间超过帧时间时更新
    while (tc_accumulator >= tc_frameTime) {
        tc_currentFrame += 1.0f;
        tc_accumulator -= tc_frameTime;
    }
}


void RbdVatStorageBufferRenderer::parseStorageData() {
    using json = nlohmann::json;
    json jsHandle;
    std::string_view path = "content/scene/rbdvat_ssbo/json/rbd.json";
    std::ifstream in(path.data());
    if (!in.good())
        throw std::runtime_error{std::string{"Could not open file "} + path.data() + "."};
    in >> jsHandle;
    std::cout << "reading json file end\n";

    const auto &jsData = jsHandle["data"].get_ref<const nlohmann::json::array_t&>();
    numPacks = jsHandle["npts"];
    numFrames = jsHandle["num_frames"];
    /*
        {
            P: [[FRAME0 DATA],[FRAME1 DATA],[...],[...]],
            orient:[[],[],[],[...],[...]]
        }
    */
    for (auto &elem : jsData) {
        auto p = glm::vec4{elem[0],elem[1],elem[2],1};
        auto orient = glm::vec4{elem[3],elem[4],elem[5],elem[6]};
        RBDData data{p, orient};
        rbdData.emplace_back(data);
    }
}
void RbdVatStorageBufferRenderer::prepareSSBO() {
    setRequiredObjectsByRenderer(this, ssboBuffer);
    VkDeviceSize bufferSize = sizeof(RBDData) * rbdData.size();
    ssboBuffer.createAndMapping(bufferSize);
    memcpy(ssboBuffer.mapped, rbdData.data(), bufferSize);
}


void RbdVatStorageBufferRenderer::prepareDescSets() {
        const auto &device = mainDevice.logicalDevice;

    {//set0 desc
        using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO>; // MVP
        using descPos = MetaDesc::desc_binding_position_t<0>;
        using descBindingUsage = MetaDesc::desc_binding_usage_t< VK_SHADER_STAGE_VERTEX_BIT>; // MVP
        constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
        const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
        if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&descSetLayout_set0) != VK_SUCCESS) throw std::runtime_error("error create set0 layout");

        std::array<VkDescriptorSetLayout,2> layouts = {descSetLayout_set0, descSetLayout_set0}; // must be two, because we USE MAX_FLIGHT_FRAME
        auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
        UT_Fn::invoke_and_check("create scene sets-0 error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, descSets_set0.data());
        // update sets
        namespace FnDesc = FnDescriptor;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet, 1> writes = {
                FnDesc::writeDescriptorSet(descSets_set0[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),          // scene model_view_proj_instance , used in VS shader
            };
            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        }

    }
    {//set1 desc
        using descTypes = MetaDesc::desc_types_t<MetaDesc::CIS, MetaDesc::SSBO>; // base/vat
        using descPos = MetaDesc::desc_binding_position_t<0,1>;
        using descBindingUsage = MetaDesc::desc_binding_usage_t<
            VK_SHADER_STAGE_FRAGMENT_BIT, // base color tex
            VK_SHADER_STAGE_VERTEX_BIT // P and orients VAT
        >;
        constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
        const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
        if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&descSetLayout_set1) != VK_SUCCESS) throw std::runtime_error("error create set1 layout");

        std::array<VkDescriptorSetLayout,2> layouts = {descSetLayout_set1, descSetLayout_set1}; // must be two, because we USE MAX_FLIGHT_FRAME
        auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
        UT_Fn::invoke_and_check("create scene sets-1 error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, descSets_set1.data());
        // update sets
        namespace FnDesc = FnDescriptor;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet,2> writes = {
                FnDesc::writeDescriptorSet(descSets_set1[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texDiff.descImageInfo),
                FnDesc::writeDescriptorSet(descSets_set1[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &ssboBuffer.descBufferInfo),
            };
            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        }
    }


    // pipeline layout
    const std::array setLayouts{descSetLayout_set0, descSetLayout_set1}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );

}


void RbdVatStorageBufferRenderer::preparePipeline(){
    const auto &device = mainDevice.logicalDevice;
    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/rbd_vat_storage_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/rbd_vat_storage_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(getMainRenderPass());

    constexpr int vertexBufferBindingID = 0;
    std::array<VkVertexInputAttributeDescription,6> attribsDesc{};
    attribsDesc[0] = { 0,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertexVATFracture, P)};
    attribsDesc[1] = { 1,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertexVATFracture, Cd)};
    attribsDesc[2] = { 2,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertexVATFracture, N)};
    attribsDesc[3] = { 3,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(GLTFVertexVATFracture, T)};
    attribsDesc[4] = { 4,vertexBufferBindingID,VK_FORMAT_R32G32_SFLOAT , offsetof(GLTFVertexVATFracture, uv0) };
    attribsDesc[5] = { 5,vertexBufferBindingID,VK_FORMAT_R32_SINT , offsetof(GLTFVertexVATFracture, fractureIndex)};
    VkVertexInputBindingDescription vertexBinding{vertexBufferBindingID, sizeof(GLTFVertexVATFracture), VK_VERTEX_INPUT_RATE_VERTEX};
    std::array bindingsDesc{vertexBinding};
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);


    UT_GraphicsPipelinePSOs::createPipeline(device, pso, getPipelineCache(), scenePipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);
}
void RbdVatStorageBufferRenderer::prepareUBO() {
    setRequiredObjectsByRenderer(this, uboBuffers);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uboData));
}


void RbdVatStorageBufferRenderer::recordCommandBuffer() {
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.0f, 0.0f, 0.0, 0.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    auto cmdBuf  = getMainCommandBuffer();
    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(getMainFramebuffer(),
        simplePass.pass,
        &simpleSwapchain.swapChainExtent,clearValues);
    const auto [width , height]= getSwapChainExtent();
    UT_Fn::invoke_and_check("begin vat storage pass command", vkBeginCommandBuffer, cmdBuf, &cmdBufferBeginInfo);
    {
        vkCmdBeginRenderPass(cmdBuf, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS ,scenePipeline);
        auto viewport = FnCommand::viewport(width, height );
        auto scissor = FnCommand::scissor(width, height );
        vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuf,0, 1, &scissor);

        auto bindSets = {descSets_set0[currentFlightFrame], descSets_set1[currentFlightFrame]};
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
             0, std::size(bindSets), std::data(bindSets) , 0, nullptr);

        VkDeviceSize offsets[1] = {0};

        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &buildings.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,buildings.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuf, buildings.parts[0].indices.size(), 1, 0, 0, 0);
        vkCmdEndRenderPass(cmdBuf);
    }
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}



LLVK_NAMESPACE_END