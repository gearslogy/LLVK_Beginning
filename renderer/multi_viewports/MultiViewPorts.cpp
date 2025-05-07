//
// Created by liuya on 4/8/2025.
//

#include "MultiViewPorts.h"
#include <LLVK_UT_VmaBuffer.hpp>
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN

void MultiViewPorts::prepare() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    // Ready pool
    HLP::createSimpleDescPool(device, descPool);
    colorSampler = FnImage::createImageSampler(phyDevice, device);
    prepareUBOs();
    prepareDescriptorSets();
    preparePipeline();

}
void MultiViewPorts::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_sampler(device, colorSampler);
    UT_Fn::cleanup_resources(grid, tree);
    UT_Fn::cleanup_descriptor_pool(device, descPool);
    UT_Fn::cleanup_pipeline(device, pipeline);
    UT_Fn::cleanup_resources(geomManager);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_descriptor_set_layout(device, setLayout);
    UT_Fn::cleanup_range_resources(uboBuffers);
}


void MultiViewPorts::loadGeometry() {
    namespace fs = std::filesystem;
    const fs::path ROOT = "content/scene/multi_viewports";
    fs::path gltfRoot = ROOT/"gltf";
    fs::path texRoot = ROOT/"textures";

    setRequiredObjectsByRenderer(this, geomManager);
    GLTFLoaderV2::CustomAttribLoader<Geometry::vertex_t> geoAttribSet;
    grid.geoLoader.load(gltfRoot/"grid.gltf", geoAttribSet);
    tree.geoLoader.load(gltfRoot/"tree.gltf", geoAttribSet);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(grid.geoLoader,geomManager);
    UT_VmaBuffer::addGeometryToSimpleBufferManager(tree.geoLoader,geomManager);


    setRequiredObjectsByRenderer(this, grid.diff, grid.nrm);
    setRequiredObjectsByRenderer(this, tree.diff, tree.nrm);
    grid.diff.create(texRoot/"grid_diff.png", colorSampler);
    grid.nrm.create(texRoot/"grid_nrm.png", colorSampler);
    tree.diff.create(texRoot/"tree_diff.png", colorSampler);
    tree.nrm.create(texRoot/"tree_nrm.png", colorSampler);
}

void MultiViewPorts::prepareUBOs() {
    setRequiredObjectsByRenderer(this, uboBuffers);
    for (int i=0;i<uboBuffers.size();i++) {
        uboBuffers[i].createAndMapping(sizeof(UBO));
    }
}
void MultiViewPorts::updateUBOs() {


    for (int i=0;i<uboBuffers.size();i++) {
        memcpy(uboBuffers[i].mapped, &ubo, sizeof(UBO));
    }
}


void MultiViewPorts::prepareDescriptorSets() {
    // 1 set
    // binding=0 UBO
    // binding=1 diff
    // binding=2 NRM
    const auto &device = mainDevice.logicalDevice;
    using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO, MetaDesc::CIS, MetaDesc::CIS>;
    using descPos = MetaDesc::desc_binding_position_t<0,1,2>;
    using descBindingUsage = MetaDesc::desc_binding_usage_t< VK_SHADER_STAGE_GEOMETRY_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT>;
    constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
    const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
    if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&setLayout) != VK_SUCCESS) throw std::runtime_error("error create set0 layout");

    std::array<VkDescriptorSetLayout,2> layouts = {setLayout, setLayout}; // must be two, because we USE MAX_FLIGHT_FRAME
    auto sceneSetAllocInfo = FnDescriptor::setAllocateInfo(descPool, layouts );
    UT_Fn::invoke_and_check("create scene tree sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, tree.sets.data());
    UT_Fn::invoke_and_check("create scene grid sets error", vkAllocateDescriptorSets,device, &sceneSetAllocInfo, grid.sets.data());
    // update sets
    namespace FnDesc = FnDescriptor;
    auto writeSets = [&device, this](Geometry &geo) {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::array<VkWriteDescriptorSet, 3> writes = {
                FnDesc::writeDescriptorSet(geo.sets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),
                FnDesc::writeDescriptorSet(geo.sets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1, &tree.diff.descImageInfo),
                FnDesc::writeDescriptorSet(geo.sets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,2, &tree.nrm.descImageInfo),
            };
            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        }
    };
    writeSets(tree);
    writeSets(grid);

    // 4. create pipeline layout
    const std::array offscreenSetLayouts{setLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(offscreenSetLayouts); // ONLY ONE SET
    UT_Fn::invoke_and_check("ERROR create offscreen pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );
}


void MultiViewPorts::preparePipeline() {
    const auto &device = mainDevice.logicalDevice;
    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/multiview_vert.spv",  device);
    const auto gsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/multiview_geom.spv",  device);
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/multiview_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);
    VkPipelineShaderStageCreateInfo gsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT, vsMD);
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, gsMD, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(getMainRenderPass());
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(HLP::VTXAttrib::VTXFmt_P_N_T_UV0_BindingsDesc,
        HLP::VTXAttrib::VTXFmt_P_N_T_UV0_AttribsDesc);
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, getPipelineCache(), pipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);
}


void MultiViewPorts::render() {

}


void MultiViewPorts::recordCommandBuffer() {
    const auto &cmdBuf = getMainCommandBuffer();
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    auto [width, height] = getSwapChainExtent();
    VkDeviceSize offsets[1] = { 0 };
    auto viewport = FnCommand::viewport(width,height );
    auto scissor = FnCommand::scissor(width,height);
    // clear
    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    // geo render
    auto renderGeo = [&cmdBuf, &offsets](auto &geo) {
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &geo.verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,geo.indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuf, geo.indices.size(), 1, 0, 0, 0);
    };


    const auto renderPassBeginInfo= FnCommand::renderPassBeginInfo(getMainFramebuffer(),
        simplePass.pass,
        getSwapChainExtent(),
        clearValues);
    UT_Fn::invoke_and_check("Rendering Error", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);

    //<0>------------- render depth -----------



    //<1> ------------   render secene ------
    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    // ---------------------------  RENDER PASS ---------------------------
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.gBuffer, 0, 1, &descSets.gBufferBook[currentFlightFrame], 0 , nullptr);
    const auto &book = resourceLoader->book.geoLoader.parts[0];
    vkCmdPushConstants(cmdBuf, pipelineLayouts.gBuffer, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(subpass::xform), &resourceLoader->book.xform);
    renderGeo(book);

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.gBuffer, 0, 1, &descSets.gBufferTelevision[currentFlightFrame], 0 , nullptr);
    const auto &television = resourceLoader->television.geoLoader.parts[0];
    vkCmdPushConstants(cmdBuf, pipelineLayouts.gBuffer, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(subpass::xform),&resourceLoader->television.xform);
    renderGeo(television);



    // ----------------------------
    vkCmdEndRenderPass(cmdBuf);
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}



LLVK_NAMESPACE_END