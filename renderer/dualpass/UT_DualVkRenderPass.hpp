//
// Created by lp on 11/20/2024.
//

#ifndef DUALVKRENDERPASS_HPP
#define DUALVKRENDERPASS_HPP

#include <array>
#include <LLVK_UT_Pipeline.hpp>
#include <Pipeline.hpp>

#include "LLVK_SYS.hpp"
#include <stdexcept>
#include <iostream>
LLVK_NAMESPACE_BEGIN
namespace UT_DualRenderPass {
    inline VkAttachmentDescription createColorDescription() {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return colorAttachment;
    }
    inline VkAttachmentDescription createDepthDescription() {
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        return depthAttachment;
    }


    inline VkRenderPass depthOnlyPass(VkDevice device ) {
        VkAttachmentDescription attachmentDescription{};
        attachmentDescription.format = VK_FORMAT_D32_SFLOAT;
        attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
        attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
        attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
        attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

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

        VkRenderPass depthPrePass;
        if (vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &depthPrePass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create depth pre-pass render pass!");
        }
        return depthPrePass;
    }

    inline VkRenderPass pass1(VkDevice device ) {
        VkRenderPass renderPass{};
        auto colorAttachment = createColorDescription();
        colorAttachment.initialLayout= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        auto depthAttachment = createDepthDescription();
        depthAttachment.initialLayout= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.finalLayout= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        std::array attachments = {colorAttachment, depthAttachment};

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // 表示来自前一个renderpass
        dependency.dstSubpass = 0;                    // 当前renderpass的第一个subpass
        // 前一个操作(深度写入)

        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;    // before
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;   // after
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT ;

        // 创建renderpass时添加依赖
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        // create
        auto ret = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
        if(ret != VK_SUCCESS) throw std::runtime_error{"ERROR"};

        return renderPass;
    }


    inline VkRenderPass pass2(VkDevice device ) {
        VkRenderPass renderPass{};
        auto colorAttachment = createColorDescription();
        colorAttachment.initialLayout= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // 加载第一个pass的颜色
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        auto depthAttachment = createDepthDescription();
        depthAttachment.initialLayout= VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;  // 加载第一个pass的深度
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        std::array attachments = {colorAttachment, depthAttachment};

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // 表示来自前一个renderpass
        dependency.dstSubpass = 0;                    // 当前renderpass的第一个subpass
        // 前一个操作(深度写入)
        // before
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;

        // 创建renderpass时添加依赖
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        // create
        auto ret = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
        if(ret != VK_SUCCESS) throw std::runtime_error{"ERROR"};

        return renderPass;
    }

    inline VkFramebuffer createDepthFramebuffer( VkDevice device, VkRenderPass mainPass,
     VkImageView depthImageView,
     uint32_t width,
     uint32_t height) {

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mainPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &depthImageView;
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer;
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create main pass framebuffer!");
        }
        return framebuffer;
    }


    inline VkFramebuffer createFramebuffer( VkDevice device, VkRenderPass mainPass,
        VkImageView colorImageView,
        VkImageView depthImageView,
        uint32_t width,
        uint32_t height) {

        std::array<VkImageView, 2> attachments = {
            colorImageView,
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mainPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer;
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create main pass framebuffer!");
        }
        return framebuffer;
    }



    inline void createPipelines(const VkDevice &device,const VkRenderPass&pass1,
        const VkRenderPass &pass2, const VkDescriptorSetLayout &setLayout, const VkPipelineCache &pipelineCache,
        VkPipelineLayout &dualPipelineLayout, VkPipeline &pipe1, VkPipeline &pipe2) {
        UT_GraphicsPipelinePSOs dualPso{};
        UT_GraphicsPipelinePSOs dualPso1{};

        const std::array sceneSetLayouts{setLayout};
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
        dualPso.setRenderPass(pass1);
        dualPso.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
        dualPso.rasterizerStateCIO.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;



        auto passBlendAS = FnPipeline::simpleColorBlendAttacmentState();
        passBlendAS.blendEnable = VK_FALSE;
        std::array colorBlendAttachmentsPass1{passBlendAS};
        VkPipelineColorBlendStateCreateInfo colorBlendStateCIO1 = FnPipeline::colorBlendStateCreateInfo(colorBlendAttachmentsPass1);

        dualPso.pipelineCIO.pColorBlendState = &colorBlendStateCIO1;
        UT_GraphicsPipelinePSOs::createPipeline(device, dualPso, pipelineCache, pipe1);
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
        colorBlendAttachmentPass2.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // 源颜色使用源Alpha作为因子
        colorBlendAttachmentPass2.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // 目标颜色使用1-源Alpha作为因子
        colorBlendAttachmentPass2.colorBlendOp = VK_BLEND_OP_ADD;
        // alpha 混合设置
        colorBlendAttachmentPass2.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentPass2.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentPass2.alphaBlendOp = VK_BLEND_OP_MAX;

        /*
        colorBlendAttachmentPass2.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachmentPass2.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachmentPass2.colorBlendOp = VK_BLEND_OP_ADD;

        colorBlendAttachmentPass2.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentPass2.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachmentPass2.alphaBlendOp = VK_BLEND_OP_ADD;*/

        std::array colorBlendAttachmentsPass2{colorBlendAttachmentPass2};
        VkPipelineColorBlendStateCreateInfo colorBlendStateCIO2 = FnPipeline::colorBlendStateCreateInfo(colorBlendAttachmentsPass2);
        colorBlendStateCIO2.blendConstants[0] = VK_BLEND_FACTOR_ZERO;
        colorBlendStateCIO2.blendConstants[1] = VK_BLEND_FACTOR_ZERO;
        colorBlendStateCIO2.blendConstants[2] = VK_BLEND_FACTOR_ONE;
        colorBlendStateCIO2.blendConstants[3] = VK_BLEND_FACTOR_ZERO;


        dualPso1.pipelineCIO.pColorBlendState = &colorBlendStateCIO2;
        dualPso1.rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE;
        dualPso1.depthStencilStateCIO.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        dualPso1.depthStencilStateCIO.depthTestEnable = VK_TRUE;
        dualPso1.depthStencilStateCIO.depthWriteEnable = VK_FALSE;

        const auto vs1MD = FnPipeline::createShaderModuleFromSpvFile("shaders/hair_vert.spv",  device);
        const auto fs1MD = FnPipeline::createShaderModuleFromSpvFile("shaders/back_frag.spv",  device);
        VkPipelineShaderStageCreateInfo pass2_vsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vs1MD);
        VkPipelineShaderStageCreateInfo pass2_fsCIO = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fs1MD);
        dualPso1.setShaderStages(pass2_vsCIO, pass2_fsCIO);
        dualPso1.setPipelineLayout(dualPipelineLayout);
        dualPso1.setRenderPass(pass2);
        UT_GraphicsPipelinePSOs::createPipeline(device, dualPso1, pipelineCache, pipe2);

        UT_Fn::cleanup_shader_module(device,vs0MD, fs0MD, vs1MD,fs1MD);
        std::cout << "created back pipeline\n";
    }




}


LLVK_NAMESPACE_END















#endif //DUALVKRENDERPASS_HPP
