﻿//
// Created by lp on 2024/9/25.
//


#include <libs/magic_enum.hpp>
#include <iostream>
#include "LLVK_Utils.hpp"
#include "VulkanRenderer.h"
#include "ShadowMapPass.h"
#include "LLVK_Descriptor.hpp"
#include "Pipeline.hpp"
#include "LLVK_GeometryLoader.h"
#include "renderer/public/UT_CustomRenderer.hpp"
LLVK_NAMESPACE_BEGIN


void ShadowMapGeometryContainer::buildSet() {
	const auto &device = requiredObjects.pVulkanRenderer->getMainDevice().logicalDevice;
	const auto &pool = *requiredObjects.pPool;
	const auto &setLayout = *requiredObjects.pSetLayout;
	const auto &ubo = *requiredObjects.pUBO;
	for(auto &geo: renderableObjects) {
		auto offscreenAllocInfo = FnDescriptor::setAllocateInfo(pool,&setLayout, 1);
		UT_Fn::invoke_and_check("Error create offscreen sets",vkAllocateDescriptorSets,device, &offscreenAllocInfo,&geo.set);
	}
	// update set
	for(const auto &geo: renderableObjects) {
		const auto &set = geo.set;
		std::vector<VkWriteDescriptorSet> opacityWriteSets;
		opacityWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(set,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,0, &ubo.descBufferInfo));
		opacityWriteSets.emplace_back(FnDescriptor::writeDescriptorSet(set,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1, &geo.pTexture->descImageInfo));
		vkUpdateDescriptorSets(device,static_cast<uint32_t>(opacityWriteSets.size()),opacityWriteSets.data(),0, nullptr);
	}
}

void ShadowMapGeometryContainer::addRenderableGeometry(RenderableObject rRenderableObject) {
	renderableObjects.emplace_back(std::forward<RenderableObject>(rRenderableObject) );
}

void ShadowMapGeometryContainer::setRequiredObjects( RequiredObjects &&rRequiredObjects) {
	requiredObjects = std::forward< RequiredObjects>(rRequiredObjects);
}


ShadowMapPass::ShadowMapPass(const VulkanRenderer *renderer,
	const VkDescriptorPool *descPool,
	const VkCommandBuffer *cmd): pRenderer{renderer},pDescriptorPool{descPool}, pCommandBuffer{cmd} {
}

void ShadowMapPass::prepare() {
	const auto &device= pRenderer->getMainDevice().logicalDevice;
	shadowFramebuffer.depthSampler = FnImage::createDepthSampler(device);
	createOffscreenDepthAttachment();
	createOffscreenRenderPass();
	createOffscreenFramebuffer();
	prepareUniformBuffers();
	prepareDescriptorSets();
	preparePipelines();
}

void ShadowMapPass::cleanup() {
	const auto device = pRenderer->getMainDevice().logicalDevice;
	vkDestroyFramebuffer(device, shadowFramebuffer.framebuffer, nullptr);
	// cleanup ubo
	UT_Fn::cleanup_resources(uboBuffer);
	// cleanup framebuffer
	UT_Fn::cleanup_resources(shadowFramebuffer.depthAttachment);
	UT_Fn::cleanup_render_pass(device, shadowFramebuffer.renderPass);
	UT_Fn::cleanup_sampler(device, shadowFramebuffer.depthSampler);
	// cleanup pipeline
	UT_Fn::cleanup_pipeline(device, offscreenPipeline);
	UT_Fn::cleanup_descriptor_set_layout(device, offscreenDescriptorSetLayout);
	UT_Fn::cleanup_pipeline_layout(device, offscreenPipelineLayout);

}

void ShadowMapPass::prepareUniformBuffers() {
	setRequiredObjectsByRenderer(pRenderer, uboBuffer);
	uboBuffer.createAndMapping(sizeof(depthMVP));
	updateUniformBuffers();
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
	setRequiredObjectsByRenderer(pRenderer, shadowFramebuffer.depthAttachment);
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

    UT_Fn::invoke_and_check("Error created render pass",vkCreateRenderPass, pRenderer->getMainDevice().logicalDevice, &renderPassCreateInfo, nullptr, &shadowFramebuffer.renderPass);
}
void ShadowMapPass::createOffscreenFramebuffer() {
	const auto &device = pRenderer->getMainDevice().logicalDevice;
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
	const auto &device = pRenderer->getMainDevice().logicalDevice;
	// we only have one set.
	auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);         // ubo
	auto set0_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // maps
	const std::array offscreen_setLayout_bindings = {set0_ubo_binding0, set0_ubo_binding1};

	const VkDescriptorSetLayoutCreateInfo offscreenSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(offscreen_setLayout_bindings);
	UT_Fn::invoke_and_check("Error create offscreen descriptor set layout",
		vkCreateDescriptorSetLayout,device,
		&offscreenSetLayoutCIO,
		nullptr, &offscreenDescriptorSetLayout);

	ShadowMapGeometryContainer::RequiredObjects SMRequiredObjects{};
	SMRequiredObjects.pVulkanRenderer = pRenderer;
	SMRequiredObjects.pPool = pDescriptorPool;
	SMRequiredObjects.pUBO = &uboBuffer;
	SMRequiredObjects.pSetLayout = &offscreenDescriptorSetLayout;
	geoContainer.setRequiredObjects(std::move(SMRequiredObjects));
	geoContainer.buildSet();
}

void ShadowMapPass::preparePipelines() {
	auto device = pRenderer->getMainDevice().logicalDevice;
	auto pipelineCache=  pRenderer->getPipelineCache();
	pipelinePSOs.requiredObjects.device = device; // Required Object first
	pipelinePSOs.asDepth("shaders/sm_v2_offscreen_vert.spv", "shaders/sm_v2_offscreen_frag.spv", shadowFramebuffer.renderPass);
	// create pipeline layout
	const std::array offscreenSetLayouts{offscreenDescriptorSetLayout};
	VkPipelineLayoutCreateInfo offscreenSetLayout_CIO = FnPipeline::layoutCreateInfo(offscreenSetLayouts); // ONLY ONE SET
	UT_Fn::invoke_and_check("ERROR create offscreen pipeline layout",vkCreatePipelineLayout,device, &offscreenSetLayout_CIO,nullptr, &offscreenPipelineLayout );
	pipelinePSOs.setPipelineLayout(offscreenPipelineLayout);
	// now create our pipeline
	UT_Fn::invoke_and_check( "error create offscreen opacity pipeline" ,vkCreateGraphicsPipelines,
		device,pipelineCache, 1, &pipelinePSOs.pipelineCIO, nullptr, &offscreenPipeline);
	pipelinePSOs.cleanupShaderModule();
}
void ShadowMapPass::recordCommandBuffer() {
	VkCommandBuffer commandBuffer = *pCommandBuffer;
	VkClearValue cleaValue;
	cleaValue.depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo renderPassBeginInfo {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = shadowFramebuffer.renderPass;
	renderPassBeginInfo.framebuffer = shadowFramebuffer.framebuffer;
	renderPassBeginInfo.renderArea.extent.width = 2048;
	renderPassBeginInfo.renderArea.extent.height = 2048;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &cleaValue;

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , offscreenPipeline); // FIRST generate depth for opaque object
	VkDeviceSize offsets[1] = { 0 };
	auto viewport = FnCommand::viewport(2048,2048 );
	auto scissor = FnCommand::scissor(2048,2048);
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer,0, 1, &scissor);
	for (const auto &renderGeo: geoContainer.getRenderableObjects()) {
		const GLTFLoader::Part *gltfPartGeo = renderGeo.pGeometry;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gltfPartGeo->verticesBuffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer,gltfPartGeo->indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipelineLayout, 0, 1, &renderGeo.set, 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, gltfPartGeo->indices.size(), 1, 0, 0, 0);
	}
	vkCmdEndRenderPass(commandBuffer); // end of depth pass
}




LLVK_NAMESPACE_END
