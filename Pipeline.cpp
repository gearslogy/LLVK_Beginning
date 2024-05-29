//
// Created by star on 4/25/2024.
//

#include "Pipeline.h"
#include "Utils.h"
#include "GeoVertexDescriptions.h"

void Pipeline::cleanup() {
    vkDestroyPipeline(bindDevice, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(bindDevice, pipelineLayout, nullptr);
}

void Pipeline::init() {
    const auto vertCode = readSpvFile("shaders/vert_shader.spv");
    const auto fragCode = readSpvFile("shaders/frag_shader.spv");

    const auto vertModule = createShaderModule(bindDevice,vertCode);
    const auto fragModule = createShaderModule(bindDevice,fragCode);

    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
    vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageCreateInfo.pName = "main";
    vertShaderStageCreateInfo.module = vertModule;
    vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
    fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageCreateInfo.pName = "main";
    fragShaderStageCreateInfo.module = fragModule;
    fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    // 1
    VkPipelineShaderStageCreateInfo shaderStates[] = {vertShaderStageCreateInfo, fragShaderStageCreateInfo};

    // 2. vertex input
    auto bindings = Vertex::bindings();
    auto attribs = Vertex::attribs();
    VkPipelineVertexInputStateCreateInfo vertexInput_ST_CIO{};
    vertexInput_ST_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput_ST_CIO.vertexBindingDescriptionCount = 1;
    vertexInput_ST_CIO.vertexAttributeDescriptionCount = attribs.size();
    vertexInput_ST_CIO.pVertexBindingDescriptions= &bindings;
    vertexInput_ST_CIO.pVertexAttributeDescriptions = attribs.data();
    // 3. assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly_ST_CIO{};
    inputAssembly_ST_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly_ST_CIO.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly_ST_CIO.primitiveRestartEnable = false;
    // 4. viewport and scissor
    /*
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(bindExtent.width);
    viewport.height = static_cast<float>(bindExtent.height);
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    VkRect2D scissor{
        {0,0},
        bindExtent
    };*/
    VkPipelineViewportStateCreateInfo viewport_ST_CIO{};
    viewport_ST_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_ST_CIO.viewportCount = 1;
    //viewport_ST_CIO.pViewports = &viewport;
    viewport_ST_CIO.scissorCount = 1;
    //viewport_ST_CIO.pScissors = &scissor;


    // 5. dynamic state
    VkPipelineDynamicStateCreateInfo dynamics_ST_CIO{};
    dynamics_ST_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    std::vector<VkDynamicState> dynamicStates;
    dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
    dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStates.emplace_back(VK_DYNAMIC_STATE_LINE_WIDTH);
    dynamics_ST_CIO.dynamicStateCount = dynamicStates.size();
    dynamics_ST_CIO.pDynamicStates = dynamicStates.data();

    // 6. rasterization
    VkPipelineRasterizationStateCreateInfo rasterization_ST_CIO{};
    rasterization_ST_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_ST_CIO.cullMode = VK_CULL_MODE_BACK_BIT;
    //rasterization_ST_CIO.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_ST_CIO.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_ST_CIO.depthClampEnable = VK_FALSE;
    rasterization_ST_CIO.rasterizerDiscardEnable = VK_FALSE;
    rasterization_ST_CIO.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_ST_CIO.lineWidth = 1.0f;
    rasterization_ST_CIO.depthBiasEnable = VK_FALSE;

    // 7. multisampling
    VkPipelineMultisampleStateCreateInfo multisample_ST_CIO{};
    multisample_ST_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_ST_CIO.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_ST_CIO.sampleShadingEnable = VK_FALSE;

    // 8. blending
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
    VkPipelineColorBlendStateCreateInfo blend_ST_CIO{};
    blend_ST_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_ST_CIO.logicOpEnable = VK_FALSE;
    blend_ST_CIO.attachmentCount = 1;
    blend_ST_CIO.pAttachments = &colorBlend_ATM_ST;
    // 9. pipeline layout
    VkPipelineLayoutCreateInfo layout_CIO{};
    layout_CIO.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_CIO.setLayoutCount = 1;
    layout_CIO.pSetLayouts = &bindDescriptorSetLayout;
    layout_CIO.pushConstantRangeCount = 0;
    layout_CIO.pPushConstantRanges = nullptr;

    // create pipeline layout
    auto result = vkCreatePipelineLayout(bindDevice, &layout_CIO, nullptr, &pipelineLayout);
    if(result != VK_SUCCESS) throw std::runtime_error{"ERROR create pipeline layout"};

    // 10 TODO depth stencil testing
    VkPipelineDepthStencilStateCreateInfo ds_ST_;

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
    pipeline_CIO.pDepthStencilState = nullptr;
    pipeline_CIO.layout = pipelineLayout;
    pipeline_CIO.renderPass = bindRenderPass ;
    pipeline_CIO.subpass = 0; // ONLY USE ONE PASS
    // can create multi pipeline that derive from one atnoher for optimisation
    pipeline_CIO.basePipelineHandle = VK_NULL_HANDLE; // exsting pipeline to derive from.
    pipeline_CIO.basePipelineIndex = -1;              // or index of pipeline being created to derive from
    result = vkCreateGraphicsPipelines(bindDevice, VK_NULL_HANDLE, 1, &pipeline_CIO, nullptr, &graphicsPipeline);
    if(result!= VK_SUCCESS) throw std::runtime_error{"Failed created graphics pipeline"};
    // finally destory shader module
    vkDestroyShaderModule(bindDevice, vertModule, nullptr);
    vkDestroyShaderModule(bindDevice, fragModule, nullptr);
}
