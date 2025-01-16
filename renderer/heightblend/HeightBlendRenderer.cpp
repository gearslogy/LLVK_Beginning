//
// Created by liuyangping on 2025/1/6.
//
#include "LLVK_Descriptor.hpp"
#include "LLVK_UT_VmaBuffer.hpp"
#include "Pipeline.hpp"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "HeightBlendRenderer.h"
LLVK_NAMESPACE_BEGIN
HeightBlendRenderer::HeightBlendRenderer() {
    mainCamera.setRotation({-53, -4.4, -2});
    mainCamera.mPosition = glm::vec3(0.448112, 7.82476, 5.99322);

}

void HeightBlendRenderer::prepare() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    setRequiredObjectsByRenderer(this, geomManager);
    setRequiredObjectsByRenderer(this, texDiff, texNDR, texIndex, texExtUV);
    auto fracture_index_loader = GLTFLoaderV2::CustomAttribLoader<VTXFmt_P_N_T_UV0>{};
    // 1.geo
    grid.load("content/scene/heightblend/gltf/grid.gltf", std::move(fracture_index_loader));
    UT_VmaBuffer::addGeometryToSimpleBufferManager(grid,geomManager);
    // 2.samplers
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    pointSampler = FnImage::createExrVATSampler(device);
    // 3.tex
    texDiff.create("content/scene/heightblend/texture_processed/diff.ktx2", colorSampler);
    texNDR.create("content/scene/heightblend/texture_processed/ndr.ktx2", colorSampler);
    texIndex.create("content/scene/heightblend/render/mix.png", pointSampler, VK_FORMAT_R8G8B8A8_UNORM);
    texExtUV.create("content/scene/heightblend/render/ext_uv_comp.exr", colorSampler);
    // desc pool
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    if (result != VK_SUCCESS) throw std::runtime_error{"ERROR"};

    prepareUBO();
    prepareDescSets();
    preparePipeline();
}

void HeightBlendRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    UT_Fn::cleanup_resources(geomManager, texDiff, texNDR, texIndex, texExtUV);
    UT_Fn::cleanup_sampler(device, colorSampler, pointSampler);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_pipeline(device, scenePipeline);
    UT_Fn::cleanup_descriptor_set_layout(device,descSetLayout_set0, descSetLayout_set1);
}

void HeightBlendRenderer::prepareDescSets() {
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
        using descTypes = MetaDesc::desc_types_t<MetaDesc::CIS, MetaDesc::CIS, MetaDesc::CIS, MetaDesc::CIS>; // base/vat
        using descPos = MetaDesc::desc_binding_position_t<0,1,2,3>;
        using descBindingUsage = MetaDesc::desc_binding_usage_t<
            VK_SHADER_STAGE_FRAGMENT_BIT, // diff texture array
            VK_SHADER_STAGE_FRAGMENT_BIT,  // nor texture array
            VK_SHADER_STAGE_FRAGMENT_BIT,   // index map
            VK_SHADER_STAGE_FRAGMENT_BIT   // ext uv
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
            std::array<VkWriteDescriptorSet,4> writes = {
                FnDesc::writeDescriptorSet(descSets_set1[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texDiff.descImageInfo),          // scene model_view_proj_instance , used in VS shader
                FnDesc::writeDescriptorSet(descSets_set1[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &texNDR.descImageInfo),          // scene model_view_proj_instance , used in VS shader
                FnDesc::writeDescriptorSet(descSets_set1[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &texIndex.descImageInfo),          // scene model_view_proj_instance , used in VS shader
                FnDesc::writeDescriptorSet(descSets_set1[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &texExtUV.descImageInfo),          // scene model_view_proj_instance , used in VS shader
            };
            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        }
    }


    // pipeline layout
    const std::array setLayouts{descSetLayout_set0, descSetLayout_set1}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );

}
void HeightBlendRenderer::prepareUBO() {
    setRequiredObjectsByRenderer(this, uboBuffers);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uboData));
}
void HeightBlendRenderer::render() {
    updateUBO();
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}
void HeightBlendRenderer::preparePipeline() {
    const auto &device = mainDevice.logicalDevice;
    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/heightblend_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/heightblend_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(getMainRenderPass());

    constexpr int vertexBufferBindingID = 0;
    std::array<VkVertexInputAttributeDescription,4> attribsDesc{};
    attribsDesc[0] = { 0,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0, P)};
    attribsDesc[1] = { 1,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0, N)};
    attribsDesc[2] = { 2,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0, T)};
    attribsDesc[3] = { 3,vertexBufferBindingID,VK_FORMAT_R32G32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0, uv0) };
    VkVertexInputBindingDescription vertexBinding{vertexBufferBindingID, sizeof(VTXFmt_P_N_T_UV0), VK_VERTEX_INPUT_RATE_VERTEX};
    std::array bindingsDesc{vertexBinding};
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);

    UT_GraphicsPipelinePSOs::createPipeline(device, pso, getPipelineCache(), scenePipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);
}


void HeightBlendRenderer::updateUBO() {
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

void HeightBlendRenderer::recordCommandBuffer() const {
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.0f, 0.0f, 0.0, 0.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    auto cmdBuf  = getMainCommandBuffer();
    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(getMainFramebuffer(),
        simplePass.pass,
        &simpleSwapchain.swapChainExtent,clearValues);
    const auto [width , height]= getSwapChainExtent();
    UT_Fn::invoke_and_check("begin dual pass command", vkBeginCommandBuffer, cmdBuf, &cmdBufferBeginInfo);
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

        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &grid.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,grid.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuf, grid.parts[0].indices.size(), 1, 0, 0, 0);
        vkCmdEndRenderPass(cmdBuf);
    }
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}
LLVK_NAMESPACE_END