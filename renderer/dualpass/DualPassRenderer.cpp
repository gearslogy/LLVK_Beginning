//
// Created by liuya on 11/11/2024.
//

#include <LLVK_Descriptor.hpp>
#include <LLVK_UT_VmaBuffer.hpp>
#include "DualpassRenderer.h"
#include "renderer/public/UT_CustomRenderer.hpp"

LLVK_NAMESPACE_BEGIN
void DualPassRenderer::cleanupObjects() {
    const auto &device = mainDevice.logicalDevice;
    UT_Fn::cleanup_resources(geometryManager, tex);
    UT_Fn::cleanup_sampler(mainDevice.logicalDevice, colorSampler);
    vkDestroyDescriptorPool(device, descPool, VK_NULL_HANDLE);
    UT_Fn::cleanup_descriptor_set_layout(device, descSetLayout);
    UT_Fn::cleanup_pipeline(device, frontPipeline, backPipeline);
    cleanupDualAttachment();
    vkDestroyFramebuffer(device, dualFrameBuffer,nullptr);
    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        vkDestroySemaphore(device, mrtSemaphores[i], nullptr);
    }
    cleanupComp();
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
            FnDescriptor::writeDescriptorSet(dualDescSets[currentFrame],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[currentFrame].descBufferInfo),
            FnDescriptor::writeDescriptorSet(dualDescSets[currentFrame],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &tex.descImageInfo)
        };
        vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
    }

    // render pass
    depthSampler = FnImage::createDepthSampler(device);
    createDualAttachment();
    createDualRenderPass();
    createDualFrameBuffer();
    createDualPipelines();

    prepareComp();
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
    recordCommandComp();

    //
    //    [[WAIT]]              ->         [[SIGNAL]]
    // imageAvailableSemaphore  ->      mrt semaphore
    // mrt semaphore            ->      renderFinishedSemaphore
    // renderFinished           ->      present
    //

    VkSubmitInfo mrtSubmitInfo = {};
    mrtSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    mrtSubmitInfo.commandBufferCount = 1;
    mrtSubmitInfo.waitSemaphoreCount = 1;
    mrtSubmitInfo.signalSemaphoreCount = 1;
    mrtSubmitInfo.pWaitDstStageMask = waitStages;
    mrtSubmitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];     // Wait for swap chain presentation to finish
    mrtSubmitInfo.pSignalSemaphores = & mrtSemaphores[currentFrame];     // Signal ready with offscreen semaphore
    // Submit mrt work
    mrtSubmitInfo.pCommandBuffers = &mrtCommandBuffers[currentFrame];
    UT_Fn::invoke_and_check("error submit mrt queue", vkQueueSubmit, mainDevice.graphicsQueue, 1, &mrtSubmitInfo, VK_NULL_HANDLE);

    VkSubmitInfo compSubmitInfo = {};
    compSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    compSubmitInfo.commandBufferCount = 1;
    compSubmitInfo.waitSemaphoreCount = 1;
    compSubmitInfo.signalSemaphoreCount = 1;
    compSubmitInfo.pWaitDstStageMask = waitStages;
    // Wait for offscreen semaphore
    compSubmitInfo.pWaitSemaphores = &mrtSemaphores[currentFrame];
    // Signal ready with render complete semaphore
    compSubmitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];
    // Submit composition work
    compSubmitInfo.pCommandBuffers = &activatedFrameCommandBufferToSubmit;
    UT_Fn::invoke_and_check("error submit render composition queue",vkQueueSubmit, mainDevice.graphicsQueue, 1, &compSubmitInfo, inFlightFences[currentFrame]);

    presentMainCommandBufferFrame();

}

void DualPassRenderer::swapChainResize() {
    const auto device = mainDevice.logicalDevice;
    cleanupDualAttachment();
    vkDestroyFramebuffer(device, dualFrameBuffer, nullptr);
    createDualAttachment();
    // update sets
    // comp
    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        std::vector<VkWriteDescriptorSet> writeSets;
        writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compDescSets[currentFrame], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &dualColorAttachment.descImageInfo));
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }
    createDualFrameBuffer();
}


void DualPassRenderer::createDualPipelines() {
    const auto &device = mainDevice.logicalDevice;
    const std::array sceneSetLayouts{descSetLayout};
    VkPipelineLayoutCreateInfo sceneSetLayout_CIO = FnPipeline::layoutCreateInfo(sceneSetLayouts);
    UT_Fn::invoke_and_check("ERROR create scene pipeline layout",vkCreatePipelineLayout,device, &sceneSetLayout_CIO,nullptr, &dualPipelineLayout );

    // first pass pipeline
    dualPso.rasterizerStateCIO.cullMode = VK_CULL_MODE_BACK_BIT;  // 第一个Pass剔除背面
    dualPso.rasterizerStateCIO.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    dualPso.depthStencilStateCIO.depthWriteEnable = VK_TRUE;// 第一个Pass写入深度
    dualPso.depthStencilStateCIO.stencilTestEnable = VK_FALSE;
    // 颜色混合状态
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                         VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT |
                                         VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;


    dualPso.colorBlendStateCIO.attachmentCount =1;
    dualPso.colorBlendStateCIO.pAttachments = &colorBlendAttachment;

    //shader modules
    const auto frontVertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/hair_vert.spv",  device);
    const auto frontFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/front_frag.spv",  device);
    //shader stages
    VkPipelineShaderStageCreateInfo front_vsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, frontVertModule);
    VkPipelineShaderStageCreateInfo front_fsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, frontFragModule);
    dualPso.setShaderStages(front_vsCIO, front_fsCIO);
    dualPso.setPipelineLayout(dualPipelineLayout);
    dualPso.setRenderPass(dualRenderPass);
    dualPso.pipelineCIO.subpass = 0;
    UT_GraphicsPipelinePSOs::createPipeline(device, dualPso, getPipelineCache(), frontPipeline);
    UT_Fn::cleanup_shader_module(device,frontVertModule, frontFragModule);
    std::cout << "created front pipeline\n";
    // create BACK PIPELINE
    //shader modules
    dualPso.rasterizerStateCIO.cullMode = VK_CULL_MODE_FRONT_BIT;  // 第二个Pass剔除背面
    dualPso.depthStencilStateCIO.depthTestEnable = VK_FALSE;
    const auto backVertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/hair_vert.spv",  device);
    const auto backFragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/back_frag.spv",  device);
    //shader stages
    VkPipelineShaderStageCreateInfo back_vsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, backVertModule);
    VkPipelineShaderStageCreateInfo back_fsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, backFragModule);
    dualPso.setShaderStages(back_vsCIO, back_fsCIO);
    dualPso.pipelineCIO.subpass = 1;
    UT_GraphicsPipelinePSOs::createPipeline(device, dualPso, getPipelineCache(), backPipeline);
    UT_Fn::cleanup_shader_module(device,backVertModule, backFragModule);
    std::cout << "created back pipeline\n";
}

void DualPassRenderer::createDualAttachment() {
    const auto [width, height] = getSwapChainExtent();
    setRequiredObjectsByRenderer(this, dualColorAttachment, dualDepthAttachment);
    VkImageUsageFlagBits colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    dualColorAttachment.create(width, height, VK_FORMAT_R8G8B8A8_UNORM, colorSampler,colorUsage);
    dualDepthAttachment.create(width, height, VK_FORMAT_D32_SFLOAT, depthSampler, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
void DualPassRenderer::createDualFrameBuffer() {
    std::array<VkImageView,2> attachments{};
    attachments[0] = dualColorAttachment.view;
    attachments[1] = dualDepthAttachment.view;

    VkFramebufferCreateInfo framebufferCreateInfo{};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.width = simpleSwapchain.swapChainExtent.width;
    framebufferCreateInfo.height = simpleSwapchain.swapChainExtent.height;
    framebufferCreateInfo.renderPass = dualRenderPass;
    framebufferCreateInfo.attachmentCount = attachments.size();
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.layers = 1;
    if(vkCreateFramebuffer(mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &dualFrameBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer");
    }
}

void DualPassRenderer::cleanupDualAttachment() {
    dualColorAttachment.cleanup();
    dualDepthAttachment.cleanup();
    vkDestroyFramebuffer(mainDevice.logicalDevice, dualFrameBuffer, nullptr);
}



void DualPassRenderer::createDualRenderPass() {
     // 附件描述
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM; // 根据实际情况选择格式
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // 保留之前的颜色
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT; // 需要实现这个函数来获取深度格式
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;  // 保留之前的深度
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 附件引用
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 创建两个SubPass
    VkSubpassDescription subpasses[2] = {};

    // 第一个SubPass - 渲染正面，写入深度
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &colorAttachmentRef;
    subpasses[0].pDepthStencilAttachment = &depthAttachmentRef;

    // 第二个SubPass - 渲染背面，只读深度
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &colorAttachmentRef;
    subpasses[1].pDepthStencilAttachment = &depthAttachmentRef;

    // SubPass依赖
    VkSubpassDependency dependencies[2] = {};

    // 第一个依赖 - 外部到第一个subpass
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = 0;

    // 第二个依赖 - 第一个subpass到第二个subpass
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = 1;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = 0;

    // 创建RenderPass
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    // 设置附件
    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;

    // 设置SubPass
    renderPassInfo.subpassCount = 2;
    renderPassInfo.pSubpasses = subpasses;

    // 设置依赖
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    if (vkCreateRenderPass(mainDevice.logicalDevice, &renderPassInfo, nullptr, &dualRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}
void DualPassRenderer::createDualCommandBuffers() {
    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = graphicsCommandPool ;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
    cbAllocInfo.commandBufferCount = 2;
    UT_Fn::invoke_and_check("create mrt commandBuffers error", vkAllocateCommandBuffers,
         mainDevice.logicalDevice, &cbAllocInfo, mrtCommandBuffers);
    VkSemaphoreCreateInfo semaphore_CIO{};
    semaphore_CIO.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for(auto & mrtSemaphore : mrtSemaphores) {
        UT_Fn::invoke_and_check("create mrt semaphores error",vkCreateSemaphore, mainDevice.logicalDevice, &semaphore_CIO, nullptr, &mrtSemaphore);
    }
}




void DualPassRenderer::recordCommandDual() {
     // Clear values for all attachments written in the fragment shader
    std::vector<VkClearValue> clearValues{};
    clearValues.resize(6);
    // position, normal, albedo, roughness, displace;
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[4].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[5].depthStencil = { 1.0f, 0 };

    auto mrtCommandBuffer = mrtCommandBuffers[currentFrame];
    vkResetCommandBuffer(mrtCommandBuffer,/*VkCommandBufferResetFlagBits*/ 0); //0: command buffer reset


    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(dualFrameBuffer,
      dualRenderPass,&simpleSwapchain.swapChainExtent,clearValues);

    auto result = vkBeginCommandBuffer(mrtCommandBuffer, &cmdBufferBeginInfo);
    if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
    vkCmdBeginRenderPass(mrtCommandBuffer, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(mrtCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , frontPipeline);

    VkDeviceSize offsets[1] = { 0 };
    auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    vkCmdSetViewport(mrtCommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(mrtCommandBuffer,0, 1, &scissor);
    // -----------draw front-----------
    // 1-draw head
    vkCmdBindDescriptorSets(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, dualPipelineLayout,
       0, 1, &dualDescSets[currentFrame], 0, nullptr);
    vkCmdBindVertexBuffers(mrtCommandBuffer, 0, 1, &headLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(mrtCommandBuffer,headLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(mrtCommandBuffer, headLoader.parts[0].indices.size(), 1, 0, 0, 0);
    // 2-draw hair
    vkCmdBindVertexBuffers(mrtCommandBuffer, 0, 1, &hairLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(mrtCommandBuffer,hairLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(mrtCommandBuffer, hairLoader.parts[0].indices.size(), 1, 0, 0, 0);

    // -------------draw back-----------------
    vkCmdBindPipeline(mrtCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , backPipeline);
    vkCmdNextSubpass(mrtCommandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindVertexBuffers(mrtCommandBuffer, 0, 1, &hairLoader.parts[0].verticesBuffer, offsets);
    vkCmdBindIndexBuffer(mrtCommandBuffer,hairLoader.parts[0].indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(mrtCommandBuffer, hairLoader.parts[0].indices.size(), 1, 0, 0, 0);

    vkCmdEndRenderPass(mrtCommandBuffer);
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,mrtCommandBuffer );
}

void DualPassRenderer::recordCommandComp() {
    std::vector<VkClearValue> clearValues(2);
    clearValues[0].color = {0.6f, 0.65f, 0.4, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    const VkFramebuffer &framebuffer = activatedSwapChainFramebuffer;
    auto [cmdBufferBeginInfo,renderpassBeginInfo ]= FnCommand::createCommandBufferBeginInfo(framebuffer,
        simplePass.pass,
        &simpleSwapchain.swapChainExtent,clearValues);
    auto result = vkBeginCommandBuffer(activatedFrameCommandBufferToSubmit, &cmdBufferBeginInfo);
    if(result!= VK_SUCCESS) throw std::runtime_error{"ERROR vkBeginCommandBuffer"};
    vkCmdBeginRenderPass(activatedFrameCommandBufferToSubmit, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS ,compPipeline);
    auto viewport = FnCommand::viewport(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    auto scissor = FnCommand::scissor(simpleSwapchain.swapChainExtent.width, simpleSwapchain.swapChainExtent.height );
    vkCmdSetViewport(activatedFrameCommandBufferToSubmit, 0, 1, &viewport);
    vkCmdSetScissor(activatedFrameCommandBufferToSubmit,0, 1, &scissor);

    vkCmdBindDescriptorSets(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, compPipelineLayout,
         0, 1, &compDescSets[currentFrame], 0, nullptr);
    vkCmdBindPipeline(activatedFrameCommandBufferToSubmit, VK_PIPELINE_BIND_POINT_GRAPHICS, compPipeline);
    vkCmdDraw(activatedFrameCommandBufferToSubmit, 3, 1, 0, 0);
    vkCmdEndRenderPass(activatedFrameCommandBufferToSubmit);
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,activatedFrameCommandBufferToSubmit );
}

void DualPassRenderer::cleanupComp() {
    const auto device = mainDevice.logicalDevice;
    UT_Fn::cleanup_descriptor_set_layout(device, compDescSetLayout);
    UT_Fn::cleanup_pipeline(device, compPipeline);
    UT_Fn::cleanup_pipeline_layout(device, compPipelineLayout);
}

void DualPassRenderer::prepareComp() {
    const auto &device =mainDevice.logicalDevice;
    std::cout << "create composition desc sets\n";
    auto binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, VK_SHADER_STAGE_FRAGMENT_BIT);
    const std::array set0_bindings = {binding0}; // only binding = 0
    const VkDescriptorSetLayoutCreateInfo ubo_createInfo = FnDescriptor::setLayoutCreateInfo(set0_bindings);
    UT_Fn::invoke_and_check("composition set=0 layout failed", vkCreateDescriptorSetLayout, device, &ubo_createInfo, nullptr, &compDescSetLayout );
    // create sets
    const std::array layouts{compDescSetLayout,compDescSetLayout};
    const auto allocInfo = FnDescriptor::setAllocateInfo(descPool, layouts);
    UT_Fn::invoke_and_check("Error create comp sets",vkAllocateDescriptorSets,device, &allocInfo, compDescSets.data() );

    // comp
    for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
        std::vector<VkWriteDescriptorSet> writeSets;
        writeSets.emplace_back(FnDescriptor::writeDescriptorSet(compDescSets[currentFrame], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &dualColorAttachment.descImageInfo));
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }

    // create pipeline layout
    const std::array deferredLayouts{compDescSetLayout};
    VkPipelineLayoutCreateInfo deferredLayout_CIO = FnPipeline::layoutCreateInfo(deferredLayouts);
    UT_Fn::invoke_and_check("ERROR create deferred pipeline layout",vkCreatePipelineLayout,device, &deferredLayout_CIO,nullptr, &compPipelineLayout );

    // create pipeline
    const auto vsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/dualpass_comp_vert.spv",  device);
    const auto fsMD = FnPipeline::createShaderModuleFromSpvFile("shaders/dualpass_comp_frag.spv",  device);
    const auto vsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);
    const auto fsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);

    auto emptyVertexInput_ST_CIO =  FnPipeline::vertexInputStateCreateInfo();
    compPso.pipelineCIO.pVertexInputState = &emptyVertexInput_ST_CIO;
    compPso.rasterizerStateCIO.cullMode = VK_CULL_MODE_FRONT_BIT;

    compPso.setRenderPass(simplePass.pass);
    compPso.setShaderStages(vsCIO,fsCIO);
    compPso.setPipelineLayout(compPipelineLayout);
    UT_GraphicsPipelinePSOs::createPipeline(device, compPso, getPipelineCache(), compPipeline);
    UT_Fn::cleanup_shader_module(device,vsMD, fsMD);
}


LLVK_NAMESPACE_END