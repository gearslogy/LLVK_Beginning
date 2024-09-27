//
// Created by lp on 2024/9/25.
//


#include <libs/magic_enum.hpp>
#include <iostream>
#include "Utils.h"
#include "VulkanRenderer.h"
#include "ShadowMapPass.h"
#include "LLVK_Descriptor.hpp"
LLVK_NAMESPACE_BEGIN

void ShadowMapGeometry::cleanup() {

}
void ShadowMapGeometry::render() {

}


ShadowMapPass::ShadowMapPass(VulkanRenderer *pRenderer):renderer{pRenderer} {	}

void ShadowMapPass::prepare() {
	const auto &device= renderer->getMainDevice().logicalDevice;
	shadowFramebuffer.depthSampler = FnImage::createDepthSampler(device);
}

void ShadowMapPass::cleanup() {
	auto device = renderer->getMainDevice().logicalDevice;
	vkDestroyFramebuffer(device, shadowFramebuffer.framebuffer, nullptr);
	UT_Fn::cleanup_resources(shadowFramebuffer.depthAttachment);
	UT_Fn::cleanup_render_pass(device, shadowFramebuffer.renderPass);
	UT_Fn::cleanup_sampler(device, shadowFramebuffer.depthSampler);
	// cleanup offscreen
	UT_Fn::cleanup_pipeline(device, offscreenPipeline);
	UT_Fn::cleanup_descriptor_set_layout(device, offscreenDescriptorSetLayout);
	UT_Fn::cleanup_pipeline_layout(device, offscreenPipelineLayout);
}

void ShadowMapPass::prepareUniformBuffers() {
	setRequiredObjects(renderer, uboBuffer);
	uboBuffer.createAndMapping(sizeof(depthMVP));
}

void ShadowMapPass::updateUniformBuffers() {
	auto depthProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, near, far);
	const auto depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
	constexpr auto depthModelMatrix = glm::mat4(1.0f);
	depthProjectionMatrix[1][1] *= -1.0f;
	depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
	memcpy(uboBuffer.mapped, &depthMVP, sizeof(depthMVP));
}


void ShadowMapPass::createOffscreenDepthAttachment() {
	setRequiredObjects(renderer, shadowFramebuffer.depthAttachment);
	shadowFramebuffer.depthAttachment.createDepth32(depth_width, depth_height, shadowFramebuffer.depthSampler);
	//shadowFramebuffer.depthAttachment.create(shadowFramebuffer.width, shadowFramebuffer.height,VK_FORMAT_D32_SFLOAT_S8_UINT, colorSampler, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}


void ShadowMapPass::createOffscreenRenderPass() {
    VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = shadowFramebuffer.depthAttachment.format;
	std::cout << "[[ShadowMapPass::createOffscreenRenderPass]]:depth attachment format: " << magic_enum::enum_name(shadowFramebuffer.depthAttachment.format) << std::endl;
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
	subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;


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

    UT_Fn::invoke_and_check("Error created render pass",vkCreateRenderPass, renderer->getMainDevice().logicalDevice, &renderPassCreateInfo, nullptr, &shadowFramebuffer.renderPass);
}
void ShadowMapPass::createOffscreenFramebuffer() {
	const auto &device = renderer->getMainDevice().logicalDevice;
	// framebuffer create
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = shadowFramebuffer.renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &shadowFramebuffer.depthAttachment.view;
	framebufferInfo.width = depth_width;           // FIXED shadow map size;
	framebufferInfo.height = depth_height;         // FIXED shadow map size;
	framebufferInfo.layers = 1;
	UT_Fn::invoke_and_check("create framebuffer failed", vkCreateFramebuffer,device, &framebufferInfo, nullptr, &shadowFramebuffer.framebuffer);
}

void ShadowMapPass::prepareDescriptorSets() {
	const auto &device = renderer->getMainDevice().logicalDevice;
	auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);         // ubo
	auto set0_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // opacity map

	const std::array offscreen_setLayout_bindings = {set0_ubo_binding0, set0_ubo_binding1};
	const VkDescriptorSetLayoutCreateInfo offscreenSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(offscreen_setLayout_bindings);
	UT_Fn::invoke_and_check("Error create offscreen descriptor set layout",vkCreateDescriptorSetLayout,device, &offscreenSetLayoutCIO, nullptr, &offscreenDescriptorSetLayout);
	// create set
	std::array offscreen_setLayouts = {offscreenDescriptorSetLayout}; // only one set
	auto offscreenAllocInfo = FnDescriptor::setAllocateInfo(descPool,offscreen_setLayouts);
	UT_Fn::invoke_and_check("Error create offscreen sets",vkAllocateDescriptorSets,device, &offscreenAllocInfo,&offscreenSets.opacity );
	UT_Fn::invoke_and_check("Error create offscreen sets",vkAllocateDescriptorSets,device, &offscreenAllocInfo,&offscreenSets.opaque );


}

void ShadowMapPass::preparePipelines() {
}


LLVK_NAMESPACE_END
