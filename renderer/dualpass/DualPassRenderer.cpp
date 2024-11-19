//
// Created by liuya on 11/11/2024.
//

#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_VmaBuffer.hpp>
#include "DualpassRenderer.h"
#include "renderer/public/UT_CustomRenderer.hpp"

LLVK_NAMESPACE_BEGIN
DualPassRenderer::DualPassRenderer() {
    mainCamera.setRotation({-13,16,2.6});
    mainCamera.mPosition = {0.475032,2.13,2.40};
    mainCamera.mMoveSpeed = 0.01;
}

void DualPassRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_resources(geometryManager, tex);
    UT_Fn::cleanup_sampler(mainDevice.logicalDevice, colorSampler);
    vkDestroyDescriptorPool(device, descPool, VK_NULL_HANDLE);
    UT_Fn::cleanup_descriptor_set_layout(device, descSetLayout);
    UT_Fn::cleanup_pipeline(device, hairPipeline1, hairPipeline2);
    UT_Fn::cleanup_pipeline_layout(device, dualPipelineLayout);
    UT_Fn::cleanup_range_resources(uboBuffers);
}


void DualPassRenderer::prepare() {
    // 1. pool
    const auto &device = mainDevice.logicalDevice;
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    assert(result == VK_SUCCESS);
    // 2. UBO create
    setRequiredObjectsByRenderer(this, uboBuffers[0], uboBuffers[1]);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uboData));

    // 3 descSetLayout
    const std::array setLayoutBindings = {
        FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT),
        FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
    };
    const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(setLayoutBindings);
    UT_Fn::invoke_and_check("Error create desc set layout",vkCreateDescriptorSetLayout,device, &setLayoutCIO, nullptr, &descSetLayout);

    // 4. model resource loading
    colorSampler = FnImage::createImageSampler(mainDevice.physicalDevice, mainDevice.logicalDevice);
    setRequiredObjectsByRenderer(this, geometryManager);
    headLoader.load("content/scene/dualpass/head.gltf");
    hairLoader.load("content/scene/dualpass/hair.gltf");
    UT_VmaBuffer::addGeometryToSimpleBufferManager(headLoader, geometryManager); // GPU buffer allocation
    UT_VmaBuffer::addGeometryToSimpleBufferManager(hairLoader, geometryManager); // GPU buffer allocation
    setRequiredObjectsByRenderer(this, tex);
    tex.create("content/scene/dualpass/gpu_D.ktx2", colorSampler);

    // 5 create desc sets
    const std::array<VkDescriptorSetLayout,2> setLayouts({descSetLayout,descSetLayout});
    auto setAllocInfo = FnDescriptor::setAllocateInfo(descPool,setLayouts);
    UT_Fn::invoke_and_check("Error create RenderContainerOneSet::uboSets", vkAllocateDescriptorSets, device, &setAllocInfo, dualDescSets.data());

    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        std::array<VkWriteDescriptorSet,2> writeSets {
            FnDescriptor::writeDescriptorSet(dualDescSets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[currentFrame].descBufferInfo),
            FnDescriptor::writeDescriptorSet(dualDescSets[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &tex.descImageInfo)
        };
        vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
    }


    createDualPipelines();
}

void DualPassRenderer::updateDualUBOs() {
    auto [width, height] =  getSwapChainExtent();
    auto &&mainCamera = getMainCamera();
    const auto frame = getCurrentFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    uboData.proj = mainCamera.projection();
    uboData.proj[1][1] *= -1;
    uboData.view = mainCamera.view();
    uboData.model = glm::mat4(1.0f);
    memcpy(uboBuffers[frame].mapped, &uboData, sizeof(uboData));
}



void DualPassRenderer::render() {
    updateDualUBOs();
    recordCommandDual();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();
}



void DualPassRenderer::createDualPipelines() {

    // create BACK PIPELINE
    // 颜色混合状态



    const auto &device = mainDevice.logicalDevice;
    const std::array sceneSetLayouts{descSetLayout};
    VkPipelineLayoutCreateInfo sceneSetLayout_CIO = FnPipeline::layoutCreateInfo(sceneSetLayouts);
    UT_Fn::invoke_and_check("ERROR create scene pipeline layout",vkCreatePipelineLayout,device, &sceneSetLayout_CIO,nullptr, &dualPipelineLayout );

    //shader modules
    const auto vs0MD = FnPipeline::createShaderModuleFromSpvFile("shaders/hair_vert.spv",  device);
    const auto fs0MD = FnPipeline::createShaderModuleFromSpvFile("shaders/front_frag.spv",  device);
    //shader stages
    VkPipelineShaderStageCreateInfo front_vsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vs0MD);
    VkPipelineShaderStageCreateInfo front_fsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fs0MD);
    dualPso.requiredObjects.device = device;
    dualPso.setShaderStages(front_vsCIO, front_fsCIO);
    dualPso.setPipelineLayout(dualPipelineLayout);
    dualPso.setRenderPass(getMainRenderPass());
    dualPso.rasterizerStateCIO.cullMode = VK_CULL_MODE_FRONT_BIT;
    dualPso.rasterizerStateCIO.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    /*
    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask = 0;  // 禁用所有颜色通道的写入
    blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    */
    VkPipelineColorBlendAttachmentState colorBlendAttachmentPass1 = {};
    colorBlendAttachmentPass1.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                         VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT |
                                         VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentPass1.blendEnable = VK_TRUE;
    // 设置混合因子
    colorBlendAttachmentPass1.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // 源颜色使用源Alpha作为因子
    colorBlendAttachmentPass1.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // 目标颜色使用1-源Alpha作为因子
    colorBlendAttachmentPass1.colorBlendOp = VK_BLEND_OP_ADD;
    // alpha 混合设置
    colorBlendAttachmentPass1.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentPass1.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentPass1.alphaBlendOp = VK_BLEND_OP_ADD;

    std::array colorBlendAttachmentsPass1{colorBlendAttachmentPass1};
    VkPipelineColorBlendStateCreateInfo colorBlendStateCIO1 = FnPipeline::colorBlendStateCreateInfo(colorBlendAttachmentsPass1);
    dualPso.pipelineCIO.pColorBlendState = &colorBlendStateCIO1;
    UT_GraphicsPipelinePSOs::createPipeline(device, dualPso, getPipelineCache(), hairPipeline1);
    std::cout << "created front pipeline\n";



    /*
    *
    *finalColor.rgb = srcColor.rgb * srcAlpha + dstColor.rgb * (1 - srcAlpha)
finalColor.a = srcColor.a * 1 + dstColor.a * 0
     *
     */

    VkPipelineColorBlendAttachmentState colorBlendAttachmentPass2 = {};
    colorBlendAttachmentPass2.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                         VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT |
                                         VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentPass2.blendEnable = VK_TRUE;
    // 设置混合因子
    colorBlendAttachmentPass2.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // 源颜色使用源Alpha作为因子
    colorBlendAttachmentPass2.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // 目标颜色使用1-源Alpha作为因子
    colorBlendAttachmentPass2.colorBlendOp = VK_BLEND_OP_ADD;


    // alpha 混合设置
    colorBlendAttachmentPass2.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentPass2.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentPass2.alphaBlendOp = VK_BLEND_OP_ADD;

    std::array colorBlendAttachmentsPass2{colorBlendAttachmentPass2};
    VkPipelineColorBlendStateCreateInfo colorBlendStateCIO2 = FnPipeline::colorBlendStateCreateInfo(colorBlendAttachmentsPass2);
    dualPso1.pipelineCIO.pColorBlendState = &colorBlendStateCIO2;
    dualPso1.rasterizerStateCIO.cullMode = VK_CULL_MODE_BACK_BIT;  // 第二个Pass剔除背面
    dualPso1.depthStencilStateCIO.depthTestEnable = VK_TRUE;
    dualPso1.depthStencilStateCIO.depthWriteEnable = VK_FALSE;
    const auto vs1MD = FnPipeline::createShaderModuleFromSpvFile("shaders/hair_vert.spv",  device);
    const auto fs1MD = FnPipeline::createShaderModuleFromSpvFile("shaders/back_frag.spv",  device);
    VkPipelineShaderStageCreateInfo pass2_vsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vs1MD);
    VkPipelineShaderStageCreateInfo pass2_fsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fs1MD);
    dualPso1.setShaderStages(pass2_vsCIO, pass2_fsCIO);
    dualPso1.setPipelineLayout(dualPipelineLayout);
    dualPso1.setRenderPass(getMainRenderPass());
    UT_GraphicsPipelinePSOs::createPipeline(device, dualPso1, getPipelineCache(), hairPipeline2);

    UT_Fn::cleanup_shader_module(device,vs0MD, fs0MD, vs1MD,fs1MD);
    std::cout << "created back pipeline\n";
}

void DualPassRenderer::recordCommandDual() {
     // Clear values for all attachments written in the fragment shader
    std::vector<VkClearValue> clearValues{};
    clearValues.resize(2);
    // position, normal, albedo, roughness, displace;
    clearValues[0].color = { { 0.5f, 0.5f, 0.5f, 0.6f } };
    clearValues[1].depthStencil = { 1.0f, 0 };


    vkResetCommandBuffer(getMainCommandBuffer(),/*VkCommandBufferResetFlagBits*/ 0); //0: command buffer reset


    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(getMainFramebuffer(),
      getMainRenderPass(),&simpleSwapchain.swapChainExtent,clearValues);

    auto result = vkBeginCommandBuffer(getMainCommandBuffer(), &cmdBufferBeginInfo);
    if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
    vkCmdBeginRenderPass(getMainCommandBuffer(), &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


    VkDeviceSize offsets[1] = { 0 };
    auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );

    // -----------draw front-----------
    /*
    // 1-draw head
    vkCmdBindDescriptorSets(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, dualPipelineLayout,
       0, 1, &dualDescSets[currentFrame], 0, nullptr);
    vkCmdBindVertexBuffers(getMainCommandBuffer(), 0, 1, &headLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(getMainCommandBuffer(),headLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(getMainCommandBuffer(), headLoader.parts[0].indices.size(), 1, 0, 0, 0);*/


    // 2-draw hair

    vkCmdBindPipeline(getMainCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS , hairPipeline1);
    vkCmdSetViewport(getMainCommandBuffer(), 0, 1, &viewport);
    vkCmdSetScissor(getMainCommandBuffer(),0, 1, &scissor);
    vkCmdBindDescriptorSets(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, dualPipelineLayout,
 0, 1, &dualDescSets[currentFrame], 0, nullptr);
    vkCmdBindVertexBuffers(getMainCommandBuffer(), 0, 1, &hairLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(getMainCommandBuffer(),hairLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(getMainCommandBuffer(), hairLoader.parts[0].indices.size(), 1, 0, 0, 0);

    // -------------draw back-----------------

    vkCmdBindPipeline(getMainCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS , hairPipeline2);
    vkCmdSetViewport(getMainCommandBuffer(), 0, 1, &viewport);
    vkCmdSetScissor(getMainCommandBuffer(),0, 1, &scissor);
    vkCmdBindDescriptorSets(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, dualPipelineLayout,
 0, 1, &dualDescSets[currentFrame], 0, nullptr);
    vkCmdBindVertexBuffers(getMainCommandBuffer(), 0, 1, &hairLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(getMainCommandBuffer(),hairLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(getMainCommandBuffer(), hairLoader.parts[0].indices.size(), 1, 0, 0, 0);


    vkCmdEndRenderPass(getMainCommandBuffer());
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,getMainCommandBuffer() );
}


LLVK_NAMESPACE_END