//
// Created by liuya on 11/8/2024.
//

#include "CSMDepthPass.h"

#include <execution>
#include <LLVK_Descriptor.hpp>

#include "LLVK_Image.h"
#include "renderer/public/UT_CustomRenderer.hpp"
#include "CSMRenderer.h"
#include <functional>
LLVK_NAMESPACE_BEGIN


CSMDepthPass::CSMDepthPass(const CSMRenderer *renderer):pRenderer(renderer){}

void CSMDepthPass::prepare() {
    prepareDepthResources();
    prepareDepthRenderPass();
    preparePipelines();
    prepareUniformBuffers();
}

void CSMDepthPass::cleanup() {
    auto device = pRenderer->getMainDevice().logicalDevice;
    UT_Fn::cleanup_resources(depthAttachment);
    UT_Fn::cleanup_sampler(device, depthSampler);
    vkDestroyRenderPass(device, depthRenderPass, nullptr);
    vkDestroyFramebuffer(device, depthFramebuffer, nullptr);
    UT_Fn::cleanup_range_resources(uboGeomBuffer);
    UT_Fn::cleanup_pipeline(device, normalPipeline, instancePipeline);
}

void CSMDepthPass::prepareDepthResources() {
    auto device = pRenderer->getMainDevice().logicalDevice;
    depthSampler = FnImage::createDepthSampler(device);
    setRequiredObjectsByRenderer(pRenderer, depthAttachment);
    depthAttachment.create2dArrayDepth32(width,width, cascade_count, depthSampler);
}

void CSMDepthPass::prepareDepthRenderPass() {
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
    framebufferInfo.height = width;          // FIXED shadow map size;
    framebufferInfo.layers = cascade_count;
    UT_Fn::invoke_and_check("create csm framebuffer failed", vkCreateFramebuffer,device, &framebufferInfo, nullptr, &depthFramebuffer);

}

void CSMDepthPass::preparePipelines() {
    const auto &device = pRenderer->getMainDevice().logicalDevice;

    const auto vertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/csm_depth_vert.spv",  device);
    const auto geomModule = FnPipeline::createShaderModuleFromSpvFile("shaders/csm_depth_geom.spv",  device);
    const auto fragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/csm_depth_frag.spv",  device);

    uint32_t enableInstance{0};
    VkSpecializationMapEntry specMapEntry = {0,0, sizeof(uint32_t)}; // one constant
    VkSpecializationInfo specInfo = {};
    specInfo.mapEntryCount = 1;
    specInfo.pMapEntries = &specMapEntry;
    specInfo.dataSize = sizeof(uint32_t);
    specInfo.pData = &enableInstance;


    VkPipelineShaderStageCreateInfo vertSSCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule);
    vertSSCIO.pSpecializationInfo = &specInfo; // inject specialize var
    VkPipelineShaderStageCreateInfo geomSSCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT, geomModule);
    VkPipelineShaderStageCreateInfo fragSSCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule);

    pso.setShaderStages(vertSSCIO, geomSSCIO, fragSSCIO);
    pso.setPipelineLayout(pRenderer->pipelineLayout);
    pso.setRenderPass(pRenderer->getMainRenderPass());
    // pipeline1
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), normalPipeline);

    // pipeline2
    enableInstance = 1;
    UT_GraphicsPipelinePSOs::createPipeline(device, pso, pRenderer->getPipelineCache(), instancePipeline);
    UT_Fn::cleanup_shader_module(device,vertModule, geomModule, fragModule);


}


void CSMDepthPass::prepareUniformBuffers() {
    setRequiredObjectsByRenderer(pRenderer, uboGeomBuffer[0], uboGeomBuffer[1]);
    for(auto &ubo : uboGeomBuffer)
        ubo.createAndMapping(sizeof(uboGeomData));
}



void CSMDepthPass::recordCommandBuffer() {
    std::vector<VkClearValue> clearValues(1);
    clearValues[0].depthStencil = { 1.0f, 0 };
    VkExtent2D shadowExtent{width, width};
    auto renderPassBeginInfo = FnCommand::renderPassBeginInfo(depthFramebuffer, depthRenderPass, shadowExtent, clearValues);
    const auto &cmdBuf = pRenderer->getMainCommandBuffer();
    vkCmdBeginRenderPass(cmdBuf,&renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

    auto viewport = FnCommand::viewport(width, width );
    auto scissor = FnCommand::scissor(width, width );
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuf,0, 1, &scissor);
    pRenderer->renderGeometry(instancePipeline, normalPipeline);
    vkCmdEndRenderPass(cmdBuf);
}


void CSMDepthPass::update() {
    constexpr std::array<glm::vec3, 8> ndcFrustumCorners = {
        {
            // NDC near plane
            {-1.0f,  1.0f, 0.0f},
           { 1.0f,  1.0f, 0.0f},
           { 1.0f, -1.0f, 0.0f},
           {-1.0f, -1.0f, 0.0f},
            //NDC far plane
           {-1.0f,  1.0f,  1.0f},
           { 1.0f,  1.0f,  1.0f},
           { 1.0f, -1.0f,  1.0f},
           {-1.0f, -1.0f,  1.0f}
        }
    };
    auto getFrustumCenter = [](const std::array<glm::vec3, 8> &corners) {
        return std::ranges::fold_left(corners, glm::vec3{0.0f},std::plus<>()) / static_cast<float>(std::size(corners));
    };


    // 计算切割百分比
    float cascadeSplits[cascade_count];
    const auto nearClip = pRenderer->mainCamera.mNear;
    const auto farClip =pRenderer->mainCamera.mFar;
    const float clipRange = farClip - nearClip;
    const float minZ = nearClip;
    const float maxZ = nearClip + clipRange;
    const float range = maxZ - minZ;
    const float ratio = maxZ / minZ;

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32_t i = 0; i < cascade_count; i++) {
        float p = static_cast<float>(i + 1) / static_cast<float>(cascade_count);
        float log = minZ * std::pow(ratio, p);
        float uniform = minZ + range * p;
        float d = cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearClip) / clipRange;
    }// 经过我的测试，当cascade_count = 3 时候。 这个cascadeSplits[] = {   0.0197262 0.0887107 1 }, 也就是可以自己定义：比如0.06   0.2   1


    float lastSplitDist{0.0f};
    for (int i=0; i < cascade_count; i++) {
        float splitDist = cascadeSplits[i];
        // world space frustum
        glm::mat4 invCam = glm::inverse(pRenderer->mainCamera.projection() * pRenderer->mainCamera.view());
        auto wpFrustumCornersIter = ndcFrustumCorners | std::views::transform([&invCam](const glm::vec3 &corner) {
            return invCam * glm::vec4{corner, 1.0f};
        });
        std::array<glm::vec3, 8> wpFrustumCorners{};
        std::ranges::copy(wpFrustumCornersIter, wpFrustumCorners.begin());

        for (uint32_t j = 0; j < 4; j++) {
            glm::vec3 dist = wpFrustumCorners[j + 4] - wpFrustumCorners[j]; // per points distance dir : near plane -> far plane_corner
            wpFrustumCorners[j + 4] = wpFrustumCorners[j] + (dist * splitDist); // 设置远平面位置
            wpFrustumCorners[j] = wpFrustumCorners[j] + (dist * lastSplitDist); // 近平面从0开始，所以使用lastSplitDist迭代
        }
        const auto wpFrustumCenter = getFrustumCenter(wpFrustumCorners);
        auto radius = std::ranges::max(
            wpFrustumCorners | std::views::transform(
                [&wpFrustumCenter](const auto &corner) {
                    return glm::length(corner - wpFrustumCenter);
                }
            )
        );
        // 量化到 1/16 单位
        radius = std::ceil(radius * 16.0f) / 16.0f;

        auto maxExtents = glm::vec3(radius);
        glm::vec3 minExtents = -maxExtents;
        glm::vec3 lightDir = glm::normalize(-pRenderer->lightPos);
        glm::mat4 lightViewMatrix = glm::lookAt(wpFrustumCenter - lightDir * -minExtents.z, wpFrustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);


        cascadesSplitDepth[i] = (nearClip + splitDist * clipRange) * -1.0f; // -1 很关键，适配到了材质系统
        cascadesViewProjMatrix[i] = lightOrthoMatrix * lightViewMatrix;     // 求出来的灯光矩阵。直接用到geometry shader上

        lastSplitDist = cascadeSplits[i];

    }
    // update geom shader: ubo for matrices write
    for (const auto &&[k, v]: UT_Fn::enumerate(cascadesViewProjMatrix)) {
        uboGeomData.lightViewProj[k] = v;
    }
    memcpy(uboGeomBuffer[pRenderer->getCurrentFrame()].mapped, &uboGeomData, sizeof(uboGeomData));
}



LLVK_NAMESPACE_END
