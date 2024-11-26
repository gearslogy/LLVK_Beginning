//
// Created by liuya on 11/8/2024.
//

#include "CSMRenderer.h"

#include <LLVK_Descriptor.hpp>
#include <LLVK_RenderPass.hpp>

#include "CSMPass.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "LLVK_RenderPass.hpp"
LLVK_NAMESPACE_BEGIN
void CSMRenderer::ResourceManager::loading() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    const auto &phyDevice = pRenderer->getMainDevice().physicalDevice;
    setRequiredObjectsByRenderer(pRenderer, geos.geometryBufferManager);
    setRequiredObjectsByRenderer(pRenderer, textures.d_tex_29, textures.d_tex_35,
        textures.d_tex_39,textures.d_tex_36, textures.d_ground);
    geos.ground.load("content/scene/csm/resources/gpu_models/ground.gltf");
    geos.geo_29.load("content/scene/csm/resources/gpu_models/29_WatchTower.gltf");
    geos.geo_35.load("content/scene/csm/resources/gpu_models/35_MedBuilding.gltf");
    geos.geo_36.load("content/scene/csm/resources/gpu_models/36_MedBuilding.gltf");
    geos.geo_36.load("content/scene/csm/resources/gpu_models/39_MedBuilding.gltf");
    colorSampler =  FnImage::createImageSampler(phyDevice, device);
    textures.d_ground.create("content/scene/csm/resources/gpu_textures/ground_gpu_D.ktx2",colorSampler);
    textures.d_tex_29.create("content/scene/csm/resources/gpu_textures/29_WatchTower_gpu_D.ktx2",colorSampler);
    textures.d_tex_35.create("content/scene/csm/resources/gpu_textures/35_MedBuilding_gpu_D.ktx2",colorSampler);
    textures.d_tex_36.create("content/scene/csm/resources/gpu_textures/36_MedBuilding_gpu_D.ktx2",colorSampler);
    textures.d_tex_39.create("content/scene/csm/resources/gpu_textures/39_MedBuilding_gpu_D.ktx2",colorSampler);
}

CSMRenderer::ResourceManager::~ResourceManager() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(geos.geometryBufferManager,textures.d_ground, textures.d_tex_29,
     textures.d_tex_35, textures.d_tex_36,textures.d_tex_39);
    UT_Fn::cleanup_sampler(device, colorSampler);

}

void CSMRenderer::prepare() {
    resourceManager.pRenderer = this;
    resourceManager.loading();
}
void CSMRenderer::render() {

}
void CSMRenderer::cleanupObjects() {
    const auto &device = getMainDevice().logicalDevice;
    UT_Fn::cleanup_sampler(device, depthSampler);
    depthAttachment.cleanup();
    UT_Fn::cleanup_render_pass(device, depthRenderPass);
    UT_Fn::cleanup_framebuffer(device,depthFramebuffer);

}
void CSMRenderer::prepareOffscreenDepth() {
    const auto &device = getMainDevice().logicalDevice;
    depthSampler = FnImage::createDepthSampler(device);
    setRequiredObjectsByRenderer(this, depthAttachment);
    // render target
    depthAttachment.create2dArrayDepth32(2048,2048, CASCADE_COUNT,depthSampler);
    // render pass
    prepareDepthRenderPass();
    prepareFramebuffer();
}
void CSMRenderer::prepareDepthRenderPass() {
    auto depthATM = FnRenderPass::depthAttachmentDescription(VK_FORMAT_R32_SFLOAT);
    auto depthATM_Ref = FnRenderPass::attachmentReference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // before renderpass
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;    // before stage
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;   // after stage
    dependency.srcAccessMask = 0; // before access 首次写入，不需要等待之前的访问
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ; // after access
    auto subpassDesc = FnRenderPass::subpassDescription();
    subpassDesc.colorAttachmentCount = 0;
    subpassDesc.pDepthStencilAttachment = &depthATM_Ref;

    const std::array attachments = { depthATM};
    auto renderPassCIO = FnRenderPass::renderPassCreateInfo(attachments);
    renderPassCIO.subpassCount = 1;
    renderPassCIO.pSubpasses = &subpassDesc;
    renderPassCIO.dependencyCount = 1;
    renderPassCIO.pDependencies = &dependency;
    const auto device = getMainDevice().logicalDevice;
    const auto ret = vkCreateRenderPass(device, &renderPassCIO, nullptr, &depthRenderPass);
    if(ret != VK_SUCCESS) throw std::runtime_error{"ERROR"};
}
void CSMRenderer::prepareFramebuffer() {
    const auto &device = getMainDevice().logicalDevice;
    std::array<VkImageView, 1> attachments{
        depthAttachment.view()
    };
    auto fbCIO  = FnRenderPass::framebufferCreateInfo(shadow_map_size,shadow_map_size, depthRenderPass,attachments);
    fbCIO.layers = CASCADE_COUNT;
    if (vkCreateFramebuffer(device, &fbCIO, nullptr, &depthFramebuffer)!= VK_SUCCESS) throw std::runtime_error{"ERROR"};

}

void CSMRenderer::drawObjects() {

    const auto &device = mainDevice.logicalDevice;
    std::array<VkDescriptorPoolSize, 2> poolSizes  = {{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_FRAMES_IN_FLIGHT},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * MAX_FRAMES_IN_FLIGHT}
    }};
    VkDescriptorPoolCreateInfo createInfo = FnDescriptor::poolCreateInfo(poolSizes, 20 * MAX_FRAMES_IN_FLIGHT); //
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // allow use free single/multi set: vkFreeDescriptorSets()
    auto result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);

    geoContainer.setRequiredObjects({
        this,  &descPool, {

        };
    });
}



LLVK_NAMESPACE_END
