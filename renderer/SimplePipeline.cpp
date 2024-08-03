//
// Created by lp on 2024/7/13.
//

#include "SimplePipeline.h"
#include <array>
#include "GeoVertexDescriptions.h"
#include "PipelineCache.h"
#include "PushConstant.hpp"
LLVK_NAMESPACE_BEGIN
void SimplePipeline::cleanup() {
    vkDestroyPipeline(bindDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(bindDevice, pipelineLayout, nullptr);
}

void SimplePipeline::init() {
    using FnPipeline = LLVK::FnPipeline;
    const auto vertModule = FnPipeline::createShaderModuleFromSpvFile("shaders/shader_vert.spv",  bindDevice);
    const auto fragModule = FnPipeline::createShaderModuleFromSpvFile("shaders/shader_frag.spv",  bindDevice);
    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule);
    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule);
    // 1
    VkPipelineShaderStageCreateInfo shaderStates[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};
    // 2. vertex input
    std::array bindings = {Basic::Vertex::bindings()};
    auto attribs = Basic::Vertex::attribs();
    VkPipelineVertexInputStateCreateInfo vertexInput_ST_CIO = FnPipeline::vertexInputStateCreateInfo(bindings, attribs);
    // 3. assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly_ST_CIO = FnPipeline::inputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, VK_FALSE);
    // 4 viewport and scissor
    VkPipelineViewportStateCreateInfo viewport_ST_CIO = FnPipeline::viewPortStateCreateInfo();
    // 5. dynamic state
    VkPipelineDynamicStateCreateInfo dynamics_ST_CIO{};
    std::vector<VkDynamicState> dynamicStates;
    dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
    dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStates.emplace_back(VK_DYNAMIC_STATE_LINE_WIDTH);
    dynamics_ST_CIO = FnPipeline::dynamicStateCreateInfo(dynamicStates);

    VkPipelineRasterizationStateCreateInfo rasterization_ST_CIO = FnPipeline::rasterizationStateCreateInfo();    // 6. rasterization
    VkPipelineMultisampleStateCreateInfo multisample_ST_CIO=FnPipeline::multisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);     // 7. multisampling
    // 8. blending
    std::array colorBlendAttamentState = {FnPipeline::simpleOpaqueColorBlendAttacmentState()};
    VkPipelineColorBlendStateCreateInfo blend_ST_CIO = FnPipeline::colorBlendStateCreateInfo(colorBlendAttamentState);

    // 9. pipeline layout
    VkPipelineLayoutCreateInfo layout_CIO{};
    layout_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_CIO.setLayoutCount = bindDescriptorSetLayouts.size();
    layout_CIO.pSetLayouts = bindDescriptorSetLayouts.data();
    layout_CIO.pushConstantRangeCount = PushConstant::count;
    layout_CIO.pPushConstantRanges = PushConstant::pushRanges;

    // create pipeline layout
    auto result = vkCreatePipelineLayout(bindDevice, &layout_CIO, nullptr, &pipelineLayout);
    if(result != VK_SUCCESS) throw std::runtime_error{"ERROR create pipeline layout"};

    // 10
    VkPipelineDepthStencilStateCreateInfo ds_ST_CIO = FnPipeline::depthStencilStateCreateInfoEnabled();
    // 11. PIPELINE
    VkGraphicsPipelineCreateInfo pipeline_CIO{};
    pipeline_CIO.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_CIO.stageCount = 2;
    pipeline_CIO.pStages = shaderStates;
    pipeline_CIO.pVertexInputState = &vertexInput_ST_CIO;
    pipeline_CIO.pInputAssemblyState = &inputAssembly_ST_CIO;
    pipeline_CIO.pViewportState = &viewport_ST_CIO;
    pipeline_CIO.pDynamicState = &dynamics_ST_CIO;
    pipeline_CIO.pRasterizationState = &rasterization_ST_CIO;
    pipeline_CIO.pMultisampleState = &multisample_ST_CIO;
    pipeline_CIO.pColorBlendState = &blend_ST_CIO;
    pipeline_CIO.pDepthStencilState = &ds_ST_CIO;
    pipeline_CIO.layout = pipelineLayout;
    pipeline_CIO.renderPass = bindRenderPass ;
    pipeline_CIO.subpass = 0; // ONLY USE ONE PASS
    // can create multi pipeline that derive from one atnoher for optimisation
    pipeline_CIO.basePipelineHandle = VK_NULL_HANDLE; // exsting pipeline to derive from.
    pipeline_CIO.basePipelineIndex = -1;              // or index of pipeline being created to derive from
    result = vkCreateGraphicsPipelines(bindDevice, bindPipelineCache->pipelineCache, 1, &pipeline_CIO, nullptr, &graphicsPipeline);
    if(result!= VK_SUCCESS) throw std::runtime_error{"Failed created graphics pipeline"};
    // finally destory shader module
    vkDestroyShaderModule(bindDevice, vertModule, nullptr);
    vkDestroyShaderModule(bindDevice, fragModule, nullptr);
}
LLVK_NAMESPACE_END