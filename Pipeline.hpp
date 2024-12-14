//
// Created by star on 4/25/2024.
//

#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include "LLVK_Utils.hpp"

LLVK_NAMESPACE_BEGIN


struct FnPipeline {
    static VkShaderModule createShaderModuleFromSpvFile(const char *shaderPath, VkDevice device);
    static VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage,
        VkShaderModule shaderModule);
    // no push constants layout API
    static VkPipelineLayoutCreateInfo layoutCreateInfo(const VkDescriptorSetLayout* pSetLayouts,uint32_t setLayoutCount = 1);
    static VkPipelineLayoutCreateInfo layoutCreateInfo(const Concept::is_range auto & layouts) ;

    // input asseambly
    static VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo(
            VkPrimitiveTopology topology,
            VkPipelineInputAssemblyStateCreateFlags flags,
            VkBool32 primitiveRestartEnable);

    // vertex bindings & attribs
    static VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo(const Concept::is_range auto & bindings,
        const Concept::is_range auto &attribs) {
        VkPipelineVertexInputStateCreateInfo ret{};
        ret.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        ret.vertexBindingDescriptionCount = bindings.size();
        ret.pVertexBindingDescriptions= bindings.data();
        ret.vertexAttributeDescriptionCount = attribs.size();
        ret.pVertexAttributeDescriptions = attribs.data();
        return ret;
    }
    // empty vertex attribute input
    static VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo(){
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {};
        pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        return pipelineVertexInputStateCreateInfo;
    }

    // viewport and scissor
    static VkPipelineViewportStateCreateInfo viewPortStateCreateInfo(uint32_t viewPortCount=1, uint32_t scissorCount=1);
    //dynamics state
    static std::vector<VkDynamicState> simpleDynamicsStates() {
        std::vector<VkDynamicState> dynamicStates;
        dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
        dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
        dynamicStates.emplace_back(VK_DYNAMIC_STATE_LINE_WIDTH);
        return dynamicStates;
    }
    static VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo(const Concept::is_range auto &dynamicStates) {
        VkPipelineDynamicStateCreateInfo ret{};
        ret.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        ret.dynamicStateCount = dynamicStates.size();
        ret.pDynamicStates = dynamicStates.data();
        return ret;
    }
    //rasterization
    static VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo(
                VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL,
                VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT,
                VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE ,
                VkPipelineRasterizationStateCreateFlags flags = 0);

    static VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo(
                VkSampleCountFlagBits rasterizationSamples,
                VkPipelineMultisampleStateCreateFlags flags = 0);

    // blend attacment
    static constexpr VkPipelineColorBlendAttachmentState simpleOpaqueColorBlendAttacmentState() {
        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
        pipelineColorBlendAttachmentState.colorWriteMask = 0xf;
        pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
        return pipelineColorBlendAttachmentState;
    }
    static VkPipelineColorBlendAttachmentState simpleColorBlendAttacmentState();
    // blend attacment create info
    static VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo(const Concept::is_range auto &attachments);


    static VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfoEnabled();
    static VkGraphicsPipelineCreateInfo pipelineCreateInfo();

};


inline VkShaderModule FnPipeline::createShaderModuleFromSpvFile(const char *shaderPath, VkDevice device) {
    const auto code = readSpvFile(shaderPath);
    const auto module = createShaderModule(device,code);
    return module;
}

inline VkPipelineLayoutCreateInfo FnPipeline::layoutCreateInfo(const VkDescriptorSetLayout *pSetLayouts, uint32_t setLayoutCount) {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
    pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
    return pipelineLayoutCreateInfo;
}
inline VkPipelineLayoutCreateInfo FnPipeline::layoutCreateInfo(const Concept::is_range auto &layouts) {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
    return pipelineLayoutCreateInfo;
}


inline VkPipelineShaderStageCreateInfo FnPipeline::shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule) {
    VkPipelineShaderStageCreateInfo ret{};
    ret.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ret.pName = "main";
    ret.module = shaderModule;
    ret.stage = stage;
    return ret;
}
inline VkPipelineInputAssemblyStateCreateInfo FnPipeline::inputAssemblyStateCreateInfo(VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags, VkBool32 primitiveRestartEnable) {
    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {};
    pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyStateCreateInfo.topology = topology;
    pipelineInputAssemblyStateCreateInfo.flags = flags;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
    return pipelineInputAssemblyStateCreateInfo;
}
inline VkPipelineViewportStateCreateInfo FnPipeline::viewPortStateCreateInfo(uint32_t viewPortCount, uint32_t scissorCount) {
    VkPipelineViewportStateCreateInfo ret{};
    ret.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ret.viewportCount = 1;
    ret.scissorCount = 1;
    return ret;
}

inline VkPipelineRasterizationStateCreateInfo FnPipeline::rasterizationStateCreateInfo(
            VkPolygonMode polygonMode,
            VkCullModeFlags cullMode,
            VkFrontFace frontFace,
            VkPipelineRasterizationStateCreateFlags flags)
{
    VkPipelineRasterizationStateCreateInfo ret {};
    ret.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    ret.polygonMode = polygonMode;
    ret.cullMode = cullMode;
    ret.frontFace = frontFace;
    ret.flags = flags;
    ret.depthClampEnable = VK_FALSE;
    ret.lineWidth = 1.0f;
    ret.depthBiasEnable = VK_FALSE;
    return ret;
}
inline VkPipelineMultisampleStateCreateInfo FnPipeline::multisampleStateCreateInfo(
            VkSampleCountFlagBits rasterizationSamples,
            VkPipelineMultisampleStateCreateFlags flags )
{
    VkPipelineMultisampleStateCreateInfo ret{};
    ret.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ret.rasterizationSamples = rasterizationSamples;
    ret.flags = flags;
    ret.sampleShadingEnable = VK_FALSE;
    return ret;
}

inline VkPipelineColorBlendAttachmentState FnPipeline::simpleColorBlendAttacmentState() {
    VkPipelineColorBlendAttachmentState colorBlend_ATM_ST{};
    colorBlend_ATM_ST.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlend_ATM_ST.blendEnable = VK_TRUE;
    // blend for color : (new alpha * new color) + (1-new alpha) * old color
    colorBlend_ATM_ST.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlend_ATM_ST.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlend_ATM_ST.colorBlendOp = VK_BLEND_OP_ADD;
    // blend for alpha : ( 1 * new alpha ) + (0 * old alpha) = new alpha
    colorBlend_ATM_ST.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlend_ATM_ST.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlend_ATM_ST.alphaBlendOp = VK_BLEND_OP_ADD;
    return colorBlend_ATM_ST;
}


inline VkPipelineColorBlendStateCreateInfo FnPipeline::colorBlendStateCreateInfo(const Concept::is_range auto &attachments) {
    VkPipelineColorBlendStateCreateInfo ret{};
    ret.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ret.logicOpEnable = VK_FALSE;
    ret.attachmentCount = attachments.size();
    ret.pAttachments = attachments.data();
    return ret;
}
inline VkPipelineDepthStencilStateCreateInfo FnPipeline::depthStencilStateCreateInfoEnabled() {
    VkPipelineDepthStencilStateCreateInfo ds_ST_CIO{};
    ds_ST_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds_ST_CIO.depthTestEnable = VK_TRUE;
    ds_ST_CIO.depthWriteEnable = VK_TRUE;
    ds_ST_CIO.depthCompareOp = VK_COMPARE_OP_LESS;
    ds_ST_CIO.depthBoundsTestEnable = VK_FALSE;
    ds_ST_CIO.stencilTestEnable = VK_FALSE;
    ds_ST_CIO.front = {}; // Optional
    ds_ST_CIO.back = {}; // Optional
    return ds_ST_CIO;
}

inline VkGraphicsPipelineCreateInfo FnPipeline::pipelineCreateInfo() {
    VkGraphicsPipelineCreateInfo pipeline_CIO{};
    pipeline_CIO.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // can create multi pipeline that derive from one another for optimisation
    pipeline_CIO.basePipelineHandle = VK_NULL_HANDLE; // exsting pipeline to derive from.
    pipeline_CIO.basePipelineIndex = -1;              // or index of pipeline being created to derive from
    return pipeline_CIO;
}

LLVK_NAMESPACE_END


