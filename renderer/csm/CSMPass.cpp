//
// Created by liuya on 11/8/2024.
//

#include "CSMPass.h"

#include <LLVK_Descriptor.hpp>

#include "LLVK_Image.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "VulkanRenderer.h"
LLVK_NAMESPACE_BEGIN


CSMPass::CSMPass(const VulkanRenderer *renderer, const VkDescriptorPool *descPool):pRenderer(renderer),pDescriptorPool(descPool){
}

void CSMPass::prepare() {
    prepareDepthResources();
    prepareDepthRenderPass();
    prepareDescriptorSets();
    preparePipelines();
    prepareUniformBuffers();
}

void CSMPass::cleanup() {
    auto device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(depthAttachment);
    UT_Fn::cleanup_sampler(device, depthSampler);
    vkDestroyRenderPass(device, depthRenderPass, nullptr);
    vkDestroyFramebuffer(device, depthFramebuffer, nullptr);
    UT_Fn::cleanup_range_resources(uboBuffers);
    UT_Fn::cleanup_pipeline_layout(device, depthPOGeneric.pipelineLayout);
    UT_Fn::cleanup_pipeline(device, depthPOGeneric.pipeline);
    UT_Fn::cleanup_descriptor_set_layout(device, depthPOGeneric.setLayout);
}

void CSMPass::prepareDepthResources() {
    auto device = pRenderer->getMainDevice().logicalDevice;
    depthSampler = FnImage::createDepthSampler(device);
    setRequiredObjectsByRenderer(pRenderer, depthAttachment);
    depthAttachment.create2dArrayDepth32(width,width, cascade_count, depthSampler);
}

void CSMPass::prepareDepthRenderPass() {
    VkAttachmentDescription attachmentDescription{};
    attachmentDescription.format = depthAttachment.format;
    std::cout << "[[CSMPass::prepareDepthRenderPass]]:depth attachment format: " << magic_enum::enum_name(depthAttachment.format) << std::endl;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end
    VkAttachmentReference depthReference = {};
    depthReference.attachment = 0;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;													// No color attachments
    subpass.pColorAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depthReference;

    // Use subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassCreateInfo.pDependencies = dependencies.data();
    UT_Fn::invoke_and_check("Error created render pass",vkCreateRenderPass, pRenderer->getMainDevice().logicalDevice,
        &renderPassCreateInfo, nullptr, &depthRenderPass);

    const auto &device = pRenderer->getMainDevice().logicalDevice;
    // framebuffer create
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = depthRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &depthAttachment.view;
    framebufferInfo.width = width;           // FIXED shadow map size;
    framebufferInfo.height = width;         // FIXED shadow map size;
    framebufferInfo.layers = 1;
    UT_Fn::invoke_and_check("create framebuffer failed", vkCreateFramebuffer,device, &framebufferInfo, nullptr, &depthFramebuffer);

}
void CSMPass::prepareDescriptorSets() {
    // only set=0. binding =0 UBO, binding=1 albedoTex
    const auto &mainDevice = pRenderer->getMainDevice();
    const auto &device = mainDevice.logicalDevice;
    const auto &physicalDevice = mainDevice.physicalDevice;

    auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_GEOMETRY_BIT);           // ubo
    auto set0_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, VK_SHADER_STAGE_FRAGMENT_BIT); // albedo
    const std::array set0_bindings = {set0_ubo_binding0, set0_ubo_binding1};

    const VkDescriptorSetLayoutCreateInfo setLayoutCIO = FnDescriptor::setLayoutCreateInfo(set0_bindings);
    UT_Fn::invoke_and_check("Error create uboDescSetLayout set layout",vkCreateDescriptorSetLayout,device, &setLayoutCIO, nullptr, &depthPOGeneric.setLayout);

}
void CSMPass::preparePipelines() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    {
        const auto vertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/depthPass_vert.spv",  device);
        const auto geomModule = FnPipeline::createShaderModuleFromSpvFile("shaders/depthPass_geom.spv",  device);
        const auto fragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/depthPass_frag.spv",  device);
        VkPipelineShaderStageCreateInfo vertSSCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule);
        VkPipelineShaderStageCreateInfo geomSSCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT, geomModule);
        VkPipelineShaderStageCreateInfo fragSSCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule);
        depthPOGeneric.pipelinePSOs.setShaderStages(vertSSCIO, geomSSCIO, fragSSCIO);

        // layout
        const std::array sceneSetLayouts{depthPOGeneric.setLayout};
        VkPipelineLayoutCreateInfo sceneSetLayout_CIO = FnPipeline::layoutCreateInfo(sceneSetLayouts);
        UT_Fn::invoke_and_check("ERROR create scene pipeline layout",vkCreatePipelineLayout,device, &sceneSetLayout_CIO,nullptr, &depthPOGeneric.pipelineLayout );
        depthPOGeneric.pipelinePSOs.setPipelineLayout(depthPOGeneric.pipelineLayout);
        depthPOGeneric.pipelinePSOs.setRenderPass(pRenderer->getMainRenderPass());
        UT_GraphicsPipelinePSOs::createPipeline(device, depthPOGeneric.pipelinePSOs, pRenderer->getPipelineCache(), depthPOGeneric.pipeline);
        UT_Fn::cleanup_shader_module(device,vertModule, geomModule, fragModule);
    }

}


void CSMPass::prepareUniformBuffers() {
    setRequiredObjectsByRenderer(pRenderer, uboBuffers[0], uboBuffers[1]);
    for(auto &ubo : uboBuffers)
        ubo.createAndMapping(sizeof(uboData));
}



void CSMPass::recordCommandBuffer() {

}


void CSMPass::updateCascade() {

}




LLVK_NAMESPACE_END
