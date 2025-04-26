//
// Created by liuya on 4/8/2025.
//

#include "MultiViewPorts.h"

#include <LLVK_UT_VmaBuffer.hpp>

#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN


inline constexpr std::array<VkVertexInputAttributeDescription, 4> attribsDesc{
                    {
                        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VTXFmt_P_N_T_UV0, P)},
                        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VTXFmt_P_N_T_UV0, N)},
                        {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VTXFmt_P_N_T_UV0, T)},
                        {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VTXFmt_P_N_T_UV0, uv0)},
                    }
};
inline constexpr VkVertexInputBindingDescription vertexBinding{0, sizeof(VTXFmt_P_N_T_UV0), VK_VERTEX_INPUT_RATE_VERTEX};
inline constexpr std::array bindingsDesc{vertexBinding};

void MultiViewPorts::prepare() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    // Ready pool
    HLP::createSimpleDescPool(device, descPool);
    colorSampler = FnImage::createImageSampler(phyDevice, device);
}
void MultiViewPorts::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_sampler(device, colorSampler);
    UT_Fn::cleanup_resources(grid, tree);
    UT_Fn::cleanup_descriptor_pool(device, descPool);
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
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.gBuffer);
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