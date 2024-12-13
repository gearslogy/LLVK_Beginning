//
// Created by liuya on 9/30/2024.
//

#ifndef UT_PIPELINE_HPP
#define UT_PIPELINE_HPP
#include <vulkan/vulkan.h>
#include "Utils.h"
#include "Pipeline.hpp"
#include "LLVK_GeometryLoader.h"
LLVK_NAMESPACE_BEGIN
struct UT_GraphicsPipelinePSOs{
    UT_GraphicsPipelinePSOs() {
        shaderModules.reserve(5);
        shaderStageCIOs.reserve(5);
        // 2. vertex input
        vertexInputBindingDescriptions =  {GLTFVertex::bindings()};
        vertexInputAttributeDescriptions  = GLTFVertex::attribs();
        vertexInputStageCIO = FnPipeline::vertexInputStateCreateInfo(vertexInputBindingDescriptions, vertexInputAttributeDescriptions);//2
        inputAssemblyCIO = FnPipeline::inputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,0, VK_FALSE); // 3
        viewportStateCIO = FnPipeline::viewPortStateCreateInfo();                     // 4
        dynamicsStates = FnPipeline::simpleDynamicsStates();// 5
        dynamicStateCIO = FnPipeline::dynamicStateCreateInfo(dynamicsStates);
        rasterizerStateCIO = FnPipeline::rasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);//6
        multisampleStateCIO = FnPipeline::multisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);// 7
        // one color blend. if one color attachment
        colorBlendStateCIO = FnPipeline::colorBlendStateCreateInfo(colorBlendAttachmentState); // 8
        depthStencilStateCIO = FnPipeline::depthStencilStateCreateInfoEnabled();    // 10
        pipelineCIO = FnPipeline::pipelineCreateInfo();                             //11
        pipelineCIO.stageCount = 2;
        pipelineCIO.pVertexInputState = &vertexInputStageCIO;
        pipelineCIO.pInputAssemblyState = &inputAssemblyCIO;
        pipelineCIO.pViewportState = &viewportStateCIO;
        pipelineCIO.pDynamicState = &dynamicStateCIO;
        pipelineCIO.pMultisampleState = &multisampleStateCIO;
        pipelineCIO.pDepthStencilState = &depthStencilStateCIO;
        pipelineCIO.pColorBlendState = &colorBlendStateCIO;
        pipelineCIO.pRasterizationState = &rasterizerStateCIO;
        pipelineCIO.subpass = 0; // ONLY USE ONE PASS

    }


    // exmp: vertPath:"shaders/offscreen_vert.spv" fragPath: "shaders/offscreen_frag.spv", with single pipelineLayout
    void asDepth(std::string_view vertPath, std::string_view fragPath, VkRenderPass renderPass) {
        auto device = requiredObjects.device;
        const auto offscreenVertModule = FnPipeline::createShaderModuleFromSpvFile(vertPath.data(),  device);
        const auto offscreenFragModule = FnPipeline::createShaderModuleFromSpvFile(fragPath.data(),  device);
        shaderModules.emplace_back(offscreenVertModule);
        shaderModules.emplace_back(offscreenFragModule);
        shaderStageCIOs.emplace_back(FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, offscreenVertModule));
        shaderStageCIOs.emplace_back(FnPipeline::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, offscreenFragModule));
        pipelineCIO.pStages = shaderStageCIOs.data();
        rasterizerStateCIO.cullMode = VK_CULL_MODE_NONE; // All faces contribute shadow
        // on color attachment
        colorBlendStateCIO.attachmentCount = 0;
        colorBlendStateCIO.logicOpEnable = VK_FALSE;
        pipelineCIO.pColorBlendState = &colorBlendStateCIO;
        pipelineCIO.renderPass = renderPass;

        /*
        // we must build it manully
        const std::array offscreenSetLayouts{offscreenDescriptorSetLayout};
        VkPipelineLayoutCreateInfo offscreenSetLayout_CIO = FnPipeline::layoutCreateInfo(offscreenSetLayouts); // ONLY ONE SET
        UT_Fn::invoke_and_check("ERROR create offscreen pipeline layout",vkCreatePipelineLayout,device, &offscreenSetLayout_CIO,nullptr, &offscreenPipelineLayout );
        pipelineCIO.layout = offscreenPipelineLayout;*/
    };

    void setShaderStages(auto && ... stage) {
        shaderStageCIOs.clear();
        pipelineCIO.stageCount = sizeof...(stage);
        (shaderStageCIOs.emplace_back(stage), ...);
        pipelineCIO.pStages = shaderStageCIOs.data();
    }
    void setRenderPass(VkRenderPass renderPass) {pipelineCIO.renderPass = renderPass;}
    void setShaderModule(auto && ... module) { (shaderModules.emplace_back(module), ... ); }
    void setPipelineLayout(VkPipelineLayout layout) { pipelineCIO.layout = layout;}
    // after pipeline creation, call this function
    void cleanupShaderModule() { UT_Fn::cleanup_shader_module(requiredObjects.device, shaderModules);}

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCIOs; // 1 : vert frag ...
    VkPipelineVertexInputStateCreateInfo vertexInputStageCIO{};   // 2
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCIO{};    // 3
    VkPipelineViewportStateCreateInfo viewportStateCIO{};         // 4
    VkPipelineDynamicStateCreateInfo dynamicStateCIO{};           // 5
    VkPipelineRasterizationStateCreateInfo rasterizerStateCIO{};  // 6
    VkPipelineMultisampleStateCreateInfo multisampleStateCIO{};   // 7
    VkPipelineColorBlendStateCreateInfo colorBlendStateCIO{};     // 8
        // 9
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCIO{}; //10
    VkGraphicsPipelineCreateInfo pipelineCIO{};                   //11

    std::vector<VkShaderModule> shaderModules{};
    // interface required
    struct {
        VkDevice device{};
    }requiredObjects;

    static void createPipeline(const VkDevice &device, const UT_GraphicsPipelinePSOs &pso,
                               const VkPipelineCache &cache, VkPipeline &pipeline) {
        UT_Fn::invoke_and_check("error create scene opacity pipeline", vkCreateGraphicsPipelines,
                                device,
                                cache,
                                1, &pso.pipelineCIO,
                                nullptr,
                                &pipeline);
    }

private:
    //Before we can create a pipeline, we must ensure that these objects are persistent
    std::vector<VkDynamicState> dynamicsStates;
    std::array<VkVertexInputBindingDescription,1> vertexInputBindingDescriptions{};      // binding = 0 ,only one vertex buffer, may be in instance,should be one more
    std::array<VkVertexInputAttributeDescription,7> vertexInputAttributeDescriptions{};  // binding = 0, 7 attributes
    std::array<VkPipelineColorBlendAttachmentState,1> colorBlendAttachmentState = { FnPipeline::simpleOpaqueColorBlendAttacmentState()};
};



LLVK_NAMESPACE_END






#endif //UT_PIPELINE_HPP
