//
// Created by liuyangping on 2025/2/26.
//

#include "SubPassRenderer.h"
#include <LLVK_Descriptor.hpp>
#include <memory>
#include "SubPassResource.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include <print>
LLVK_NAMESPACE_BEGIN
SubPassRenderer::SubPassRenderer() {
    mainCamera.mPosition = {-0.079330, 1.343584, 1.3575352};
    mainCamera.setRotation({-15.7468085, -13.0923764, -7.33684});
}
SubPassRenderer::~SubPassRenderer() {}



void SubPassRenderer::prepare() {
    usedDevice = getMainDevice().logicalDevice;
    usedPhyDevice = getMainDevice().physicalDevice;

    resourceLoader = std::make_unique<SubPassResource>();
    resourceLoader->renderer = this;
    resourceLoader->prepare();
    colorSampler = FnImage::createImageSampler(usedPhyDevice, usedDevice);

    createDescPool();
    prepareUBO();
    prepareCompLightsSSBO();
    prepareDescSets();
    preparePipelines();

}
void SubPassRenderer::cleanupObjects() {
    UT_Fn::cleanup_resources(*resourceLoader);
    UT_Fn::cleanup_sampler(usedDevice, colorSampler);
    UT_Fn::cleanup_resources(gBufferAttachments.albedo, gBufferAttachments.NRM, gBufferAttachments.V, gBufferAttachments.P);
    UT_Fn::cleanup_descriptor_pool(usedDevice, descPool);
    UT_Fn::cleanup_descriptor_set_layout(usedDevice, descSetLayouts.comp, descSetLayouts.transparent, descSetLayouts.gBuffer);
    UT_Fn::cleanup_pipeline(usedDevice,pipelines.comp, pipelines.transparent, pipelines.gBuffer);
    UT_Fn::cleanup_pipeline_layout(usedDevice, pipelineLayouts.comp, pipelineLayouts.transparent, pipelineLayouts.gBuffer);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_range_resources(compSSBOBuffers);
}

void SubPassRenderer::render() {
    // 在更新当前帧矩阵之前，保存它们作为上一帧的矩阵
    updatePreviousUBO();
    // 更新当前帧的矩阵
    updateCurrentUBO();
    // 更新UBO数据
    updateUBO();

    updateCompLightsSSBO();
    recordCommandBuffer();
    submitMainCommandBuffer();
    presentMainCommandBufferFrame();

}

void SubPassRenderer::createGBufferAttachments() {
    auto w = simpleSwapchain.swapChainExtent.width;
    auto h = simpleSwapchain.swapChainExtent.height;

    setRequiredObjectsByRenderer(this, gBufferAttachments.albedo,
        gBufferAttachments.NRM,
        gBufferAttachments.V,
        gBufferAttachments.P);

    // VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT flag is required for input attachments
    if(gBufferAttachments.albedo.isValid)
        gBufferAttachments.albedo.cleanup();
    gBufferAttachments.albedo.create(w,h , VK_FORMAT_R8G8B8A8_UNORM, colorSampler, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    if(gBufferAttachments.NRM.isValid)
        gBufferAttachments.NRM.cleanup();
    gBufferAttachments.NRM.create(w,h , VK_FORMAT_R16G16B16A16_SFLOAT, colorSampler, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    if (gBufferAttachments.V.isValid)
        gBufferAttachments.V.cleanup();
    gBufferAttachments.V.create(w,h,VK_FORMAT_R16G16B16A16_SFLOAT,colorSampler, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    if (gBufferAttachments.P.isValid)
        gBufferAttachments.P.cleanup();
    gBufferAttachments.P.create(w,h,VK_FORMAT_R16G16B16A16_SFLOAT,colorSampler, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

    gBufferAttachments.width = w;
    gBufferAttachments.height = h;
}

void SubPassRenderer::createRenderpass() {
    usedDevice = getMainDevice().logicalDevice;
    usedPhyDevice = getMainDevice().physicalDevice;
    simplePass.bindDevice = usedDevice;
    simplePass.bindPhysicalDevice = usedPhyDevice;
    createGBufferAttachments();
    std::array<VkAttachmentDescription, 6> attachments{};
    //swapchain attachment
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM; // same as swapchain format
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;            // before rendering
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;          // after rendering
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // 渲染之前stencil 干什么
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 渲染之后stencil 干什么
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // albedo
    attachments[1].format = gBufferAttachments.albedo.format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 以前再deferred哪一张，这里是VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    // NRM
    attachments[2].format = gBufferAttachments.NRM.format;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 以前再deferred哪一张，这里是VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    // V.z存depth
    attachments[3].format = gBufferAttachments.V.format;
    attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 以前再deferred哪一张，这里是VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    // P . 这里我留了一手. V得z通道我会存depth. 然后试图从这个depth推出来P
    attachments[4].format = gBufferAttachments.P.format;
    attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[4].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 以前再deferred哪一张，这里是VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    //depth
    attachments[5].format = FnImage::findDepthFormat(usedPhyDevice);
    std::cout << "[[RENDER PASS]]:createRenderpass depth format:" <<magic_enum::enum_name(attachments[1].format) <<  " idx:"<<attachments[1].format << std::endl;
    attachments[5].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[5].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[5].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[5].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[5].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[5].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[5].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // sub render pass create
    std::array<VkSubpassDescription, 3> subpassDescriptions{};
    {
        // 1:gbuffer renderpass
        std::array<VkAttachmentReference,5> outRefs{};
        outRefs[0] = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; // swapchain, outs ref must be: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        outRefs[1] = {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; // albedo
        outRefs[2] = {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; // NRM
        outRefs[3] = {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; // V
        outRefs[4] = {4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; // P
        VkAttachmentReference depthReference = { 5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }; // DS
        subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescriptions[0].colorAttachmentCount = outRefs.size();
        subpassDescriptions[0].pColorAttachments = outRefs.data();
        subpassDescriptions[0].pDepthStencilAttachment = &depthReference;
    }
    {
        // 2:comp renderpass
        VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; //0 swapchain. gbuffer data - > swapchain
        // inputs
        subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescriptions[1].colorAttachmentCount = 1; // only one out, swapchain
        subpassDescriptions[1].pColorAttachments  = &colorReference;
        // outputs
        std::array<VkAttachmentReference,4> inputsRefs{};
        inputsRefs[0] = {1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}; // albedo, input layout: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        inputsRefs[1] = {2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}; // nrm
        inputsRefs[2] = {3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}; // v
        inputsRefs[3] = {4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}; // P
        subpassDescriptions[1].inputAttachmentCount = inputsRefs.size();
        subpassDescriptions[1].pInputAttachments  = inputsRefs.data();
        VkAttachmentReference depthReference = { 5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }; // DS
        subpassDescriptions[1].pDepthStencilAttachment = &depthReference;
    }
    {
        // 3:forward renderpass
        VkAttachmentReference outRef = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }; //0 swapchain. gbuffer data - > swapchain
        subpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescriptions[2].colorAttachmentCount = 1; // only one out, swapchain
        subpassDescriptions[2].pColorAttachments  = &outRef;


        std::array<VkAttachmentReference,2> inRefs{};
        inRefs[0] = {3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}; // V
        inRefs[1] = {4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}; // P
        subpassDescriptions[2].inputAttachmentCount = inRefs.size();
        subpassDescriptions[2].pInputAttachments  = inRefs.data();
        VkAttachmentReference depthReference = { 5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }; // DS
        subpassDescriptions[2].pDepthStencilAttachment = &depthReference;
    }


    // dependency.
    std::array<VkSubpassDependency ,4> dependencies{};
    // extern -> 0  (gBuffer write)
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dependencyFlags = 0;

    // 0 -> 1 gbuffer color-attachment -> input-attachment for shader shaderead
    dependencies[1].srcSubpass = 0;  // gbuffer
    dependencies[1].dstSubpass = 1;  // comp
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // 确保上一个写入到附件完成
    dependencies[1].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;  // 这里我们要读取附件 作为input_attachment
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 确保上一个pass完成了写出attachment阶段
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;         //
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    // 1 -> 2 : comp -> forward
    dependencies[2].srcSubpass = 1;
    dependencies[2].dstSubpass = 2;
    dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;   // 确保上一个写入附件完成
    dependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;    // 我们要读取一个附件
    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 确保上一个阶段颜色写完
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    // 2 -> extern : forward -> extern
    dependencies[3].srcSubpass = 2;
    dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;   // 统一读操作
    dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // 确保上一个阶段颜色写完
    dependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

/*
    // Subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 5> dependencies;

		// This makes sure that writes to the depth image are done before we try to write to it again
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
		dependencies[0].srcAccessMask = 0;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = 0;

		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = 0;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dependencyFlags = 0;

		// This dependency transitions the input attachment from color attachment to input attachment read
		dependencies[2].srcSubpass = 0;
		dependencies[2].dstSubpass = 1;
		dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[3].srcSubpass = 1;
		dependencies[3].dstSubpass = 2;
		dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[3].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[4].srcSubpass = 2;
		dependencies[4].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[4].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[4].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
*/
    // create render pass
    VkRenderPassCreateInfo pass_CIO{};
    pass_CIO.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass_CIO.attachmentCount = attachments.size();
    pass_CIO.pAttachments = attachments.data();
    pass_CIO.subpassCount = subpassDescriptions.size();
    pass_CIO.pSubpasses = subpassDescriptions.data();
    pass_CIO.dependencyCount = dependencies.size();
    pass_CIO.pDependencies = dependencies.data();
    // create
    auto ret = vkCreateRenderPass(usedDevice, &pass_CIO, nullptr, &simplePass.pass);
    if(ret != VK_SUCCESS) throw std::runtime_error{"ERROR"};

}



void SubPassRenderer::createFramebuffers() {
    usedDevice = getMainDevice().logicalDevice;
    usedPhyDevice = getMainDevice().physicalDevice;

    // 1. MUST bind device. that for vkDestroyFrameBuffer()
    simpleFramebuffer.bindDevice = mainDevice.logicalDevice;

    // 2.first check need recreate attachments.
    // check attachment-w-h == swapchain. we only check albedo wh
    const auto [w,h] = simpleSwapchain.swapChainExtent;
    if(gBufferAttachments.width != w or gBufferAttachments.height != h) {
        createGBufferAttachments();
    }

    VkDescriptorImageInfo diffImageInfo{VK_NULL_HANDLE, gBufferAttachments.albedo.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo nrmImageInfo{VK_NULL_HANDLE, gBufferAttachments.NRM.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo vImageInfo{VK_NULL_HANDLE, gBufferAttachments.V.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkDescriptorImageInfo pImageInfo{VK_NULL_HANDLE, gBufferAttachments.P.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    // update comp descript sets
    {
        for (int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
            auto &descSet = descSets.comp[i];
            if (descSet == nullptr) break; // because we should call createFramebuffers() first. so the descSet = nullptr
            std::array writes = {
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &diffImageInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,2, &nrmImageInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,3, &vImageInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,4, &pImageInfo),
            };
            vkUpdateDescriptorSets(usedDevice,writes.size(),writes.data(),0, nullptr);
        }
    }

    // update forward descriptor sets
    {
        for (int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
            auto &descSet = descSets.transparent[i];
            if (descSet == nullptr) break; // because we should call createFramebuffers() first. so the descSet = nullptr
            std::array writes = {
                // binding =0 is UBO
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &vImageInfo), // binding=1 is V input attachment
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, &pImageInfo), // binding=2 is P input attachment
            };
            vkUpdateDescriptorSets(usedDevice,static_cast<uint32_t>(writes.size()),writes.data(),0, nullptr);
        }
    }

    // Resize framebuffer count to equal swap chain image count
    const uint32_t genFramebufferCount = simpleSwapchain.swapChainImages.size();
    simpleFramebuffer.swapChainFramebuffers.resize( genFramebufferCount );
    //std::cout << __FUNCTION__ <<simpleSwapchain.swapChainImages.size() << std::endl;
    std::println("***************frame buffer size: {},{}", w, h);
    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < genFramebufferCount; i++){
        std::array<VkImageView,6> attachments = {
            simpleSwapchain.swapChainImages[i].imageView,
            attachments[1] = this->gBufferAttachments.albedo.view,
            attachments[2] = this->gBufferAttachments.NRM.view,
            attachments[3] = this->gBufferAttachments.V.view,
            attachments[4] = this->gBufferAttachments.P.view,
            depthImageView
        };
        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = simplePass.pass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = w;
        framebufferCreateInfo.height = h;
        framebufferCreateInfo.layers = 1;

        auto result = vkCreateFramebuffer(usedDevice, &framebufferCreateInfo, nullptr, &simpleFramebuffer.swapChainFramebuffers[i]);
        if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to create a Framebuffer!");
    }

}
void SubPassRenderer::createDescPool() {
    // 1. pool
    const auto &device = mainDevice.logicalDevice;
    std::array<VkDescriptorPoolSize, 4> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 40 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 40 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 40 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 40 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 40 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    if (result!=VK_SUCCESS) throw std::runtime_error("Failed to create descriptor pool!");
}


void SubPassRenderer::prepareDescSets() {
    auto createPipelineLayout = [this](const VkDescriptorSetLayout &descLayout, VkPipelineLayout &pipeLayout ) {
        const std::array sceneSetLayouts{descLayout};
        VkPipelineLayoutCreateInfo sceneSetLayout_CIO = FnPipeline::layoutCreateInfo(sceneSetLayouts);
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(subpass::xform);
        sceneSetLayout_CIO.pPushConstantRanges = &pushConstantRange;
        sceneSetLayout_CIO.pushConstantRangeCount =1 ;
        UT_Fn::invoke_and_check("ERROR create scene pipeline layout",vkCreatePipelineLayout,usedDevice, &sceneSetLayout_CIO,nullptr, &pipeLayout );
    };
    auto allocateSets = [this](const VkDescriptorSetLayout &layout, subpass::FramedSet &sets) {
        const std::array<VkDescriptorSetLayout,2> tmpSetLayouts({layout, layout});
        auto setAllocInfo = FnDescriptor::setAllocateInfo(descPool,tmpSetLayouts);
        UT_Fn::invoke_and_check("Error create gBufferBook", vkAllocateDescriptorSets, usedDevice, &setAllocInfo, sets.data());
    };

    // GBUFFER oneset : 0:ubo 1:diff 2:nrm
    {
        using desc_types = MetaDesc::desc_types_t<MetaDesc::UBO,MetaDesc::CIS, MetaDesc::CIS>;
        using binding_positions_t = MetaDesc::desc_binding_position_t<0,1,2>;
        using binding_usages_t =  MetaDesc::desc_binding_usage_t<VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT,VK_SHADER_STAGE_FRAGMENT_BIT>;
        constexpr std::array setLayoutBindings= MetaDesc::generateSetLayoutBindings<desc_types, binding_positions_t, binding_usages_t>();
        //setLayout
        const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(setLayoutBindings);
        UT_Fn::invoke_and_check("Error create desc set layout",vkCreateDescriptorSetLayout, usedDevice, &setLayoutCIO, nullptr, &descSetLayouts.gBuffer);
        // sets
        const std::array<VkDescriptorSetLayout,2> tmpSetLayouts({descSetLayouts.gBuffer,descSetLayouts.gBuffer});
        auto setAllocInfo = FnDescriptor::setAllocateInfo(descPool,tmpSetLayouts);
        UT_Fn::invoke_and_check("Error create gBufferBook", vkAllocateDescriptorSets, usedDevice, &setAllocInfo, descSets.gBufferBook.data());
        UT_Fn::invoke_and_check("Error create gBufferTelevision", vkAllocateDescriptorSets, usedDevice, &setAllocInfo, descSets.gBufferTelevision.data());
        UT_Fn::invoke_and_check("Error create gBufferWall", vkAllocateDescriptorSets, usedDevice, &setAllocInfo, descSets.gBufferWall.data());
        UT_Fn::invoke_and_check("Error create gBufferTable", vkAllocateDescriptorSets, usedDevice, &setAllocInfo, descSets.gBufferWoodenTable.data());
        // write sets
        auto fnGBufferWriteSets = [](const VkDevice &device, const subpass::FramedUBO &uboBuffers, const auto &diff, const auto &nrm,subpass::FramedSet &sets) {
            for (int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
                std::array<VkWriteDescriptorSet, 3> writes = {
                    FnDescriptor::writeDescriptorSet(sets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),
                    FnDescriptor::writeDescriptorSet(sets[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &diff.descImageInfo),
                    FnDescriptor::writeDescriptorSet(sets[i],VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2, &nrm.descImageInfo),
                };
                // do not use lambda capture this, because this->uboBuffer this->usedDevice in compile time is nullptr
                vkUpdateDescriptorSets(device,static_cast<uint32_t>(writes.size()),writes.data(),0, nullptr);
            }
        };
        fnGBufferWriteSets(usedDevice, uboBuffers,resourceLoader->book.diff, resourceLoader->book.nrm,descSets.gBufferBook);
        fnGBufferWriteSets(usedDevice, uboBuffers,resourceLoader->television.diff, resourceLoader->television.nrm,descSets.gBufferTelevision);
        fnGBufferWriteSets(usedDevice, uboBuffers,resourceLoader->wall.diff, resourceLoader->wall.nrm,descSets.gBufferWall);
        fnGBufferWriteSets(usedDevice, uboBuffers, resourceLoader->table.diff, resourceLoader->table.nrm,descSets.gBufferWoodenTable);
    }

    // COMP
    {
        // albedo/nrm/v/p/lightPosition
        using desc_types = MetaDesc::desc_types_t<
            MetaDesc::UBO,              // GLOBAL UBO
            MetaDesc::INPUT_ATTACHMENT, // albedo
            MetaDesc::INPUT_ATTACHMENT, // NRM
            MetaDesc::INPUT_ATTACHMENT, // V
            MetaDesc::INPUT_ATTACHMENT, // P
            MetaDesc::SSBO>;            // Lights
        using binding_positions_t = MetaDesc::desc_binding_position_t<0,1,2,3,4,5>;
        using binding_usages_t =  MetaDesc::desc_binding_usage_t<
            VK_SHADER_STAGE_FRAGMENT_BIT,  // GLOBAL UBO
            VK_SHADER_STAGE_FRAGMENT_BIT,  // alebdo
            VK_SHADER_STAGE_FRAGMENT_BIT,  // NRM
            VK_SHADER_STAGE_FRAGMENT_BIT,  // V
            VK_SHADER_STAGE_FRAGMENT_BIT,  // P
            VK_SHADER_STAGE_FRAGMENT_BIT>; // Lights
        constexpr std::array setLayoutBindings= MetaDesc::generateSetLayoutBindings<desc_types, binding_positions_t, binding_usages_t>();
        //setLayout
        const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(setLayoutBindings);
        UT_Fn::invoke_and_check("Error create desc set layout",vkCreateDescriptorSetLayout, usedDevice, &setLayoutCIO, nullptr, &descSetLayouts.comp);
        //sets
        allocateSets(descSetLayouts.comp, descSets.comp);
        for (int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
            auto &descSet = descSets.comp[i];
            VkDescriptorImageInfo diffImageInfo{VK_NULL_HANDLE, gBufferAttachments.albedo.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo nrmImageInfo{VK_NULL_HANDLE, gBufferAttachments.NRM.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo vImageInfo{VK_NULL_HANDLE, gBufferAttachments.V.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
            VkDescriptorImageInfo pImageInfo{VK_NULL_HANDLE, gBufferAttachments.P.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

            std::array writes = {
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &diffImageInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,2, &nrmImageInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,3, &vImageInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,4, &pImageInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,5, &compSSBOBuffers[i].descBufferInfo),
            };
            vkUpdateDescriptorSets(usedDevice,writes.size(),writes.data(),0, nullptr);
        }
    }

    // forward  TRANSPARENT
    {
        using desc_types = MetaDesc::desc_types_t<MetaDesc::UBO, MetaDesc::INPUT_ATTACHMENT,MetaDesc::INPUT_ATTACHMENT>;
        using binding_positions_t = MetaDesc::desc_binding_position_t<0,1,2>;
        using binding_usages_t =  MetaDesc::desc_binding_usage_t<VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT>;
        constexpr std::array setLayoutBindings= MetaDesc::generateSetLayoutBindings<desc_types, binding_positions_t, binding_usages_t>();
        //setLayout

        const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(setLayoutBindings);
        UT_Fn::invoke_and_check("Error create desc set layout",vkCreateDescriptorSetLayout, usedDevice, &setLayoutCIO, nullptr, &descSetLayouts.transparent);
        allocateSets(descSetLayouts.transparent, descSets.transparent);
        VkDescriptorImageInfo vImageInfo{VK_NULL_HANDLE, gBufferAttachments.V.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo pImageInfo{VK_NULL_HANDLE, gBufferAttachments.P.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        for (int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
            auto &descSet = descSets.transparent[i];
            std::array writes = {
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &vImageInfo),
                FnDescriptor::writeDescriptorSet(descSet,VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, &pImageInfo),
            };
            vkUpdateDescriptorSets(usedDevice,static_cast<uint32_t>(writes.size()),writes.data(),0, nullptr);
        }
    }

    // pipeline layout
    createPipelineLayout(descSetLayouts.gBuffer, pipelineLayouts.gBuffer);
    createPipelineLayout(descSetLayouts.comp, pipelineLayouts.comp);
    createPipelineLayout(descSetLayouts.transparent, pipelineLayouts.transparent);

}

void SubPassRenderer::preparePipelines() {
    // vertex input
    std::array<VkVertexInputAttributeDescription,4> attribsDesc{};
    attribsDesc[0] = { 0,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0, P)};
    attribsDesc[1] = { 1,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0, N)};
    attribsDesc[2] = { 2,0,VK_FORMAT_R32G32B32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0, T)};
    attribsDesc[3] = { 3,0,VK_FORMAT_R32G32_SFLOAT , offsetof(VTXFmt_P_N_T_UV0, uv0) };
    VkVertexInputBindingDescription vertexBinding{0, sizeof(VTXFmt_P_N_T_UV0), VK_VERTEX_INPUT_RATE_VERTEX};
    std::array bindingsDesc{vertexBinding};

    // GBUFFER
    {
        const auto vs = "shaders/subpass_gbuffer_vert.spv";
        const auto fs = "shaders/subpass_gbuffer_frag.spv";
        const auto vsMD = FnPipeline::createShaderModuleFromSpvFile(vs,  usedDevice);    //shader modules
        const auto fsMD = FnPipeline::createShaderModuleFromSpvFile(fs,  usedDevice);
        VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
        VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
        psoGBuffer.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
        psoGBuffer.setPipelineLayout(pipelineLayouts.gBuffer);
        psoGBuffer.setRenderPass(this->getMainRenderPass());
        psoGBuffer.pipelineCIO.subpass = 0;
        // blend
        constexpr std::array colorBlendAttachments{
            FnPipeline::simpleOpaqueColorBlendAttachmentState(), // swapchain
            FnPipeline::simpleOpaqueColorBlendAttachmentState(), // albedo
            FnPipeline::simpleOpaqueColorBlendAttachmentState(), // NRM
            FnPipeline::simpleOpaqueColorBlendAttachmentState(), // V
            FnPipeline::simpleOpaqueColorBlendAttachmentState(), // P
        };
        psoGBuffer.colorBlendStateCIO.attachmentCount = colorBlendAttachments.size();
        psoGBuffer.colorBlendStateCIO.pAttachments = colorBlendAttachments.data();
        psoGBuffer.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);
        UT_GraphicsPipelinePSOs::createPipeline(usedDevice, psoGBuffer, getPipelineCache(), pipelines.gBuffer);
        UT_Fn::cleanup_shader_module(usedDevice,vsMD,fsMD);
    }
    // COMP
    {
        const auto vs = "shaders/subpass_comp_vert.spv";
        const auto fs = "shaders/subpass_comp_frag.spv";
        const auto vsMD = FnPipeline::createShaderModuleFromSpvFile(vs,  usedDevice);    //shader modules
        const auto fsMD = FnPipeline::createShaderModuleFromSpvFile(fs,  usedDevice);
        VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
        VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
        psoComp.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
        psoComp.setPipelineLayout(pipelineLayouts.comp);
        psoComp.setRenderPass(this->getMainRenderPass());
        psoComp.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
        psoComp.depthStencilStateCIO.depthWriteEnable = VK_FALSE; // 不写入深度
        psoComp.depthStencilStateCIO.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        psoComp.pipelineCIO.subpass = 1;
        // empty vertex input
        psoComp.vertexInputStageCIO = VkPipelineVertexInputStateCreateInfo{};
        psoComp.vertexInputStageCIO.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        UT_GraphicsPipelinePSOs::createPipeline(usedDevice, psoComp, getPipelineCache(), pipelines.comp);
        UT_Fn::cleanup_shader_module(usedDevice,vsMD,fsMD);
    }

    // TRANSPARENT
    {
        const auto vs = "shaders/subpass_forward_vert.spv";
        const auto fs = "shaders/subpass_forward_frag.spv";
        const auto vsMD = FnPipeline::createShaderModuleFromSpvFile(vs,  usedDevice);    //shader modules
        const auto fsMD = FnPipeline::createShaderModuleFromSpvFile(fs,  usedDevice);
        VkPipelineShaderStageCreateInfo vsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vsMD);    //shader stages
        VkPipelineShaderStageCreateInfo fsMD_ssCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fsMD);
        psoTransparent.setShaderStages(vsMD_ssCIO, fsMD_ssCIO);
        psoTransparent.setPipelineLayout(pipelineLayouts.transparent);
        psoTransparent.setRenderPass(this->getMainRenderPass());
        psoTransparent.pipelineCIO.subpass = 2;
        psoTransparent.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(bindingsDesc, attribsDesc);
        VkPipelineColorBlendAttachmentState blendState = FnPipeline::simpleColorBlendAttachmentState();
        const auto blend = FnPipeline::simpleColorBlendAttachmentState();
        psoTransparent.colorBlendStateCIO.attachmentCount = 1;
        psoTransparent.colorBlendStateCIO.pAttachments = &blend;
        psoTransparent.depthStencilStateCIO.depthWriteEnable = VK_FALSE;
        psoTransparent.depthStencilStateCIO.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;// ONLY COMPARE
        UT_GraphicsPipelinePSOs::createPipeline(usedDevice, psoTransparent, getPipelineCache(), pipelines.transparent);
        UT_Fn::cleanup_shader_module(usedDevice,vsMD,fsMD);
    }

}

void SubPassRenderer::prepareUBO() {
    setRequiredObjectsByRenderer(this, uboBuffers[0], uboBuffers[1]);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(subpass::GlobalUBO));
}
void SubPassRenderer::prepareCompLightsSSBO() {
    setRequiredObjectsByRenderer(this, compSSBOBuffers[0], compSSBOBuffers[1]);
    lights.emplace_back(Light{
        {7.7405f, 3.71551f, 2.52443f,1 },{8,8,8}, 20
    });

    lights.emplace_back(Light{
            {-3.2912, 0.9594, 1.8390,1 },{0.583, 0.7919, 1.0}, 10
    });

    VkDeviceSize bufferSize = sizeof(Light) * lights.size();
    for (auto &ssbo : compSSBOBuffers){
        ssbo.createAndMapping(bufferSize);
        memcpy(ssbo.mapped, lights.data(), sizeof(Light) * lights.size() );
    }

}
void SubPassRenderer::updateCompLightsSSBO() {

}


void SubPassRenderer::updatePreviousUBO() {
    // 将当前帧的矩阵保存为上一帧的矩阵
    globalUBO.preProj = globalUBO.proj;
    globalUBO.preView = globalUBO.view;
}


void SubPassRenderer::updateCurrentUBO() {
    auto [width, height] =  getSwapChainExtent();
    auto &&mainCamera = getMainCamera();
    const auto frame = getCurrentFlightFrame();
    mainCamera.mAspect = static_cast<float>(width) / static_cast<float>(height);
    globalUBO.proj = mainCamera.projection();
    globalUBO.proj[1][1] *= -1;
    globalUBO.view = mainCamera.view();
    globalUBO.cameraPosition = glm::vec4{mainCamera.position(), 1.0};
}


void SubPassRenderer::updateUBO() {
    memcpy(uboBuffers[getCurrentFlightFrame()].mapped, &globalUBO, sizeof(subpass::GlobalUBO));
}


void SubPassRenderer::recordCommandBuffer() {

    const auto &cmdBuf = getMainCommandBuffer();
    auto cmdBeginInfo = FnCommand::commandBufferBeginInfo();
    auto [width, height] = getSwapChainExtent();
    VkDeviceSize offsets[1] = { 0 };
    auto viewport = FnCommand::viewport(width,height );
    auto scissor = FnCommand::scissor(width,height);
    // clear
    VkClearValue clearValues[6];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[4].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    clearValues[5].depthStencil = { 1.0f, 0 };
    // geo render
    auto renderGeo = [&cmdBuf, &offsets](auto &geo) {
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, &geo.verticesBuffer, offsets);
        vkCmdBindIndexBuffer(cmdBuf,geo.indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuf, geo.indices.size(), 1, 0, 0, 0);
    };


    const auto renderPassBeginInfo= FnCommand::renderPassBeginInfo(getMainFramebuffer(), simplePass.pass, getSwapChainExtent(), clearValues);
    UT_Fn::invoke_and_check("begin dual pass command", vkBeginCommandBuffer, cmdBuf, &cmdBeginInfo);
    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);
    // -------------subpass 0
    DebugV2::CommandLabel::cmdBeginLabel(cmdBuf, "gbuffer-book-television-table-wall", {1,0,0,1});
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.gBuffer);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.gBuffer, 0, 1, &descSets.gBufferBook[currentFlightFrame], 0 , nullptr);
    const auto &book = resourceLoader->book.geoLoader.parts[0];
    vkCmdPushConstants(cmdBuf, pipelineLayouts.gBuffer, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(subpass::xform), &resourceLoader->book.xform);
    renderGeo(book);

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.gBuffer, 0, 1, &descSets.gBufferTelevision[currentFlightFrame], 0 , nullptr);
    const auto &television = resourceLoader->television.geoLoader.parts[0];
    vkCmdPushConstants(cmdBuf, pipelineLayouts.gBuffer, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(subpass::xform),&resourceLoader->television.xform);
    renderGeo(television);

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.gBuffer, 0, 1, &descSets.gBufferWoodenTable[currentFlightFrame], 0 , nullptr);
    const auto &table = resourceLoader->table.geoLoader.parts[0];
    vkCmdPushConstants(cmdBuf, pipelineLayouts.gBuffer, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(subpass::xform),&resourceLoader->table.xform);
    renderGeo(table);

    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.gBuffer, 0, 1, &descSets.gBufferWall[currentFlightFrame], 0 , nullptr);
    const auto &wall = resourceLoader->wall.geoLoader.parts[0];
    vkCmdPushConstants(cmdBuf, pipelineLayouts.gBuffer, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(subpass::xform),&resourceLoader->wall.xform);
    renderGeo(wall);
    DebugV2::CommandLabel::cmdEndLabel(cmdBuf);

    // --------------subpass 1
    DebugV2::CommandLabel::cmdBeginLabel(cmdBuf, "comp", {0,1,0,1});
    vkCmdNextSubpass(cmdBuf, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.comp);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.comp, 0, 1, &descSets.comp[currentFlightFrame], 0 , nullptr);
    vkCmdDraw(cmdBuf, 3, 1, 0, 0); // 花一个三角形，做comp.
    DebugV2::CommandLabel::cmdEndLabel(cmdBuf);


    // --------------subpass 2
    DebugV2::CommandLabel::cmdBeginLabel(cmdBuf, "foward", {0,0,1,1});
    vkCmdNextSubpass(cmdBuf, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.transparent);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.transparent, 0, 1, &descSets.transparent[currentFlightFrame], 0 , nullptr);
    vkCmdPushConstants(cmdBuf, pipelineLayouts.gBuffer, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(subpass::xform),&resourceLoader->bottle.xform);
    auto transGeo = resourceLoader->bottle.geoLoader.parts[0];
    renderGeo(transGeo);
    DebugV2::CommandLabel::cmdEndLabel(cmdBuf);


    vkCmdEndRenderPass(cmdBuf);
    UT_Fn::invoke_and_check("failed to record command buffer!",vkEndCommandBuffer,cmdBuf );
}






LLVK_NAMESPACE_END