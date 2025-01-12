//
// Created by liuya on 1/6/2025.
//

#include "ScreenShotRenderer.h"

#include <chrono>
#include "LLVK_Descriptor.hpp"
#include "LLVK_UT_VmaBuffer.hpp"
#include "Pipeline.hpp"
#include "renderer/public/UT_CustomRenderer.hpp"


LLVK_NAMESPACE_BEGIN
ScreenShotRenderer::ScreenShotRenderer() {
    mainCamera.setRotation({-27.7798, 46.74863, 0});
    mainCamera.mPosition = glm::vec3(6, 5, 6);
    //mainCamera.updateCameraVectors(); // DO NOT CALL THIS
}

void ScreenShotRenderer::prepare() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    setRequiredObjectsByRenderer(this, geomManager);
    auto fracture_index_loader = GLTFLoaderV2::CustomAttribLoader<VTXFmt_P_N>{};
    // 1.geo
    scene.load("content/scene/screenshot/screenshot.gltf", std::move(fracture_index_loader));
    UT_VmaBuffer::addGeometryToSimpleBufferManager(scene,geomManager);

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

void ScreenShotRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    const auto &phyDevice = mainDevice.physicalDevice;
    UT_Fn::cleanup_resources(geomManager);
    UT_Fn::cleanup_sampler(device);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_pipeline_layout(device, pipelineLayout);
    UT_Fn::cleanup_pipeline(device, scenePipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, descSetLayout);
}

void ScreenShotRenderer::prepareDescSets() {
    const auto &device = mainDevice.logicalDevice;

    {//set0 desc
        using descTypes = MetaDesc::desc_types_t<MetaDesc::UBO>; // MVP
        using descPos = MetaDesc::desc_binding_position_t<0>;
        using descBindingUsage = MetaDesc::desc_binding_usage_t< VK_SHADER_STAGE_VERTEX_BIT>; // MVP
        constexpr auto sceneDescBindings = MetaDesc::generateSetLayoutBindings<descTypes,descPos,descBindingUsage>();
        const auto sceneSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(sceneDescBindings);
        if (vkCreateDescriptorSetLayout(device,&sceneSetLayoutCIO,nullptr,&descSetLayout) != VK_SUCCESS) throw std::runtime_error("error create set0 layout");

        std::array<VkDescriptorSetLayout,2> layouts = {descSetLayout, descSetLayout}; // must be two, because we USE MAX_FLIGHT_FRAME
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


    // pipeline layout
    const std::array setLayouts{descSetLayout}; // just one set
    VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(setLayouts);
    UT_Fn::invoke_and_check("ERROR create screenshot pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &pipelineLayout );

}
void ScreenShotRenderer::prepareUBO() {
    setRequiredObjectsByRenderer(this, uboBuffers);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uboData));
}
void ScreenShotRenderer::render() {
    updateUBO();
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}
void ScreenShotRenderer::preparePipeline() {
    const auto &device = mainDevice.logicalDevice;
    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/headlight_vert.spv",  device);    //shader modules
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/headlight_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
    VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
    pso.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
    pso.setPipelineLayout(pipelineLayout);
    pso.setRenderPass(getMainRenderPass());

    constexpr int vertexBufferBindingID = 0;
    std::array<VkVertexInputAttributeDescription,2> attribsDesc{};
    attribsDesc[0] = { 0,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N, P)};
    attribsDesc[1] = { 1,vertexBufferBindingID,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N, N)};
    VkVertexInputBindingDescription vertexBinding{vertexBufferBindingID, sizeof(VTXFmt_P_N), VK_VERTEX_INPUT_RATE_VERTEX};
    std::array bindingsDesc{vertexBinding};
    pso.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);


    UT_GraphicsPipelinePSOs::createPipeline(device, pso, getPipelineCache(), scenePipeline);
    UT_Fn::cleanup_shader_module(device,vsMD,fsMD);
}


void ScreenShotRenderer::updateUBO() {
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

void ScreenShotRenderer::recordCommandBuffer() {
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

        auto bindSets = {descSets_set0[currentFlightFrame]};
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
             0, std::size(bindSets), std::data(bindSets) , 0, nullptr);

        VkDeviceSize offsets[1] = {0};

        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &scene.parts[0].verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,scene.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuf, scene.parts[0].indices.size(), 1, 0, 0, 0);
        vkCmdEndRenderPass(cmdBuf);
    }
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}



void ScreenShotRenderer::capture() {
    // check support
    const auto device = getMainDevice().logicalDevice;
    const auto phyDevice = getMainDevice().physicalDevice;
    const auto queue = getMainDevice().graphicsQueue;
    const auto swapChainColorFormat = simpleSwapchain.swapChainFormat;
    const auto [width,height] = getSwapChainExtent();
    constexpr auto dstImageColorFormat = VK_FORMAT_B8G8R8A8_SNORM;
    VkFormatProperties formatProperties;
    // check source
    vkGetPhysicalDeviceFormatProperties(phyDevice, swapChainColorFormat, &formatProperties);
    if (not (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT ) ) {
        ssRes.supportSplit = false;
    }
    // check dst
    vkGetPhysicalDeviceFormatProperties(phyDevice, dstImageColorFormat, &formatProperties);
    if (not (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT ) ) { // 我们目标图像必须是线性储存
        ssRes.supportSplit = false;
    }

    VmaBufferRequiredObjects requiredObjects{};
    setRequiredObjectsByRenderer(this, requiredObjects);


    auto dstImageCIO = FnImage::imageCreateInfo(width,height);
    dstImageCIO.tiling = VK_IMAGE_TILING_LINEAR;
    dstImageCIO.format = dstImageColorFormat;
    FnVmaImage::createImageAndAllocation(requiredObjects,
         dstImageCIO, false,
        ssRes.dstImage,
        ssRes.dstAllocation
        );
    // transfer the dstImage layout: undefined -> DST_OPTIMAL
    FnImage::transitionImageLayout(device, getGraphicsCommandPool(), queue, ssRes.dstImage, dstImageColorFormat,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 1);

    // PRESENT-> SRC : ready to copy
    ssRes.srcImage = simpleSwapchain.swapChainImages[currentFlightFrame].image;
    FnImage::transitionImageLayout(device, getGraphicsCommandPool(), queue, ssRes.srcImage, swapChainColorFormat,
       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, 1);


    auto cmd = FnCommand::beginSingleTimeCommand(device, getGraphicsCommandPool());
    if (ssRes.supportSplit) { // RGB顺序copy
        VkOffset3D blitSize;
        blitSize.x = width;
        blitSize.y = height;
        blitSize.z = 1;
        VkImageBlit blitRegion = {};
        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.srcOffsets[1] = blitSize;
        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.layerCount = 1;
        blitRegion.dstOffsets[1] = blitSize;

        vkCmdBlitImage(
            cmd,
            ssRes.srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            ssRes.dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blitRegion,
            VK_FILTER_NEAREST);
    }
    else {
        // 注意这里不是RGB顺序 是BGR
        VkImageCopy copyRegion = {};
        copyRegion.srcOffset = {0, 0, 0};
        copyRegion.dstOffset = {0, 0, 0};
        copyRegion.extent.width = width;
        copyRegion.extent.height = height;
        copyRegion.extent.depth = 1;
        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.layerCount = 1;
        vkCmdCopyImage( cmd, ssRes.srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            ssRes.dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copyRegion );
    }
    FnCommand::endSingleTimeCommand(device, getGraphicsCommandPool(), queue, cmd);
    // copy image data to RGBA 


    // 把目标转换到 GENERAL_LAYOUT 可以 来映射内存
    FnImage::transitionImageLayout(device, getGraphicsCommandPool(), queue, ssRes.dstImage, swapChainColorFormat,
           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,1,1);
    // 把swapchain 转换回去到PRENSET_SRC_KHR
    FnImage::transitionImageLayout(device, getGraphicsCommandPool(), queue, ssRes.srcImage, swapChainColorFormat,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,1,1);


    vmaDestroyImage(vmaAllocator, ssRes.dstImage,ssRes.dstAllocation);

}




LLVK_NAMESPACE_END