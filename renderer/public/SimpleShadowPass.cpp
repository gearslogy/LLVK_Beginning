//
// Created by liuya on 4/25/2025.
//

#include "SimpleShadowPass.h"
#include "UT_CustomRenderer.hpp"
#include "VulkanValidation.h"
#include "VulkanRenderer.h"

LLVK_NAMESPACE_BEGIN
SimpleShadowPass::SimpleShadowPass(const VulkanRenderer *renderer):pRenderer(renderer) {
	const auto &device = pRenderer->getMainDevice().logicalDevice;
	// 1. create attachment
	setRequiredObjectsByRenderer(pRenderer, shadowFramebuffer.depthAttachment);
	shadowFramebuffer.depthSampler = FnImage::createDepthSampler(device);
	shadowFramebuffer.depthAttachment.createDepth32(size, size, shadowFramebuffer.depthSampler);
}
SimpleShadowPass::~SimpleShadowPass()  = default;

void SimpleShadowPass::cleanup() {
	std::cout << "--------------clean up simple shadow pass\n";
	const auto device = pRenderer->getMainDevice().logicalDevice;
	vkDestroyFramebuffer(device, shadowFramebuffer.framebuffer, nullptr);
	// cleanup ubo
	UT_Fn::cleanup_range_resources(uboBuffers);
	// cleanup framebuffer
	UT_Fn::cleanup_resources(shadowFramebuffer.depthAttachment);
	UT_Fn::cleanup_render_pass(device, shadowFramebuffer.renderPass);
	UT_Fn::cleanup_sampler(device, shadowFramebuffer.depthSampler);
	// cleanup pipeline
	UT_Fn::cleanup_pipeline(device, offscreenPipeline);
}



void SimpleShadowPass::prepareFramebuffer() {
	const auto &device = pRenderer->getMainDevice().logicalDevice;
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = shadowFramebuffer.renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = &shadowFramebuffer.depthAttachment.view;
	framebufferInfo.width = size;           // FIXED shadow map size;
	framebufferInfo.height = size;         // FIXED shadow map size;
	framebufferInfo.layers = 1;
	UT_Fn::invoke_and_check("create framebuffer failed", vkCreateFramebuffer,device, &framebufferInfo, nullptr, &shadowFramebuffer.framebuffer);
}



void SimpleShadowPass::prepareRenderPass() {
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

/*
void SimpleShadowPass::prepareDescSets() {
	const auto &device = pRenderer->getMainDevice().logicalDevice;
	assert(pDescImageInfo==nullptr);
	// 1. create setlayout
	auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);         // ubo
	auto set0_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);         // image.a to drop fragment
	const std::array offscreen_setLayout_bindings = {set0_ubo_binding0, set0_ubo_binding1};

	const VkDescriptorSetLayoutCreateInfo offscreenSetLayoutCIO = FnDescriptor::setLayoutCreateInfo(offscreen_setLayout_bindings);
	UT_Fn::invoke_and_check("Error create offscreen descriptor set layout",
		vkCreateDescriptorSetLayout,device,
		&offscreenSetLayoutCIO,
		nullptr, &offscreenDescriptorSetLayout);
	// 2. create sets
	const std::array<VkDescriptorSetLayout,2> tmpSetLayouts({offscreenDescriptorSetLayout,offscreenDescriptorSetLayout});
	auto setAllocInfo = FnDescriptor::setAllocateInfo(*pDescPool,tmpSetLayouts);
	UT_Fn::invoke_and_check("Error create shadow sets", vkAllocateDescriptorSets, device, &setAllocInfo, sets.data());
	// 3. write update sets
	for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
		std::array writeSets{
			FnDescriptor::writeDescriptorSet(sets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uboBuffers[i].descBufferInfo),
			FnDescriptor::writeDescriptorSet(sets[i],VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, pDescImageInfo)
		};
		vkUpdateDescriptorSets(device,static_cast<uint32_t>(writeSets.size()),writeSets.data(),0, nullptr);
	}
	// 4. create pipeline layout
	const std::array offscreenSetLayouts{offscreenDescriptorSetLayout};
	VkPipelineLayoutCreateInfo pipelineLayoutCIO = FnPipeline::layoutCreateInfo(offscreenSetLayouts); // ONLY ONE SET
	UT_Fn::invoke_and_check("ERROR create offscreen pipeline layout",vkCreatePipelineLayout,device, &pipelineLayoutCIO,nullptr, &offscreenPipelineLayout );
}*/


void SimpleShadowPass::preparePipeline(const VkPipelineLayout &pl) {
	pipelineLayout = pl;
	const auto &device = pRenderer->getMainDevice().logicalDevice;
	// we only have one set.
	auto set0_ubo_binding0 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, VK_SHADER_STAGE_VERTEX_BIT);         // ubo
	auto set0_ubo_binding1 = FnDescriptor::setLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT); // maps
	const std::array offscreen_setLayout_bindings = {set0_ubo_binding0, set0_ubo_binding1};

	auto pipelineCache=  pRenderer->getPipelineCache();
	pipelinePSOs.requiredObjects.device = device; // Required Object first
	pipelinePSOs.asDepth("shaders/GSM_vert.spv", "shaders/GSM_frag.spv", shadowFramebuffer.renderPass);
	pipelinePSOs.setPipelineLayout(pl);
	pipelinePSOs.vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(HLP::VTXAttrib::VTXFmt_P_N_T_UV0_BindingsDesc,
                                                                              HLP::VTXAttrib::VTXFmt_P_N_T_UV0_AttribsDesc);
	// now create our pipeline
	UT_Fn::invoke_and_check( "error create offscreen opacity pipeline" ,vkCreateGraphicsPipelines,
		device,pipelineCache, 1, &pipelinePSOs.pipelineCIO, nullptr, &offscreenPipeline);
	pipelinePSOs.cleanupShaderModule();
}


void SimpleShadowPass::prepareUBO() {
	setRequiredObjectsByRenderer(pRenderer, uboBuffers);
	for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++)
		uboBuffers[i].createAndMapping(sizeof(depthMVP));
    updateUBO();
}


void SimpleShadowPass::updateUBO() {
	auto depthProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, near, far);
	const auto depthViewMatrix = glm::lookAt(keyLightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
	constexpr auto depthModelMatrix = glm::mat4(1.0f);
	depthProjectionMatrix[1][1] *= -1.0f;
	depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
	for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++) {
		memcpy(uboBuffers[i].mapped, &depthMVP, sizeof(depthMVP));
	}
}

void SimpleShadowPass::recordCommandBuffer() {
	VkCommandBuffer cmdBuf = pRenderer->getMainCommandBuffer();
	VkClearValue cleaValue;
	cleaValue.depthStencil = { 1.0f, 0 };
	VkRenderPassBeginInfo renderPassBeginInfo {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = shadowFramebuffer.renderPass;
	renderPassBeginInfo.framebuffer = shadowFramebuffer.framebuffer;
	renderPassBeginInfo.renderArea.extent.width = size;
	renderPassBeginInfo.renderArea.extent.height = size;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &cleaValue;

	vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	DebugV2::CommandLabel::cmdBeginLabel(cmdBuf, "depth-rendering-pass", {1,1,1,1});
	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS , offscreenPipeline); // FIRST generate depth for opaque object
	VkDeviceSize offsets[1] = { 0 };
	auto viewport = FnCommand::viewport(2048,2048 );
	auto scissor = FnCommand::scissor(2048,2048);
	vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuf,0, 1, &scissor);

	drawObjects();

	DebugV2::CommandLabel::cmdEndLabel(cmdBuf);
	vkCmdEndRenderPass(cmdBuf); // end of depth pass

}


LLVK_NAMESPACE_END

