//
// Created by liuyangping on 2024/8/7.
//
#include "ktx_texture.h"
#include "LLVK_Descriptor.hpp"
#include "Pipeline.hpp"
LLVK_NAMESPACE_BEGIN


void ktx_texture::bindResources() {
    geoBufferManager.requiredObjects.allocator = vmaAllocator;
    geoBufferManager.requiredObjects.device = mainDevice.logicalDevice;
    geoBufferManager.requiredObjects.physicalDevice = mainDevice.physicalDevice;
    geoBufferManager.requiredObjects.commandPool = graphicsCommandPool;
    geoBufferManager.requiredObjects.queue = mainDevice.graphicsQueue;

    uboBuffer.requiredObjects.allocator = vmaAllocator;
    uboBuffer.requiredObjects.device = mainDevice.logicalDevice;
    uboBuffer.requiredObjects.physicalDevice = mainDevice.physicalDevice;
    uboBuffer.requiredObjects.queue = mainDevice.graphicsQueue;
    uboBuffer.requiredObjects.commandPool = graphicsCommandPool;

    uboTexture.requiredObjects.allocator = vmaAllocator;
    uboTexture.requiredObjects.device = mainDevice.logicalDevice;
    uboTexture.requiredObjects.physicalDevice = mainDevice.physicalDevice;
    uboTexture.requiredObjects.queue = mainDevice.graphicsQueue;
    uboTexture.requiredObjects.commandPool = graphicsCommandPool;

}
void ktx_texture::cleanupObjects() {
    vkDestroySampler(mainDevice.logicalDevice,sampler, nullptr);
    uboTexture.cleanup();
    uboBuffer.cleanup();
    geoBufferManager.cleanup();
    auto device = mainDevice.logicalDevice;
    vkDestroyDescriptorPool(mainDevice.logicalDevice, pipelineObject.descPool, nullptr);
    UT_Fn::cleanup_descriptor_set_layout(device,pipelineObject.uboSetLayout, pipelineObject.texSetLayout);
    UT_Fn::cleanup_pipeline_layout(device,pipelineObject.pipelineLayout);
    UT_Fn::cleanup_pipeline(device, pipelineObject.pipeline);
}
void ktx_texture::loadModel() {
    quad.init();
    constexpr VkBufferUsageFlags vertex_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkBufferUsageFlags index_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    size_t vertexBufferSize = sizeof(Basic::Vertex) * quad.vertices.size();
    size_t indexBufferSize = sizeof(uint32_t) * quad.indices.size();
    geoBufferManager.createBufferWithStagingBuffer<vertex_usage>(vertexBufferSize, quad.vertices.data());
    geoBufferManager.createBufferWithStagingBuffer<index_usage>(indexBufferSize, quad.indices.data());
}

void ktx_texture::loadTexture() {
    sampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    uboTexture.create("content/ktx_files/diff_B.ktx2", sampler);


}


void ktx_texture::setupDescriptors() {
    // 1 ubo
    // 1 tex
    namespace FnDesc = FnDescriptor;
    auto device = mainDevice.logicalDevice;

    std::array<VkDescriptorPoolSize, 2> poolSizes= {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  1}
    }};

    VkDescriptorPoolCreateInfo createInfo = FnDesc::poolCreateInfo(poolSizes, 2);

    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    if(vkCreateDescriptorPool(device, &createInfo, nullptr, &pipelineObject.descPool)!=VK_SUCCESS)
        throw std::runtime_error{"ERROR create descriptor pool"};

    // create UBO set layout
    const auto set0_binding0 = FnDesc::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);
    const std::array set0_bindings = {set0_binding0};
    const VkDescriptorSetLayoutCreateInfo ubo_createInfo = FnDesc::setLayoutCreateInfo(set0_bindings);
    if(vkCreateDescriptorSetLayout(device, &ubo_createInfo, nullptr, &pipelineObject.uboSetLayout)!=VK_SUCCESS)
        throw std::runtime_error{"Error create plant ubo set layout"};
    // create tex set layout

    const std::array set1_bindings = {
        FnDesc::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,0, VK_SHADER_STAGE_FRAGMENT_BIT)
    };
    const VkDescriptorSetLayoutCreateInfo tex_createInfo = FnDesc::setLayoutCreateInfo(set1_bindings);
    if(vkCreateDescriptorSetLayout(device, &tex_createInfo, nullptr, &pipelineObject.texSetLayout)!=VK_SUCCESS)
        throw std::runtime_error{"Error create plant tex set layout"};

    // create plant sets based on setLayouts
    const std::array layouts{pipelineObject.uboSetLayout, pipelineObject.texSetLayout};
    const auto plantSetAllocateInfo = FnDesc::setAllocateInfo(pipelineObject.descPool,layouts);
    if(vkAllocateDescriptorSets(device, &plantSetAllocateInfo,pipelineObject.sets) != VK_SUCCESS)
        throw std::runtime_error{"can not create plant descriptor set"};
    // -- write set end--
    std::vector writeSets{
        FnDesc::writeDescriptorSet(pipelineObject.sets[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffer.descBufferInfo ), // set=0 binding=0
        FnDesc::writeDescriptorSet(pipelineObject.sets[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &uboTexture.descImageInfo ),// set=1 binding=1
    };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()),
        writeSets.data(), 0, nullptr);
}


void ktx_texture::preparePipelines() {
     auto device = mainDevice.logicalDevice;
    const auto vertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/vma_vert.spv",  device);
    const auto fragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/vma_frag.spv",  device);
    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule);
    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule);
    // 1
    VkPipelineShaderStageCreateInfo shaderStates[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};
    // 2. vertex input
    std::array bindings = {Basic::Vertex::bindings()};
    auto attribs = Basic::Vertex::attribs();
    VkPipelineVertexInputStateCreateInfo vertexInput_ST_CIO = FnPipeline::vertexInputStateCreateInfo(bindings, attribs);
    // 3. assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly_ST_CIO = FnPipeline::inputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, VK_FALSE);
    // 4 viewport and scissor
    VkPipelineViewportStateCreateInfo viewport_ST_CIO = FnPipeline::viewPortStateCreateInfo();
    // 5. dynamic state
    auto dynamicsStates = FnPipeline::simpleDynamicsStates();
    VkPipelineDynamicStateCreateInfo dynamics_ST_CIO= FnPipeline::dynamicStateCreateInfo(dynamicsStates);
    // 6. rasterization
    VkPipelineRasterizationStateCreateInfo rasterization_ST_CIO = FnPipeline::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
    // 7. multisampling
    VkPipelineMultisampleStateCreateInfo multisample_ST_CIO=FnPipeline::multisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    // 8. blending
    std::array colorBlendAttamentState = {FnPipeline::simpleOpaqueColorBlendAttachmentState()};
    VkPipelineColorBlendStateCreateInfo blend_ST_CIO = FnPipeline::colorBlendStateCreateInfo(colorBlendAttamentState);

    // 9. pipeline layout
    const std::array plantLayouts{pipelineObject.uboSetLayout, pipelineObject.texSetLayout};
    VkPipelineLayoutCreateInfo layout_CIO = FnPipeline::layoutCreateInfo(plantLayouts);
    // create pipeline layout
    auto result = vkCreatePipelineLayout(device, &layout_CIO, nullptr, &pipelineObject.pipelineLayout);
    if(result != VK_SUCCESS) throw std::runtime_error{"ERROR create pipeline layout"};

    // 10
    VkPipelineDepthStencilStateCreateInfo ds_ST_CIO = FnPipeline::depthStencilStateCreateInfoEnabled();
    // 11. PIPELINE
    VkGraphicsPipelineCreateInfo pipeline_CIO = FnPipeline::pipelineCreateInfo();
    pipeline_CIO.stageCount = 2;
    pipeline_CIO.pStages = shaderStates;
    pipeline_CIO.pVertexInputState = &vertexInput_ST_CIO;
    pipeline_CIO.pInputAssemblyState = &inputAssembly_ST_CIO;
    pipeline_CIO.pViewportState = &viewport_ST_CIO;
    pipeline_CIO.pDynamicState = &dynamics_ST_CIO;
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
    pipeline_CIO.pMultisampleState = &multisample_ST_CIO;
    pipeline_CIO.pColorBlendState = &blend_ST_CIO;
    pipeline_CIO.pDepthStencilState = &ds_ST_CIO;
    pipeline_CIO.layout = pipelineObject.pipelineLayout;
    pipeline_CIO.renderPass = simplePass.pass ;
    pipeline_CIO.subpass = 0; // ONLY USE ONE PASS
    result = vkCreateGraphicsPipelines(device, simplePipelineCache.pipelineCache,
        1, &pipeline_CIO, nullptr, &pipelineObject.pipeline);
    if(result!= VK_SUCCESS) throw std::runtime_error{"Failed created graphics pipeline"};
    // finally destory shader module
    UT_Fn::cleanup_shader_module(device, vertModule, fragModule);
}

void ktx_texture::recordCommandBuffer() {
 // select framebuffer
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.6f, 0.65f, 0.4, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    const VkFramebuffer &framebuffer = getMainFramebuffer();
    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(framebuffer,
        simplePass.pass,
        &simpleSwapchain.swapChainExtent,clearValues);

    auto cmdBuf = getMainCommandBuffer();
    auto result = vkBeginCommandBuffer(cmdBuf, &cmdBufferBeginInfo);
    if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
    vkCmdBeginRenderPass(cmdBuf, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);

    VkDeviceSize offsets[1] = { 0 };
    // render ground
    //
    auto &verticesBuffer = geoBufferManager.createVertexBuffers[0].buffer;
    auto &indicesBuffer = geoBufferManager.createIndexedBuffers[0].buffer;
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS ,pipelineObject.pipeline);
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &verticesBuffer, offsets);
    vkCmdBindIndexBuffer(cmdBuf,indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineObject.pipelineLayout, 0, 2, pipelineObject.sets, 0, nullptr);
    vkCmdDrawIndexed(cmdBuf, quad.indices.size(), 1, 0, 0, 0);


    vkCmdEndRenderPass(cmdBuf);
    if (vkEndCommandBuffer(cmdBuf) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}
void ktx_texture::prepareUniformBuffers() {
    uboBuffer.createAndMapping(sizeof(uboData));
    updateUniformBuffer();
}

void ktx_texture::updateUniformBuffer() {
    memcpy(uboBuffer.mapped, &uboData, sizeof(uboData) );
}




LLVK_NAMESPACE_END